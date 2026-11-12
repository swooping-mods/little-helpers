#ifndef LH_INTERACTIONS_H
#define LH_INTERACTIONS_H
static unsigned lh_interact;
static const MoonHints *lh_hints;
static int lh_interaction_query(void *context,void *actor,MoonInteractionTarget *target){
    (void)context;PTR walker=actor;
    if(!ready||failed||!moon_interaction_unity_gate(walker,su_get(sw.world),sw.world_field,sw.restoring,sw.blocked,sw.typing,sw.firstperson,focus))return 0;
    DWORD pid=0;window_pid(foreground_window(),&pid);if(pid!=GetCurrentProcessId())return 0;
    MoonInteractionPoint origin,direction;if(!moon_interaction_unity_view(walker,&origin,&direction))return 0;
    int button=target_button(walker),found=0;
    if(button>=0){SPVec3 point=su_vector(su.tr_get_position,su_transform(su_get(buttons[button])));*target=(MoonInteractionTarget){buttons[button],su_sqrt(moon_interaction_distance2(origin,moon_interaction_from_sp(point))),0,(unsigned)button};found=1;}
    for(unsigned i=0;i<helpers.owned&&i<LH_MAX;i++){
        const LHWorker *p=helpers.workers+i;const LHVisual *v=lv_helpers+i;if(!p->active||!v->made||!su_alive(su_get(v->root)))continue;
        SPVec3 center=su_add(su_vector(su.tr_get_position,su_get(v->transform)),su_mul(v->up,.32f));MoonInteractionPoint point=moon_interaction_from_sp(center);
        float distance=su_sqrt(moon_interaction_distance2(origin,point));
        if((found&&distance>=target->distance)||!moon_interaction_aim(moon_interaction_unity_ray,0,origin,direction,2.5f,point,.24f,0))continue;
        *target=(MoonInteractionTarget){v->root,distance,0,i+2};found=1;
    }return found;
}
static int lh_interactions_bind(PTR domain,PTR game){
    lh_hints=lh_hints_connect(domain,game);if(!lh_hints)return 0;
    lh_interact=moon_interaction_register("little-helpers.interact","E",MOON_INTERACTION_EXCLUSIVE,lh_interaction_query,0);return lh_interact!=0;
}
static void lh_interaction_hint(PTR walker,MoonInteractionState state){
    if(!lh_hints)return;
    if(state.selected){char text[128];const char *suffix=state.token>=2?": Greet helper":state.token?": Upgrade helpers":": Buy helper";
        if(moon_interaction_binding_hint(text,sizeof(text),"","little-helpers.interact","E",suffix)){lh_hints->publish("little-helpers",walker,text);return;}}
    lh_hints->clear("little-helpers");
}
#endif
