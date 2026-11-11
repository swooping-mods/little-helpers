#ifndef LITTLE_HELPERS_DOORS_H
#define LITTLE_HELPERS_DOORS_H
/* Contribute nearby helper presence to the existing controller. Vanilla still
 * owns timing, leaf movement, collision and audio; no collider is disabled. */
#include "door_core.h"
typedef LHDoorBounds LDBounds;
typedef struct {SURoot controller,trigger;LDBounds box;float amount;unsigned observed;} LDDoor;
static LDDoor helper_doors[16];static unsigned helper_door_count,helper_portal[LH_MAX];
static PTR ld_presence,ld_time,ld_bounds,ld_amount;
static void (*ld_set_field)(PTR,PTR,void *);
static int ld_ready;
static SPVec3 ld_nearest(LDBounds b,SPVec3 p){return lhd_nearest(b,p);}
static int ld_near(LDBounds b,SPVec3 p,float radius){return lhd_near(b,p,radius);}
static void ld_drop(void){for(unsigned i=0;i<helper_door_count;i++){su_drop(&helper_doors[i].controller);su_drop(&helper_doors[i].trigger);}helper_door_count=0;lh_zero(helper_portal,sizeof(helper_portal));}
static int ld_bind(PTR game){
    PTR klass=find_class("MoonCleaners","MoonCollectorDoor"),stay=method("MoonCleaners","MoonCollectorDoor","Stay",1),fixed=method("MoonCleaners","MoonCollectorDoor","FixedUpdate",0);
    ld_presence=klass?get_field(klass,"presence"):0;PTR (*field_type)(PTR)=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_field_get_type");
    ld_set_field=(void(*)(PTR,PTR,void *))GetProcAddress(game,"il2cpp_field_set_value");
    if(!stay||!fixed||!ld_presence||!field_type||!ld_set_field||method_token(stay)!=100664281||method_token(fixed)!=100664282||su.field_offset(ld_presence)!=68)return 0;
    if((U64)*(PTR *)stay-(U64)game!=0x681100||(U64)*(PTR *)fixed-(U64)game!=0x680d70)return 0;
    char *type=su.type_name(field_type(ld_presence));int valid=type&&su_eq(type,"System.Single");if(type)su.free(type);if(!valid)return 0;
    ld_time=su_method("UnityEngine","Time","get_time","");ld_bounds=su_method("UnityEngine","Collider","get_bounds","");ld_amount=method("MoonCleaners","MoonCollectorDoor","get_OpenAmount",0);
    return ld_time&&ld_bounds&&ld_amount;
}
static void ld_load(void){
    ld_drop();if(!ld_ready)return;
    int ok=1,inactive=0,sort=0;PTR type=su_type("MoonCleaners","MoonCollectorDoor");void *args[]={type,&inactive,&sort};PTR objects=call(sw.find_objects,0,args,&ok);
    if(!ok||!objects||su.array_length(objects)>16){event("door-discovery-unavailable");return;}
    PTR *items=su.array_address(objects,8,0);unsigned count=(unsigned)su.array_length(objects);
    for(unsigned i=0;i<count;i++){
        PTR controller=items[i];PTR go=call(method("UnityEngine","Component","get_gameObject",0),controller,0,&ok);PTR trigger=go?su_component(go,"BoxCollider"):0;
        if(!trigger||!boolean(su_method("UnityEngine","Collider","get_isTrigger",""),trigger,&ok)||!ok)continue;
        PTR value=call(ld_bounds,trigger,0,&ok);if(!ok||!value)continue;LDBounds box=*(LDBounds *)unbox(value);
        if(!lh_valid(box.center)||!lh_valid(box.extents)||box.extents.x<=0||box.extents.y<=0||box.extents.z<=0)continue;
        helper_doors[helper_door_count++]=(LDDoor){su_hold(controller),su_hold(trigger),box,0,0};
    }
    if(test_tools){char line[128],*p=cat(line,"{\"event\":\"helper-doors\",\"count\":");p=num(p,helper_door_count);cat(p,"}\n");record(line);}
}
static void ld_tick(PTR walker){
    if(!helper_door_count||!lhd_has_workers(&helpers))return;float now=su_number(ld_time,0);
    for(unsigned i=0;i<helper_door_count;i++){
        LDDoor *door=&helper_doors[i];PTR controller=su_get(door->controller),trigger=su_get(door->trigger);if(!su_alive(controller)||!su_alive(trigger))continue;
        int ok=1;PTR value=call(ld_bounds,trigger,0,&ok);if(!ok||!value)continue;door->box=*(LDBounds *)unbox(value);door->amount=su_number(ld_amount,controller);
        unsigned presence=lhd_presence(&helpers,door->box,SP_RADIUS);if(presence){unsigned n=presence-1;
            float prior=0;field_value(controller,ld_presence,&prior);if(prior<now)ld_set_field(controller,ld_presence,&now);
            if(test_tools&&!door->observed&&door->amount<.2f){
                SPVec3 player=su_vector(su.tr_get_position,sw_transform(walker));char line[256],*p=cat(line,"{\"event\":\"door-helper-presence\",\"door\":");p=num(p,i);p=cat(p,",\"worker\":");p=num(p,n);p=cat(p,",\"player_distance\":");p=measure(p,su_sqrt(lh_distance2(player,ld_nearest(door->box,player))));cat(p,"}\n");record(line);door->observed=1;
            }
        }
    }
}
static unsigned ld_route_portal(LHRoute *route,LREdge edge,void *ctx,SPVec3 up){
    SPVec3 point=route->nodes[route->last].point;
    for(unsigned i=0;i<helper_door_count;i++){
        LDDoor *door=&helper_doors[i];if(door->amount>.85f||!ld_near(door->box,point,1.2f))continue;
        SPVec3 near=ld_nearest(door->box,point),delta=lh_sub(near,point);
        SPVec3 desired=lh_add(point,lh_sub(delta,lh_mul(up,lh_dot(delta,up)))),end;
        if(ld_near(door->box,desired,SP_RADIUS)&&edge(ctx,point,desired,&end)&&lr_accept(route,route->last,end)==LR_READY)return i+1;
    }
    return 0;
}
#endif
