#ifndef LITTLE_HELPERS_ROUTE_PROBE_H
#define LITTLE_HELPERS_ROUTE_PROBE_H
/* Private test observations wrap the real callbacks once, preserving their
 * context, arguments and return values. An accepted edge never becomes a
 * rejection; failed optional ground probes are not themselves edge failures. */
enum {LHP_NONE=0,LHP_UP,LHP_GROUND,LHP_CLEAR,LHP_POLICY,LHP_KINDS};
typedef struct {unsigned id;SPVec3 point,normal,center,extents;char name[96];} LHProbeCollider;
typedef struct {unsigned stage,count,observed,query_ok,saturated,identity_enabled;SPVec3 from,to,up;float radius,height;LHProbeCollider colliders[3];} LHProbePhysics;
typedef struct {unsigned reason,provider,id;SPVec3 position;float radius;LHProbePhysics physics;} LHProbeDetail;
typedef struct {
    SPVec3 from,to,query,up,clear_to;
    LHProbeDetail detail;
    SPGround ground;
    float drop,radius,height,score;
    SPU64 support;
    unsigned callback,result,ground_seen,rejection_kind;
} LHProbeEdge;
typedef struct {
    unsigned attempts,accepted,rejected,counts[LHP_KINDS];
    LHProbeEdge nearest[LHP_KINDS],recent[32];
} LHRouteProbe;
typedef struct {SPWorld inner;LHProbeEdge edge;const LHProbeDetail *detail;} LHProbeCall;
static void lhp_zero(void *value,unsigned count){unsigned char *p=value;while(count--)*p++=0;}
static int lhp_up(void *context,SPVec3 point,SPVec3 *up){
    LHProbeCall *p=context;int ok=p->inner.up(p->inner.ctx,point,up);
    p->edge.callback=LHP_UP;p->edge.result=(unsigned)ok;p->edge.query=point;
    if(ok)p->edge.up=*up;return ok;
}
static int lhp_ground(void *context,SPVec3 origin,SPVec3 up,float drop,SPGround *ground){
    LHProbeCall *p=context;int ok=p->inner.ground(p->inner.ctx,origin,up,drop,ground);
    p->edge.callback=LHP_GROUND;p->edge.result=(unsigned)ok;p->edge.query=origin;p->edge.up=up;p->edge.drop=drop;
    if(ok){p->edge.ground=*ground;p->edge.ground_seen=1;}return ok;
}
static int lhp_clear(void *context,SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height,SPU64 support){
    LHProbeCall *p=context;int ok=p->inner.clear(p->inner.ctx,from,to,up,radius,height,support);
    p->edge.callback=LHP_CLEAR;p->edge.result=(unsigned)ok;p->edge.query=from;p->edge.clear_to=to;p->edge.up=up;
    p->edge.radius=radius;p->edge.height=height;p->edge.support=support;if(p->detail)p->edge.detail=*p->detail;return ok;
}
static int lhp_support(void *context,SPU64 id,SPTransform *out){LHProbeCall *p=context;return p->inner.support?p->inner.support(p->inner.ctx,id,out):0;}
static int lhp_edge(SPCore *core,SPPet *pet,const SPWorld *world,SPVec3 from,SPVec3 to,SPVec3 *end,SPVec3 *up,SPGround *ground,LHRouteProbe *store,float score,const LHProbeDetail *detail){
    LHProbeCall call;lhp_zero(&call,sizeof(call));call.inner=*world;call.detail=detail;call.edge.from=from;call.edge.to=to;call.edge.score=score;
    SPWorld wrapped={&call,lhp_up,lhp_ground,lhp_clear,world->support?lhp_support:0};
    int ok=sp_edge(core,pet,&wrapped,from,to,end,up,ground,0);store->attempts++;
    if(ok){store->accepted++;return ok;}
    store->rejected++;
    unsigned kind=call.edge.callback&& !call.edge.result?call.edge.callback:LHP_POLICY;
    call.edge.rejection_kind=kind;store->recent[(store->rejected-1)%32]=call.edge;
    if(!store->counts[kind]++||score<store->nearest[kind].score)store->nearest[kind]=call.edge;
    return ok;
}
#endif
