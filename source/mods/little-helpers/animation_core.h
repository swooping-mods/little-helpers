#ifndef LITTLE_HELPERS_ANIMATION_CORE_H
#define LITTLE_HELPERS_ANIMATION_CORE_H
#ifndef LH_DATA_HEADER
#define LH_DATA_HEADER "assets/generated/plush_data.h"
#endif
#include LH_DATA_HEADER
#ifndef LH_MORPH_COUNT
#define LH_MORPH_COUNT 0
#endif
#ifndef LH_MATERIAL_COUNT
#define LH_MATERIAL_COUNT 4u
#endif
enum {LHV_IDLE,LHV_WALK,LHV_RUN,LHV_WAVE,LHV_BRUSH_SWEEP,LHV_BRUSH_RUN,LHV_HAPPY,LHV_TURN_LEFT,LHV_TURN_RIGHT,LHV_BRUSH_READY};
static inline float lva_sqrt(float x){
    if(x<=0)return 0;
    union {float f;unsigned u;} n={x};n.u=(n.u>>1)+0x1fc00000u;
    for(unsigned i=0;i<4;i++)n.f=.5f*(n.f+x/n.f);
    return n.f;
}
static inline float lva_ease(float t){if(t<0)return 0;if(t>1)return 1;return t*t*t*(10+t*(-15+6*t));}
static inline void lva_quat(const float *a,const float *b,float t,float *out){
    float dot=0,norm=0;for(unsigned k=0;k<4;k++)dot+=a[k]*b[k];float sign=dot<0?-1.f:1.f;
    for(unsigned k=0;k<4;k++){out[k]=a[k]+(b[k]*sign-a[k])*t;norm+=out[k]*out[k];}
    float inverse=1/lva_sqrt(norm);for(unsigned k=0;k<4;k++)out[k]*=inverse;
}
static inline void lva_sample(unsigned clip,float seconds,LHPose *out){
    const LHClip *c=&lh_clips[clip];float at=seconds*30;if(at<0)at=0;unsigned frame=(unsigned)at;float alpha=at-(float)frame;
    if(c->loop)frame%=c->frames-1;else if(frame>=c->frames-1){frame=c->frames-1;alpha=0;}
    unsigned next=frame+1<c->frames?frame+1:frame;const LHPose *all=(const LHPose *)lh_data_poses,*a=all+(c->offset+frame)*LH_NODE_COUNT,*b=all+(c->offset+next)*LH_NODE_COUNT;
    for(unsigned i=0;i<LH_NODE_COUNT;i++){
        lva_quat(a[i].q,b[i].q,alpha,out[i].q);
        for(unsigned k=0;k<3;k++){out[i].t[k]=a[i].t[k]+(b[i].t[k]-a[i].t[k])*alpha;out[i].s[k]=a[i].s[k]+(b[i].s[k]-a[i].s[k])*alpha;}
    }
}
static inline void lva_blend(const LHPose *from,const LHPose *to,float alpha,LHPose *out){
    alpha=lva_ease(alpha);
    for(unsigned i=0;i<LH_NODE_COUNT;i++){
        lva_quat(from[i].q,to[i].q,alpha,out[i].q);
        for(unsigned k=0;k<3;k++){out[i].t[k]=from[i].t[k]+(to[i].t[k]-from[i].t[k])*alpha;out[i].s[k]=from[i].s[k]+(to[i].s[k]-from[i].s[k])*alpha;}
    }
    /* Visibility is discrete: a hidden brush must not grow through the suit. */
    if(from[LH_BRUSH_NODE].s[0]<.01f||to[LH_BRUSH_NODE].s[0]<.01f)
        for(unsigned k=0;k<3;k++)out[LH_BRUSH_NODE].s[k]=(alpha<.75f?from:to)[LH_BRUSH_NODE].s[k];
}
static inline void lva_multiply(const float *a,const float *b,float *out){
    for(unsigned row=0;row<3;row++){
        for(unsigned col=0;col<3;col++)out[row*4+col]=a[row*4]*b[col]+a[row*4+1]*b[4+col]+a[row*4+2]*b[8+col];
        out[row*4+3]=a[row*4]*b[3]+a[row*4+1]*b[7]+a[row*4+2]*b[11]+a[row*4+3];
    }
}
/* Keep FK's bounded workspace separate from the caller's pose sample on the
 * freestanding Win64 stack, which has no CRT stack-probe helper. */
static __attribute__((noinline)) void lva_palette(const LHPose *pose,float *palette){
    float world[LH_NODE_COUNT][12];
    for(unsigned i=0;i<LH_NODE_COUNT;i++){
        const LHPose *p=&pose[i];float x=p->q[0],y=p->q[1],z=p->q[2],w=p->q[3];
        float local[12]={(1-2*(y*y+z*z))*p->s[0],2*(x*y-z*w)*p->s[1],2*(x*z+y*w)*p->s[2],p->t[0],2*(x*y+z*w)*p->s[0],(1-2*(x*x+z*z))*p->s[1],2*(y*z-x*w)*p->s[2],p->t[1],2*(x*z-y*w)*p->s[0],2*(y*z+x*w)*p->s[1],(1-2*(x*x+y*y))*p->s[2],p->t[2]};
        if(lh_node_parents[i]>=0)lva_multiply(world[lh_node_parents[i]],local,world[i]);else for(unsigned k=0;k<12;k++)world[i][k]=local[k];
    }
    const float *inverse=(const float *)lh_data_inverse_bind;
    for(unsigned i=0;i<LH_BONE_COUNT-1;i++)lva_multiply(world[lh_skin_nodes[i]],inverse+i*12,palette+i*12);
    for(unsigned k=0;k<12;k++)palette[(LH_BONE_COUNT-1)*12+k]=world[LH_BRUSH_NODE][k];
}
#endif
