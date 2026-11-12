#ifndef LITTLE_HELPERS_SPAWN_CORE_H
#define LITTLE_HELPERS_SPAWN_CORE_H
/* Try directly in view first. Side alternatives are clear-space fallbacks,
 * never the purchase slot's default shelf or formation position. */
static inline SPVec3 lh_spawn_candidate(SPVec3 origin,SPVec3 right,SPVec3 forward,unsigned slot,unsigned attempt){
    static const float sides[5]={0,.75f,-.75f,1.5f,-1.5f};
    unsigned side=attempt%5;
    if(side&&(slot&1))side=side%2?side+1:side-1;
    return lh_add(origin,lh_add(lh_mul(right,sides[side]),lh_mul(forward,1.1f+(float)(attempt/5)*.65f)));
}
static inline void lh_activate(LHWorker *h,SPVec3 point,SPVec3 up,SPVec3 facing,unsigned slot){
    lh_zero(h,sizeof(*h));h->active=1;h->position=point;h->up=up;h->heading=facing;
    h->mode=LH_CHEER;h->seek_timer=2.4f+(float)slot*.12f;
}
#endif
