#ifndef LITTLE_HELPERS_CORE_H
#define LITTLE_HELPERS_CORE_H
/* Pure bounded simulation. The native consumer owns gravity, collision, actual
 * dust mutations, wallet debits and durable commits. No callback may teleport. */
#ifndef SP_VEC3_DEFINED
#define SP_VEC3_DEFINED
typedef struct {float x,y,z;} SPVec3;
#endif
#define LH_MAX 4u
#define LH_TARGETS 64u
#define LH_PRICE 75u
#define LH_BRUSH_RADIUS .15f
#define LH_BRUSH_STRENGTH .008f
#define LH_STROKE_SECONDS .35f
enum {LH_IDLE,LH_WALK,LH_LOWER,LH_SWEEP,LH_BLOCKED,LH_CHEER};
enum {LH_MOVE_BLOCKED,LH_MOVE_OK,LH_MOVE_PENDING};
enum {LH_BUY=1,LH_UPGRADE=2};
enum {LH_OK,LH_NO_FUNDS,LH_FULL,LH_MAX_CAPACITY,LH_INVALID,LH_DUPLICATE};
typedef unsigned long long LHTxn;
typedef struct {unsigned code,debit;LHTxn txn;unsigned action;} LHResult;
typedef struct {SPVec3 point;float dust;} LHTarget;
typedef struct {
    SPVec3 position,up,heading,target,brush,avoided;
    unsigned active,mode,has_target,has_brush;
    float clock,seek_timer,stroke_timer,blocked_time,avoid_timer;
    unsigned strokes,cells;
} LHWorker;
typedef struct {
    unsigned capacity,owned;LHTxn watermark;LHResult latest;
    LHWorker workers[LH_MAX];unsigned strokes,cells;
} LHCore;
typedef struct {
    void *ctx;
    int (*contact)(void *,unsigned,SPVec3 *,SPVec3 *,SPVec3 *);
    unsigned (*scan)(void *,unsigned,LHTarget *,unsigned);
    int (*move)(void *,unsigned,SPVec3,float,float,SPVec3 *,SPVec3 *,SPVec3 *);
    int (*brush)(void *,unsigned,SPVec3,SPVec3 *);
    float (*dust)(void *,SPVec3);
    int (*clean)(void *,SPVec3,SPVec3,float,float);
} LHWorld;
static inline SPVec3 lh_v(float x,float y,float z){return (SPVec3){x,y,z};}
static inline SPVec3 lh_add(SPVec3 a,SPVec3 b){return lh_v(a.x+b.x,a.y+b.y,a.z+b.z);}
static inline SPVec3 lh_sub(SPVec3 a,SPVec3 b){return lh_v(a.x-b.x,a.y-b.y,a.z-b.z);}
static inline SPVec3 lh_mul(SPVec3 a,float n){return lh_v(a.x*n,a.y*n,a.z*n);}
static inline float lh_dot(SPVec3 a,SPVec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static inline SPVec3 lh_cross(SPVec3 a,SPVec3 b){return lh_v(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static inline int lh_finite(float f){return f==f&&f<1e20f&&f>-1e20f;}
static inline int lh_valid(SPVec3 a){return lh_finite(a.x)&&lh_finite(a.y)&&lh_finite(a.z);}
static inline float lh_distance2(SPVec3 a,SPVec3 b){SPVec3 d=lh_sub(a,b);return lh_dot(d,d);}
static inline SPVec3 lh_steer(const LHCore *c,unsigned slot,SPVec3 forward){
    const LHWorker *p=&c->workers[slot];SPVec3 result=forward,right=lh_cross(p->up,forward);
    for(unsigned i=0;i<c->owned;i++)if(i!=slot&&c->workers[i].active){
        SPVec3 d=lh_sub(c->workers[i].position,p->position);float distance=lh_dot(d,d);if(distance>=.75f*.75f)continue;
        float weight=1-distance/(.75f*.75f);d=lh_sub(d,lh_mul(p->up,lh_dot(d,p->up)));
        result=lh_sub(result,lh_mul(d,weight*.5f/.75f));
        /* Soft toys keep right when face-to-face. Scene collision remains
         * hard; narrow passages may briefly overlap peer plush volumes. */
        if(lh_dot(d,forward)>0&&lh_dot(d,right)<.28f&&lh_dot(d,right)>-.28f)result=lh_add(result,lh_mul(right,.65f*weight));
    }
    return result;
}
static inline void lh_zero(void *p,unsigned n){unsigned char *a=p;for(unsigned i=0;i<n;i++)a[i]=0;}
static inline void lh_init(LHCore *c){lh_zero(c,sizeof(*c));c->capacity=1;}
static inline unsigned lh_upgrade_price(const LHCore *c){static const unsigned p[4]={100,200,350,0};return c->capacity>=1&&c->capacity<=4?p[c->capacity-1]:0;}
static inline LHResult lh_purchase(LHCore *c,unsigned action,LHTxn txn,unsigned balance){
    LHResult r={LH_INVALID,0,txn,action};
    if(!c||!txn||c->capacity<1||c->capacity>LH_MAX||c->owned>c->capacity)return r;
    if(txn==c->watermark&&action==c->latest.action){r=c->latest;r.debit=0;r.code=LH_DUPLICATE;return r;}
    if(txn<=c->watermark)return r;
    unsigned price=action==LH_BUY?LH_PRICE:action==LH_UPGRADE?lh_upgrade_price(c):0;
    if(action==LH_BUY&&c->owned>=c->capacity)r.code=LH_FULL;
    else if(action==LH_UPGRADE&&c->capacity==LH_MAX)r.code=LH_MAX_CAPACITY;
    else if(!price)r.code=LH_INVALID;
    else if(balance<price)r.code=LH_NO_FUNDS;
    else {r.code=LH_OK;r.debit=price;if(action==LH_BUY)c->owned++;else c->capacity++;}
    c->watermark=txn;c->latest=r;return r;
}
static inline int lh_reserved(const LHCore *c,unsigned slot,SPVec3 target){
    for(unsigned i=0;i<c->owned;i++)if(i!=slot&&c->workers[i].active&&c->workers[i].has_target&&lh_distance2(target,c->workers[i].target)<.64f)return 1;
    return 0;
}
static inline int lh_assign(LHCore *c,unsigned slot,const LHWorld *w){
    LHTarget candidates[LH_TARGETS];LHWorker *p=&c->workers[slot];
    unsigned n=w->scan(w->ctx,slot,candidates,LH_TARGETS);if(n>LH_TARGETS)return 0;
    float best=1e20f;int found=0;
    for(unsigned i=0;i<n;i++){
        LHTarget *t=&candidates[i];if(!lh_valid(t->point)||!lh_finite(t->dust)||t->dust<=.002f||lh_reserved(c,slot,t->point))continue;
        if(p->avoid_timer>0&&lh_distance2(t->point,p->avoided)<1.f)continue;
        float d=lh_distance2(p->position,t->point);if(d<best){best=d;p->target=t->point;found=1;}
    }
    p->has_target=(unsigned)found;p->seek_timer=1.f;p->has_brush=0;p->blocked_time=0;return found;
}
static inline void lh_avoid(LHWorker *p){p->avoided=p->target;p->avoid_timer=20;p->has_target=0;p->has_brush=0;p->seek_timer=.5f;p->blocked_time=0;}
static inline int lh_tick(LHCore *c,float dt,const LHWorld *w){
    if(!c||!w||!w->contact||!w->scan||!w->move||!w->brush||!w->dust||!w->clean||!lh_finite(dt)||dt<=0||dt>.1f||c->capacity<1||c->capacity>LH_MAX||c->owned>c->capacity)return 0;
    for(unsigned i=0;i<c->owned;i++){
        LHWorker *p=&c->workers[i];if(!p->active)continue;
        p->clock+=dt;p->seek_timer-=dt;p->avoid_timer-=dt;
        /* Refresh contact even at rest as cleaning removes the supporting dust. */
        if(!w->contact(w->ctx,i,&p->position,&p->up,&p->heading)){p->mode=LH_BLOCKED;p->has_brush=0;continue;}
        if(!p->has_target&&p->seek_timer<=0){lh_assign(c,i,w);p->clock=0;}
        if(!p->has_target){if(p->mode!=LH_CHEER||p->seek_timer<=0)p->mode=LH_IDLE;continue;}
        float dust=w->dust(w->ctx,p->target);
        if(!lh_finite(dust)||dust<=.002f){p->has_target=0;p->has_brush=0;p->mode=LH_CHEER;p->clock=0;p->seek_timer=.8f;continue;}
        SPVec3 delta=lh_sub(p->target,p->position);delta=lh_sub(delta,lh_mul(p->up,lh_dot(delta,p->up)));
        if(lh_dot(delta,delta)>.32f*.32f){
            p->mode=LH_WALK;p->clock=0;p->has_brush=0;
            int moved=w->move(w->ctx,i,p->target,.55f,dt,&p->position,&p->up,&p->heading);
            if(!moved){
                p->mode=LH_BLOCKED;p->blocked_time+=dt;
                if(p->blocked_time>2.f)lh_avoid(p);
            }else {p->blocked_time=0;if(moved==LH_MOVE_PENDING)p->mode=LH_IDLE;}
            continue;
        }
        if(p->mode!=LH_LOWER&&p->mode!=LH_SWEEP){p->mode=LH_LOWER;p->clock=0;p->has_brush=0;p->stroke_timer=0;}
        if(p->mode==LH_LOWER){if(p->clock<.4f)continue;p->mode=LH_SWEEP;p->clock=0;}
        /* A short contact-verified forward push. New distant targets require
         * walking first; a brush can never erase dust along a teleport chord. */
        SPVec3 goal=lh_add(p->position,lh_mul(p->heading,.15f));
        int swept=w->move(w->ctx,i,goal,.32f,dt,&p->position,&p->up,&p->heading);
        if(swept!=LH_MOVE_OK){p->has_brush=0;if(swept==LH_MOVE_PENDING){p->mode=LH_IDLE;continue;}p->mode=LH_BLOCKED;p->blocked_time+=.4f;if(p->blocked_time>2.f)lh_avoid(p);continue;}
        SPVec3 brush;
        if(!w->brush(w->ctx,i,lh_add(p->position,lh_mul(p->heading,.32f)),&brush)){p->has_brush=0;p->blocked_time+=dt;if(p->blocked_time>2.f)lh_avoid(p);continue;}
        p->stroke_timer+=dt;
        if(p->stroke_timer>=LH_STROKE_SECONDS){
            p->stroke_timer=0;
            if(p->has_brush&&lh_distance2(p->brush,brush)<.16f*.16f){
                int cells=w->clean(w->ctx,p->brush,brush,LH_BRUSH_RADIUS,LH_BRUSH_STRENGTH);
                if(cells>0){p->strokes++;p->cells+=(unsigned)cells;c->strokes++;c->cells+=(unsigned)cells;}
            }
            p->brush=brush;p->has_brush=1;
        }
        if(p->clock>1.8f){p->has_target=0;p->has_brush=0;p->seek_timer=.1f;p->mode=LH_IDLE;}
    }
    return 1;
}
#endif
