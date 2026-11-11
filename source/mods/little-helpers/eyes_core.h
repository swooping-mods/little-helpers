#ifndef LITTLE_HELPERS_EYES_CORE_H
#define LITTLE_HELPERS_EYES_CORE_H
#include "animation_core.h"
static inline void lve_sample(unsigned clip,float seconds,float out[3]){
    for(unsigned k=0;k<3;k++)out[k]=0;
#if LH_MORPH_COUNT
    const unsigned *c=lh_eye_clips[clip];float at=seconds*LH_EYE_FPS;if(at<0)at=0;
    unsigned frame=(unsigned)at;float alpha=at-(float)frame;
    if(lh_clips[clip].loop)frame%=c[1]-1;else if(frame>=c[1]-1){frame=c[1]-1;alpha=0;}
    unsigned next=frame+1<c[1]?frame+1:frame;
    const float *all=(const float *)lh_data_eye_weights,*a=all+(c[0]+frame)*3,*b=all+(c[0]+next)*3;
    for(unsigned k=0;k<3;k++)out[k]=a[k]+(b[k]-a[k])*alpha;
#else
    (void)clip;(void)seconds;
#endif
}
#if LH_MORPH_COUNT
static inline void lve_deform(unsigned vertex,const float weights[3],float position[3],float normal[3]){
    const float *delta=(const float *)lh_data_eye_deltas+vertex*18;
    for(unsigned shape=0;shape<3;shape++)for(unsigned axis=0;axis<3;axis++){
        position[axis]+=weights[shape]*delta[shape*6+axis];
        normal[axis]+=weights[shape]*delta[shape*6+3+axis];
    }
}
#endif
#endif
