#ifndef LITTLE_HELPERS_BRUSH_CONTACT_H
#define LITTLE_HELPERS_BRUSH_CONTACT_H
#define LH_BRUSH_GAP_MIN -.05f
#define LH_BRUSH_GAP_MAX .08f
typedef struct {SPVec3 tip,ground;float gap;unsigned measured,admitted;} LHBrushProbe;
static inline int lh_brush_gap_allowed(float gap){return lh_finite(gap)&&gap>=LH_BRUSH_GAP_MIN&&gap<=LH_BRUSH_GAP_MAX;}
static inline int lh_brush_visible(const float *matrix){
    for(unsigned column=0;column<3;column++){
        float square=matrix[column]*matrix[column]+matrix[column+4]*matrix[column+4]+matrix[column+8]*matrix[column+8];
        if(!lh_finite(square)||square<.25f||square>4.f)return 0;
    }
    return 1;
}
/* The authored bristles span local y=0..0.04 and are centered at z=.47.
 * Use their bottom, rather than the previous y=.02 midpoint. */
static inline SPVec3 lh_brush_tip(const float *matrix,float scale){
    return lh_v(matrix[2]*.47f*scale+matrix[3],matrix[6]*.47f*scale+matrix[7],matrix[10]*.47f*scale+matrix[11]);
}
#endif
