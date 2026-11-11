#ifndef LITTLE_HELPERS_UNIT_WORLD_H
#define LITTLE_HELPERS_UNIT_WORLD_H
#include "../unit-runtime/contact.h"
static MUCContactCache helper_contact_cache;
static int helper_unit_clear(void *ctx,SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height,SPU64 support){
    if(!sw_clear(ctx,from,to,up,radius,height,support)){
#ifdef LH_UNIT_CLEAR_OBSERVER
        LH_UNIT_CLEAR_OBSERVER(1,from,to,up,radius,height);
#endif
        return 0;
    }
    int clear=muc_capsule_clear(&helper_contact_cache,helper_units,helper_contacts,2,
        (MoonUnitVector){from.x,from.y,from.z},(MoonUnitVector){to.x,to.y,to.z},(MoonUnitVector){up.x,up.y,up.z},radius,height);
#ifdef LH_UNIT_CLEAR_OBSERVER
    LH_UNIT_CLEAR_OBSERVER(clear?0:helper_contact_cache.valid?2:3,from,to,up,radius,height);
#endif
    return clear;
}
/* Each contact, route or spawn callback is a synchronous continuation; foreign
 * providers cannot mutate their markers during it. */
static SPWorld helper_unit_world(void){muc_invalidate(&helper_contact_cache);SPWorld w=sw_callbacks();w.clear=helper_unit_clear;return w;}
/* Ground contact must run before soft avoidance can move an overlapping
 * worker away. Foreign advisory markers cannot revoke that physical contact. */
static SPWorld helper_contact_world(void){return sw_callbacks();}
#endif
