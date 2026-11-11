#ifndef LITTLE_HELPERS_INPUT_H
#define LITTLE_HELPERS_INPUT_H
#include "../../runtime/native/interaction.h"
static inline int lh_controls_interact(void){return moon_input_action("little-helpers.interact","E",1).held;}
static inline int lh_controls_hint(char *out,unsigned cap){return moon_interaction_binding_hint(out,cap,"AIM AT BUTTON + ","little-helpers.interact","E","");}
#endif
