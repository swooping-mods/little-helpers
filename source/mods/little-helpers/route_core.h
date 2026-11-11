#ifndef LITTLE_HELPERS_ROUTE_CORE_H
#define LITTLE_HELPERS_ROUTE_CORE_H
/* Incremental local-area A*. Unlike the short steering grid, this may search
 * behind the worker to find a room exit. Only verified edges enter the route. */
#define LR_SIDE 49
#define LR_NODES (LR_SIDE*LR_SIDE)
#define LR_PATH 256
enum {LR_SEARCHING=1,LR_READY,LR_FAILED};
typedef int (*LREdge)(void *,SPVec3,SPVec3,SPVec3 *);
typedef struct {SPVec3 point;float cost;short parent;unsigned char state;} LRNode;
typedef struct {
    SPVec3 origin,right,forward,goal;
    LRNode nodes[LR_NODES];unsigned short path[LR_PATH];
    unsigned state,count,index,expanded,last;SPVec3 endpoint;
} LHRoute;
static float lr_abs(float f){return f<0?-f:f;}
static float lr_h(const LHRoute *r,SPVec3 point){SPVec3 d=lh_sub(r->goal,point);return lr_abs(lh_dot(d,r->right))+lr_abs(lh_dot(d,r->forward));}
static void lr_begin(LHRoute *r,SPVec3 position,SPVec3 right,SPVec3 forward,SPVec3 goal){
    lh_zero(r,sizeof(*r));r->origin=position;r->right=right;r->forward=forward;r->goal=goal;r->state=LR_SEARCHING;
    unsigned start=(LR_SIDE/2)*LR_SIDE+LR_SIDE/2;r->nodes[start].point=position;r->nodes[start].state=1;r->nodes[start].parent=-1;
}
static unsigned lr_accept(LHRoute *r,unsigned node,SPVec3 endpoint){
    if(node>=LR_NODES||r->nodes[node].state!=2)return LR_FAILED;
    r->count=r->index=0;r->endpoint=endpoint;int cursor=(int)node;
    while(r->nodes[cursor].parent>=0){
        if(r->count==LR_PATH){r->state=LR_FAILED;return r->state;}
        r->path[r->count++]=(unsigned short)cursor;cursor=r->nodes[cursor].parent;
    }
    for(unsigned i=0;i<r->count/2;i++){unsigned short n=r->path[i];r->path[i]=r->path[r->count-1-i];r->path[r->count-1-i]=n;}
    r->state=LR_READY;return r->state;
}
static unsigned lr_step(LHRoute *r,LREdge edge,void *ctx,unsigned budget){
    static const int dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
    if(r->state!=LR_SEARCHING)return r->state;
    for(unsigned work=0;work<budget;work++){
        int best=-1;float score=1e20f;
        for(unsigned i=0;i<LR_NODES;i++)if(r->nodes[i].state==1){float f=r->nodes[i].cost+lr_h(r,r->nodes[i].point);if(f<score){score=f;best=(int)i;}}
        if(best<0){r->state=LR_FAILED;return r->state;}
        LRNode *a=&r->nodes[best];a->state=2;r->expanded++;r->last=(unsigned)best;
        if(lr_h(r,a->point)<1.25f&&edge(ctx,a->point,r->goal,&r->endpoint)){
            return lr_accept(r,(unsigned)best,r->endpoint);
        }
        for(unsigned side=0;side<4;side++){
            int x=best%LR_SIDE+dx[side],y=best/LR_SIDE+dy[side];if(x<0||y<0||x>=LR_SIDE||y>=LR_SIDE)continue;
            int next=y*LR_SIDE+x;LRNode *b=&r->nodes[next];float cost=a->cost+1;
            if(b->state==2||(b->state==1&&cost>=b->cost))continue;
            SPVec3 desired=lh_add(r->origin,lh_add(lh_mul(r->right,(float)(x-LR_SIDE/2)),lh_mul(r->forward,(float)(y-LR_SIDE/2)))),point;
            if(!edge(ctx,a->point,desired,&point))continue;
            b->point=point;b->cost=cost;b->parent=(short)best;b->state=1;
        }
    }
    return r->state;
}
static SPVec3 lr_waypoint(LHRoute *r,SPVec3 current){
    while(r->index<r->count&&lh_distance2(current,r->nodes[r->path[r->index]].point)<.18f*.18f)r->index++;
    return r->index<r->count?r->nodes[r->path[r->index]].point:r->endpoint;
}
#endif
