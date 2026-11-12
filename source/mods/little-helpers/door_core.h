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
/* The trigger may be deeper than it is wide, so its narrow AABB axis is
 * not the leaf normal. Use the native door frame's forward direction, then
 * cross the full trigger volume. Physics still admits every route segment. */
static inline int lhd_exit_point(LHDoorBounds box,SPVec3 position,SPVec3 up,SPVec3 normal,SPVec3 *out){
    if(!out||!lh_valid(box.center)||!lh_valid(box.extents)||!lh_valid(position)||!lh_valid(up)||!lh_valid(normal)||box.extents.x<=0||box.extents.y<=0||box.extents.z<=0)return 0;
    SPVec3 direction=lh_sub(normal,lh_mul(up,lh_dot(normal,up)));float square=lh_dot(direction,direction);
    if(square<.95f||square>1.05f||lh_distance2(lhd_nearest(box,position),position)>32.f*32.f)return 0;
    float side=lh_dot(lh_sub(position,box.center),direction)>=0?1.f:-1.f;
    SPVec3 center=lh_sub(box.center,lh_mul(up,lh_dot(lh_sub(box.center,position),up)));
    float extent=(direction.x<0?-direction.x:direction.x)*box.extents.x+(direction.y<0?-direction.y:direction.y)*box.extents.y+(direction.z<0?-direction.z:direction.z)*box.extents.z;
    *out=lh_sub(center,lh_mul(direction,side*(extent+1.2f)));return lh_valid(*out);
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
