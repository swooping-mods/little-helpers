#ifndef LITTLE_HELPERS_TERMINAL_CONTROLS_H
#define LITTLE_HELPERS_TERMINAL_CONTROLS_H
#include "../../runtime/sdk/interactible_gizmos.h"
#include "terminal_model.h"

#define LT_BUTTON_TRAVEL -.024f
typedef struct {MoonGizmoMotion buttons[2];} LTControls;
static inline void lt_controls_init(LTControls *controls){
    for(unsigned i=0;i<2;i++)moon_gizmo_init(&controls->buttons[i]);
}
static inline void lt_controls_press(LTControls *controls,unsigned index){
    if(index<2)moon_gizmo_press(&controls->buttons[index],moon_gizmo_default_timing());
}
static inline void lt_controls_step(LTControls *controls,float dt){
    for(unsigned i=0;i<2;i++)moon_gizmo_step(&controls->buttons[i],moon_gizmo_default_timing(),dt);
}
static inline LTPoint lt_controls_position(const LTControls *controls,unsigned index){
    if(index>=2)return (LTPoint){0,0,0};
    LTPoint rest=lt_buttons[index].center;
    MoonGizmoPoint p=moon_gizmo_position(&controls->buttons[index],
        (MoonGizmoPoint){rest.x,rest.y,rest.z},(MoonGizmoPoint){0,0,LT_BUTTON_TRAVEL});
    return (LTPoint){p.x,p.y,p.z};
}
#endif
