#ifndef LITTLE_HELPERS_DOOR_CORE_H
#define LITTLE_HELPERS_DOOR_CORE_H
#include "helper_core.h"
typedef struct {SPVec3 center,extents;} LHDoorBounds;
static inline SPVec3 lhd_nearest(LHDoorBounds b,SPVec3 p){
    float *v=&p.x,*c=&b.center.x,*e=&b.extents.x;
    for(unsigned i=0;i<3;i++){float low=c[i]-e[i],high=c[i]+e[i];if(v[i]<low)v[i]=low;if(v[i]>high)v[i]=high;}
    return p;
}
static inline int lhd_near(LHDoorBounds b,SPVec3 p,float radius){
    if(!lh_valid(b.center)||!lh_valid(b.extents)||!lh_valid(p)||b.extents.x<0||b.extents.y<0||b.extents.z<0||!lh_finite(radius)||radius<0)return 0;
    return lh_distance2(lhd_nearest(b,p),p)<=radius*radius;
}
static inline int lhd_has_workers(const LHCore *c){
    if(!c||c->owned>LH_MAX)return 0;for(unsigned i=0;i<c->owned;i++)if(c->workers[i].active)return 1;return 0;
}
/* Presence is a local door-controller input in either role. The guest's source
 * is authenticated presentation state, never an independent movement step. */
static inline unsigned lhd_presence(const LHCore *c,LHDoorBounds box,float radius){
    if(!c||c->owned>LH_MAX)return 0;
    for(unsigned i=0;i<c->owned;i++)if(c->workers[i].active&&lhd_near(box,c->workers[i].position,radius))return i+1;return 0;
}
#endif
