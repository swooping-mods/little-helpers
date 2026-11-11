/* Host authority with authenticated guest commands. Every Unity call runs in the pinned
 * MoonWalker.Update main-thread boundary; ownership never enters vanilla saves. */
#include "../../runtime/native/gameplay.h"
#include "../unit-runtime/client.h"
#include "adapters/unity.h"
#define SP_IMPLEMENTATION
#include "adapters/pet_core.h"
static int test_tools,helper_route_probe,helper_physics_ids;
static void helper_physics_observe(unsigned,SPVec3,SPVec3,SPVec3,float,float,PTR,int);
#define SW_BLOCK_OBSERVER(S,F,T,U,R,H,A,C) do{if(test_tools&&helper_route_probe)helper_physics_observe(S,F,T,U,R,H,A,C);}while(0)
#include "adapters/world.h"
#undef SW_BLOCK_OBSERVER
#include "../../runtime/native/interaction_unity.h"
#include "input.h"
#include "../../runtime/native/interaction_client.h"
#define mh_connect lh_hints_connect
#include "../hints-framework/sdk/client.h"
#undef mh_connect
#include "adapters/pet_store.h"
#include "helper_core.h"
#include "unit_replica.h"
#include "unit_commands.h"
#include "contact_adapter.h"
#include "route_core.h"
#include "route_probe.h"
#include "visual.h"
#include "sound.h"
int _fltused=0;
void *memcpy(void *target,const void *source,U64 size){su_copy(target,source,size);return target;}
static LHCore helpers;static SPCore navigation;static char save_root[1024];
static const MoonUnits *helper_units;
static const MoonUnitContacts *helper_contacts;
static LHProbeDetail helper_clear_detail;
static LHProbePhysics helper_physics_detail;
static SURoot helper_overlap_array;
static PTR helper_overlap_method,helper_collider_name,helper_collider_bounds;
static unsigned helper_physics_bound;
static int helper_hit_stride;
static void helper_physics_observe(unsigned stage,SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height,PTR hits,int count){
    lhp_zero(&helper_physics_detail,sizeof(helper_physics_detail));helper_physics_detail.stage=stage;
    helper_physics_detail.from=from;helper_physics_detail.to=to;helper_physics_detail.up=up;helper_physics_detail.radius=radius;helper_physics_detail.height=height;
    helper_physics_detail.identity_enabled=(unsigned)helper_physics_ids;
    if(count>0){helper_physics_detail.count=(unsigned)count;if(hits)helper_physics_detail.saturated=(U64)count>=su.array_length(hits);}
    if(stage==3||!helper_physics_ids)return;
    if(!helper_physics_bound){
        helper_physics_bound=1;
        helper_overlap_method=su_method("UnityEngine","Physics","OverlapCapsuleNonAlloc","UnityEngine.Vector3;UnityEngine.Vector3;System.Single;UnityEngine.Collider[];System.Int32;UnityEngine.QueryTriggerInteraction");
        helper_collider_name=su_method("UnityEngine","Object","get_name","");
        helper_collider_bounds=su_method("UnityEngine","Collider","get_bounds","");
        helper_overlap_array=su_hold(su.new_array(find_class("UnityEngine","Collider"),8));
        PTR game=GetModuleHandleA("GameAssembly.dll");int (*size)(PTR,DWORD *)=game?(int(*)(PTR,DWORD *))GetProcAddress(game,"il2cpp_class_value_size"):0;
        DWORD alignment=0;helper_hit_stride=size?size(find_class("UnityEngine","RaycastHit"),&alignment):0;
        if(helper_hit_stride<su.hit_distance+4||helper_hit_stride>96)helper_hit_stride=0;
    }
    int ok=1;
    if(stage==1){
        if(!helper_overlap_method||!helper_overlap_array)return;
        SPVec3 a=su_add(to,su_mul(up,height*.5f-radius)),b=su_sub(to,su_mul(up,height*.5f-radius));
        float r=radius-.012f;int mask=~(1<<2),triggers=1;hits=su_get(helper_overlap_array);
        void *args[]={&a,&b,&r,hits,&mask,&triggers};PTR result=call(helper_overlap_method,0,args,&ok);
        if(!ok||!result)return;count=*(int *)unbox(result);
    }
    if(!hits||count<0)return;
    U64 length=su.array_length(hits);helper_physics_detail.count=(unsigned)count;
    helper_physics_detail.saturated=(U64)count>=length;helper_physics_detail.query_ok=1;
    unsigned n=(unsigned)count;if((U64)n>length)n=(unsigned)length;if(n>3)n=3;
    if(stage==2&&!helper_hit_stride){helper_physics_detail.query_ok=0;return;}
    for(unsigned i=0;i<n;i++){
        LHProbeCollider *out=&helper_physics_detail.colliders[helper_physics_detail.observed];PTR collider=0;
        if(stage==1){PTR *items=su.array_address(hits,8,0);collider=items?items[i]:0;}
        else {PTR hit=su.array_address(hits,helper_hit_stride,i);collider=hit?call(su.hit_collider,hit,0,&ok):0;
            if(hit){su_copy(&out->point,(unsigned char *)hit+su.hit_point,sizeof(SPVec3));su_copy(&out->normal,(unsigned char *)hit+su.hit_normal,sizeof(SPVec3));}}
        if(!ok||!collider)continue;out->id=(unsigned)sw_object_id(collider);
        PTR value=helper_collider_name?call(helper_collider_name,collider,0,&ok):0;if(ok&&value)su_string(value,out->name,sizeof(out->name));
        value=helper_collider_bounds?call(helper_collider_bounds,collider,0,&ok):0;
        if(ok&&value){SPVec3 *bounds=unbox(value);out->center=bounds[0];out->extents=bounds[1];}
        helper_physics_detail.observed++;
    }
}
static void helper_observe_clear(unsigned,SPVec3,SPVec3,SPVec3,float,float);
#define LH_UNIT_CLEAR_OBSERVER(K,F,T,U,R,H) do{if(test_tools&&helper_route_probe)helper_observe_clear(K,F,T,U,R,H);}while(0)
#include "unit_world.h"
#undef LH_UNIT_CLEAR_OBSERVER
static void helper_observe_clear(unsigned reason,SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height){
    lhp_zero(&helper_clear_detail,sizeof(helper_clear_detail));helper_clear_detail.reason=reason;
    if(reason==1)helper_clear_detail.physics=helper_physics_detail;
    if(reason!=2)return;
    for(unsigned i=0;i<helper_contact_cache.count;i++){
        const MoonUnitNearby *p=&helper_contact_cache.nearby[i];
        if(p->provider==1||p->provider==2)continue;
        if(muc_marker_clear((MoonUnitVector){from.x,from.y,from.z},(MoonUnitVector){to.x,to.y,to.z},(MoonUnitVector){up.x,up.y,up.z},radius,height,p))continue;
        helper_clear_detail.provider=p->provider;helper_clear_detail.id=p->id;
        helper_clear_detail.position=(SPVec3){p->position.x,p->position.y,p->position.z};helper_clear_detail.radius=p->radius;break;
    }
}
static LHReplica helper_replica;
static PTR helper_frame,helper_real_delta;
static U64 helper_sim_tick;
static int helper_registry_failed;
static LHRoute routes[LH_MAX];
static LHRouteProbe helper_route_probes[LH_MAX];
typedef struct {SPVec3 point;float before,age;unsigned cells,active;} LHDustProbe;
static LHDustProbe dust_probes[32];
static PTR helper_log,focus,clean_stroke,local_id,host_id,joined;
static int ticks,thread_id,world_frames,ready,failed,dirty,store_failed;
static unsigned terminal_upgrade_quote;
static char terminal_hint[96];
static float live_dt,save_timer;
static short (*key_state)(int);static PTR (*foreground_window)(void);static DWORD (*window_pid)(PTR,DWORD *);
static SURoot terminal,buttons[2],labels[5],terminal_button_ink;
static void event(const char *name){char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\",\"mod\":\"little-helpers\",\"ticks\":");p=num(p,(U64)ticks);p=cat(p,"}\n");DWORD n=0;WriteFile(helper_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(helper_log);}
static char *measure(char *p,float value){if(!lh_finite(value))return cat(p,"null");if(value<0){*p++='-';value=-value;}U64 n=(U64)(value*1000000.f+.5f);p=num(p,n/1000000);*p++='.';U64 fraction=n%1000000;for(U64 divisor=100000;divisor;divisor/=10)*p++=(char)('0'+fraction/divisor%10);*p=0;return p;}
static void record(const char *b){DWORD n=0,size=0;while(b[size])size++;WriteFile(helper_log,b,size,&n,0);FlushFileBuffers(helper_log);}
#include "doors.h"
static int authority(void){
    int ok=1;if(!boolean(joined,0,&ok))return ok;PTR mine=call(local_id,0,0,&ok),host=call(host_id,0,0,&ok);
    return ok&&mine&&host&&*(U64 *)unbox(mine)!=0&&*(U64 *)unbox(mine)==*(U64 *)unbox(host);
}
static unsigned helper_snapshot(unsigned char *out,unsigned capacity){return ready&&helper_units->host()?lhr_write(&helpers,out,capacity):0;}
static int helper_receive(const unsigned char *bytes,unsigned length){return !helper_units->host()&&lhr_receive(&helper_replica,bytes,length);}
static void helper_replica_reset(void){lh_zero(&helper_replica,sizeof(helper_replica));}
static unsigned checksum(const unsigned char *p,unsigned n){unsigned h=2166136261u;for(unsigned i=0;i<n;i++){h^=p[i];h*=16777619u;}return h;}
typedef struct {unsigned magic,version,capacity,owned;U64 watermark;unsigned action,code,check;} LHSaved;
static void save_helpers(void){
    if(!helper_units->host()||!dirty||store_failed||!save_root[0])return;LHSaved s;su_zero(&s,sizeof(s));s.magic=0x50484c4d;s.version=1;s.capacity=helpers.capacity;s.owned=helpers.owned;s.watermark=helpers.watermark;s.action=helpers.latest.action;s.code=helpers.latest.code;
    s.check=checksum((const unsigned char *)&s,__builtin_offsetof(LHSaved,check));
    if(ss_save(save_root,(const unsigned char *)&s,sizeof(s))){dirty=0;event("state-saved");}else event("state-save-retry");
}
static int load_helpers(void){
    lh_init(&helpers);LHSaved s;unsigned n=0;
    if(!ss_load(save_root,(unsigned char *)&s,sizeof(s),&n))return ss.last_error==SS_NOT_FOUND;
    if(n!=sizeof(s)||s.magic!=0x50484c4d||s.version!=1||s.capacity<1||s.capacity>4||s.owned>s.capacity||s.check!=checksum((const unsigned char *)&s,__builtin_offsetof(LHSaved,check)))return 0;
    helpers.capacity=s.capacity;helpers.owned=s.owned;helpers.watermark=s.watermark;helpers.latest=(LHResult){s.code,0,s.watermark,s.action};return 1;
}
static int support_used(SPU64 id){for(unsigned i=0;i<LH_MAX;i++)if(navigation.players[i].active&&navigation.players[i].support_id==id)return 1;return 0;}
static int spawn_point(PTR walker,unsigned slot,SPVec3 *point,SPVec3 *up,SPVec3 *heading){
    PTR tr=sw_transform(walker);SPVec3 origin=su_vector(su.tr_get_position,tr),right=su_vector(su.tr_right,tr);*heading=su_vector(su.tr_forward,tr);
    SPWorld w=helper_unit_world();SPGround ground;
    for(unsigned attempt=0;attempt<12;attempt++){
        float side=(float)((slot+attempt)%4)-1.5f;SPVec3 query=su_add(origin,su_add(su_mul(right,side*.85f),su_mul(*heading,1.5f+(float)(attempt/4)*.8f)));
        if(sp_grounded(&w,query,.6f,3.f,point,up,&ground)&&w.clear(w.ctx,*point,*point,*up,SP_RADIUS,SP_HEIGHT,ground.support_id)&&!sp_crowded(&navigation,0,*point))return 1;
    }return 0;
}
static void activate(unsigned slot,SPVec3 point,SPVec3 up,SPVec3 heading){
    LHWorker *h=&helpers.workers[slot];su_zero(h,sizeof(*h));h->active=1;h->position=point;h->up=up;h->heading=sp_tangent(heading,up);h->mode=LH_CHEER;h->seek_timer=2.4f+(float)slot*.12f;
    SPPet *p=&navigation.players[slot];su_zero(p,sizeof(*p));p->used=p->active=p->owned=1;p->position=point;p->up=up;p->heading=h->heading;
}
static int contact(void *ctx,unsigned slot,SPVec3 *point,SPVec3 *up,SPVec3 *heading){
    (void)ctx;SPPet *p=&navigation.players[slot];SPWorld w=helper_contact_world();int grounded=lhc_contact(&navigation,slot,live_dt,&w);
    *point=p->position;*up=p->up;*heading=p->heading;return grounded;
}
static unsigned scan(void *ctx,unsigned slot,LHTarget *out,unsigned maximum){
    (void)ctx;LHWorker *p=&helpers.workers[slot];SPVec3 right=su_normal(su_cross(p->up,p->heading));unsigned n=0;
    static const float radii[]={.35f,1.f,2.f,4.f,7.f,11.f,16.f,22.f};
    static const float directions[8][2]={{0,1},{.7071f,.7071f},{1,0},{.7071f,-.7071f},{0,-1},{-.7071f,-.7071f},{-1,0},{-.7071f,.7071f}};
    for(unsigned ring=0;ring<8&&n<maximum;ring++)for(unsigned j=0;j<8&&n<maximum;j++){
        SPVec3 q=su_add(p->position,su_mul(su_add(su_mul(right,directions[j][0]),su_mul(p->heading,directions[j][1])),radii[ring]));SPVec3 up;SPGround g;
        if(!sw_up(&sw,q,&up)||!sw_ground(&sw,su_add(q,su_mul(up,1)),up,8,&g)||g.support_id||su_dot(g.normal,up)<SP_SLOPE_COS)continue;
        int ok=1;void *a[]={&g.point};PTR value=call(sw.dust_height,su_get(sw.world),a,&ok);float dust=ok&&value?*(float *)unbox(value):0;
        if(dust>.002f)out[n++]=(LHTarget){g.point,dust};
    }
    return n;
}
typedef struct {SPPet *pet;SPWorld world;} LREdgeContext;
static float dust(void *ctx,SPVec3 point);
static int route_edge(void *ctx,SPVec3 from,SPVec3 desired,SPVec3 *end){
    LREdgeContext *c=ctx;SPVec3 up;SPGround g;
    if(test_tools&&helper_route_probe){unsigned slot=(unsigned)(c->pet-navigation.players);return lhp_edge(&navigation,c->pet,&c->world,from,desired,end,&up,&g,&helper_route_probes[slot],lr_h(&routes[slot],from),&helper_clear_detail);}
    return sp_edge(&navigation,c->pet,&c->world,from,desired,end,&up,&g,0);
}
static int move(void *ctx,unsigned slot,SPVec3 goal,float speed,float dt,SPVec3 *point,SPVec3 *up,SPVec3 *heading){
    (void)ctx;SPWorld w=helper_unit_world();SPPet *p=&navigation.players[slot];SPVec3 end,new_up;SPGround g;
    SPVec3 requested=goal;goal=su_sub(goal,su_mul(p->up,su_dot(su_sub(goal,p->position),p->up)));
    if(!sp_edge(&navigation,p,&w,p->position,goal,&end,&new_up,&g,0)){
        LHRoute *route=&routes[slot];
        if(!route->state||lh_distance2(route->goal,requested)>.25f){lr_begin(route,p->position,su_normal(su_cross(p->up,p->heading)),p->heading,requested);if(test_tools&&helper_route_probe)lhp_zero(&helper_route_probes[slot],sizeof(helper_route_probes[slot]));helper_portal[slot]=0;}
        if(route->state==LR_SEARCHING){
            /* One worker gets one expansion per logical tick. Four pets cannot turn
             * a blocked room exit into four unbounded synchronous searches. */
            if(helper_sim_tick%helpers.owned==slot){
                LREdgeContext edge={p,w};lr_step(route,route_edge,&edge,1);
                if(route->state==LR_SEARCHING){
                    /* Radial scans may nominate dirt behind a station. Prefer
                     * real dirty terrain encountered on a verified corridor. */
                    SPVec3 reached=route->nodes[route->last].point;SPGround ground;
                    if(sw_ground(&sw,su_add(reached,su_mul(p->up,.1f)),p->up,SP_HALF_HEIGHT+.2f,&ground)&&!ground.support_id&&dust(0,ground.point)>.002f&&!lh_reserved(&helpers,slot,ground.point)){
                        helpers.workers[slot].target=ground.point;route->goal=ground.point;lr_accept(route,route->last,reached);helper_portal[slot]=0;
                        if(test_tools)event("reachable-dirt-job");
                    }else helper_portal[slot]=ld_route_portal(route,route_edge,&edge,p->up);
                }
            }
            if(route->state==LR_SEARCHING)return LH_MOVE_PENDING;
        }
        if(route->state!=LR_READY)return LH_MOVE_BLOCKED;
        goal=lr_waypoint(route,p->position);
        if(helper_portal[slot]&&lh_distance2(goal,p->position)<.18f*.18f&&route->index==route->count){
            if(helper_doors[helper_portal[slot]-1].amount>.85f){route->state=0;helper_portal[slot]=0;}
            return LH_MOVE_PENDING;
        }
    }else {routes[slot].state=0;helper_portal[slot]=0;}
    SPVec3 delta=sp_tangent(sp_sub(goal,p->position),p->up);float distance=sp_length(sp_sub(goal,p->position));if(distance>speed*dt)distance=speed*dt;
    if(distance<.0001f)return 1;SPVec3 steered=su_normal(lh_steer(&helpers,slot,delta)),desired=su_add(p->position,su_mul(steered,distance));
    if(!sp_edge(&navigation,p,&w,p->position,desired,&end,&new_up,&g,0)){
        desired=su_add(p->position,su_mul(delta,distance));
        if(!sp_edge(&navigation,p,&w,p->position,desired,&end,&new_up,&g,0)){routes[slot].state=0;return 0;}steered=delta;
    }
    p->position=end;p->up=new_up;p->heading=sp_tangent(steered,new_up);sp_contact(p,&w,&g);*point=p->position;*up=p->up;*heading=p->heading;return 1;
}
static int brush(void *ctx,unsigned slot,SPVec3 query,SPVec3 *point){
    (void)ctx;SPGround g;LHWorker *p=&helpers.workers[slot];SPVec3 up=p->up;
    /* Probe the visible bristle head from the previous rendered pose. This
     * keeps the contact tied to the same prop, including its short sweep. */
    LHVisual *v=&lv_helpers[slot];if(!v->made)return 0;
    const float *m=v->last+(LH_BONE_COUNT-1)*12;const float s=LH_SOURCE_SCALE;
    SPVec3 local=su_v(m[1]*.02f*s+m[2]*.47f*s+m[3],m[5]*.02f*s+m[6]*.47f*s+m[7],m[9]*.02f*s+m[10]*.47f*s+m[11]);
    up=v->up;SPVec3 right=su_normal(su_cross(up,v->heading));query=su_add(su_sub(v->position,su_mul(up,SP_HALF_HEIGHT)),su_add(su_mul(right,local.x),su_add(su_mul(up,local.y),su_mul(v->heading,local.z))));
    if(!sw_ground(&sw,su_add(query,su_mul(up,.25f)),up,1,&g)||g.support_id||su_dot(g.normal,up)<.9f)return 0;*point=g.point;return 1;
}
static float dust(void *ctx,SPVec3 point){(void)ctx;int ok=1;void *a[]={&point};PTR value=call(sw.dust_height,su_get(sw.world),a,&ok);return ok&&value?*(float *)unbox(value):0;}
static int clean(void *ctx,SPVec3 a,SPVec3 b,float radius,float strength){
    if(!helper_units->host())return 0;float before=test_tools?dust(ctx,b):0;int ok=1;float width=2*radius;void *args[]={&a,&b,&width,&strength};PTR result=call(clean_stroke,su_get(sw.world),args,&ok);
    int cells=ok&&result?*(int *)unbox(result):0;
    if(test_tools){
        char line[512],*p=cat(line,"{\"event\":\"brush-observation\",\"cells\":");p=num(p,cells>0?(U64)cells:0);p=cat(p,",\"call_ok\":");p=num(p,(U64)ok);p=cat(p,",\"dust_before\":");p=measure(p,before);p=cat(p,",\"dust_after\":");p=measure(p,dust(ctx,b));p=cat(p,",\"radius\":");p=measure(p,radius);p=cat(p,",\"strength\":");p=measure(p,strength);cat(p,"}\n");record(line);
        if(ok&&cells>0)for(unsigned i=0;i<32;i++)if(!dust_probes[i].active){dust_probes[i]=(LHDustProbe){b,before,0,(unsigned)cells,1};break;}
    }
    return cells;
}
static void observe_dust(float dt){
    if(!test_tools)return;
    for(unsigned i=0;i<32;i++)if(dust_probes[i].active){
        LHDustProbe *q=&dust_probes[i];q->age+=dt;if(q->age<.75f)continue;
        char line[512],*p=cat(line,"{\"event\":\"dust-change\",\"cells\":");p=num(p,q->cells);p=cat(p,",\"dust_before\":");p=measure(p,q->before);p=cat(p,",\"dust_after\":");p=measure(p,dust(0,q->point));p=cat(p,",\"delay\":");p=measure(p,q->age);p=cat(p,",\"point\":[");p=measure(p,q->point.x);*p++=',';p=measure(p,q->point.y);*p++=',';p=measure(p,q->point.z);cat(p,"]}\n");record(line);q->active=0;
    }
}
static SGText glyphs;
static void text(PTR go,const char *message){
    float height=go==su_get(labels[3])?.08f:go==su_get(labels[4])?.045f:.055f;
    if(!sg_build(message,height,&glyphs))return;for(unsigned i=0;i<glyphs.vertex_count;i++)glyphs.positions[i].x=glyphs.width*.5f-glyphs.positions[i].x;
    for(unsigned i=0;i<glyphs.index_count;i+=3){int n=glyphs.indices[i+1];glyphs.indices[i+1]=glyphs.indices[i+2];glyphs.indices[i+2]=n;}
    int ok=1;PTR mesh=su_new("UnityEngine","Mesh");call(su.mesh_ctor,mesh,0,&ok);
    PTR vs=su_array(su.vector_class,glyphs.vertex_count,glyphs.positions,12),ns=su_array(su.vector_class,glyphs.vertex_count,glyphs.normals,12),is=su_array(su.int_class,glyphs.index_count,glyphs.indices,4);
    su_int(su.mesh_subcount,mesh,1);
    void *va[]={vs},*na[]={ns};call(su.mesh_vertices,mesh,va,&ok);call(su.mesh_normals,mesh,na,&ok);int index=0;unsigned char bounds=1;void *ia[]={is,&index,&bounds};call(su.mesh_indices,mesh,ia,&ok);
    PTR filter=su_component(go,"MeshFilter");if(!filter)filter=su_add_component(go,"MeshFilter");PTR renderer=su_component(go,"MeshRenderer");if(!renderer)renderer=su_add_component(go,"MeshRenderer");
    PTR old=call(su_method("UnityEngine","MeshFilter","get_sharedMesh",""),filter,0,&ok);void *ma[]={mesh};call(su.meshfilter_set,filter,ma,&ok);if(old)su_destroy(old);SURoot ink=terminal_button_ink&&(go==su_get(labels[0])||go==su_get(labels[1]))?terminal_button_ink:lv_label_material;lv_assign(renderer,&ink,1);call(su.mesh_bounds,mesh,0,&ok);if(!ok)event("label-mesh-failed");
}
static void terminal_refresh_hint(void){
    if(!labels[4])return;char hint[96];if(!lh_controls_hint(hint,sizeof(hint)))cat(hint,"INPUT UNAVAILABLE");
    if(!su_eq(terminal_hint,hint)){cat(terminal_hint,hint);text(su_get(labels[4]),hint);}
}
static void terminal_show(void *context,const char *status){(void)context;if(labels[2])text(su_get(labels[2]),status);}
static void terminal_status(const char *status){moon_interaction_feedback((MoonInteractionFeedback){0,terminal_show},status);}
static void update_prices_capacity(unsigned capacity){
    if(!labels[1])return;LHCore shown={0};shown.capacity=capacity;char message[64],*p=cat(message,"UPGRADE $");unsigned price=lh_upgrade_price(&shown);terminal_upgrade_quote=price;
    if(price){num(p,price);text(su_get(labels[1]),message);}else text(su_get(labels[1]),"CAPACITY MAX 4");
}
static void update_prices(void){update_prices_capacity(helpers.capacity);}
#include "terminal.h"
static int target_button(PTR walker){
    unsigned long long objects[2]={sw_object_id(su_get(buttons[0])),sw_object_id(su_get(buttons[1]))};
    return moon_interaction_unity_target(walker,3.5f,objects,2);
}
#include "interaction.h"
#include "unit_purchase.h"
static void purchase_feedback(unsigned code,unsigned owned,unsigned capacity){
    if(code!=LH_OK){const char *messages[]={"","NEED FUNDS","UPGRADE CAPACITY","MAX FOUR HELPERS","INVALID REQUEST","ALREADY APPLIED","HOST NOT READY","NEED CLEAR SPACE","PAYMENT FAILED","MOVE CLOSER","PRICE CHANGED / TRY AGAIN"};terminal_status(messages[code<=LHC_PRICE_CHANGED?code:LH_INVALID]);return;}
    char message[80],*p=cat(message,"HELPERS ");p=num(p,owned);p=cat(p," / ");num(p,capacity);terminal_status(message);
}
static void purchase(PTR walker,PTR world,unsigned action){
    unsigned quote=action==LH_BUY?LH_PRICE:terminal_upgrade_quote;
    if(!helper_units->host()){
        unsigned char bytes[12];unsigned n=lhc_request(bytes,action,quote);
        terminal_status(n&&helper_units->command(2,bytes,n)?"WAITING FOR HOST":"HOST NOT READY / WAIT");return;
    }
    int ok=1;PTR player=call(method("MoonCleaners","MoonCrew","get_LocalPlayerId",0),0,0,&ok);SURoot root=su_hold(ok?player:0);
    unsigned code=purchase_for(walker,world,su_get(root),action,quote);su_drop(&root);if(code==LHC_PRICE_CHANGED)update_prices();purchase_feedback(code,helpers.owned,helpers.capacity);
}
static unsigned helper_command(U64 peer,const unsigned char *bytes,unsigned n,unsigned char *out,unsigned capacity){
    unsigned action=lhc_action(bytes,n);if(!action||capacity<16)return 0;
    if(ready&&helper_units->host()){unsigned quote=lhc_request_quote(&helpers,bytes,n);if(quote!=LH_OK)return lhc_response(out,quote,&helpers);}
    unsigned code=LHC_NOT_READY;PTR actor=helper_units->peer_actor(peer);SURoot root=su_hold(actor);
    if(ready&&helper_units->host()&&root&&terminal){
        SPVec3 p=su_vector(su.tr_get_position,sw_transform(su_get(root))),at=su_vector(su.tr_get_position,sw_transform(su_get(terminal)));
        code=LHC_TOO_FAR;
        if(moon_interaction_reach(moon_interaction_from_sp(p),moon_interaction_from_sp(at),4.5f)){
            int ok=1;void *args[]={&peer};PTR player=call(su_method("MoonCleaners","MoonCrew","PlayerId","System.UInt64"),0,args,&ok);SURoot owner=su_hold(ok?player:0);
            code=purchase_for(su_get(root),su_get(sw.world),su_get(owner),action,lhc_quote(bytes,n));su_drop(&owner);
        }
    }
    su_drop(&root);return lhc_response(out,code,&helpers);
}
static int helper_result(const unsigned char *bytes,unsigned n){
    if(!lhc_response_valid(bytes,n))return 0;update_prices_capacity(mup_r32(bytes+12));purchase_feedback(mup_r32(bytes+4),mup_r32(bytes+8),mup_r32(bytes+12));event("host-purchase-result");return 1;
}
static void teardown(void){
    if(lh_hints)lh_hints->clear("little-helpers");
    lv_override_clip=-1;
    save_helpers();for(unsigned i=0;i<LH_MAX;i++){lha_stop(i);if(lv_helpers[i].made)lv_drop(&lv_helpers[i]);}
    su_drop(&helper_overlap_array);helper_physics_bound=0;
    ld_drop();lh_zero(routes,sizeof(routes));if(test_tools&&helper_route_probe)lhp_zero(helper_route_probes,sizeof(helper_route_probes));lh_zero(dust_probes,sizeof(dust_probes));
    for(unsigned i=0;i<5;i++)if(labels[i]){int ok=1;PTR filter=su_component(su_get(labels[i]),"MeshFilter");PTR mesh=call(su_method("UnityEngine","MeshFilter","get_sharedMesh",""),filter,0,&ok);if(ok&&mesh)su_destroy(mesh);}
    if(terminal)su_destroy(su_get(terminal));su_drop(&terminal);for(unsigned i=0;i<2;i++)su_drop(&buttons[i]);for(unsigned i=0;i<5;i++)su_drop(&labels[i]);lt_drop();terminal_hint[0]=0;
    for(unsigned i=0;i<LH_MATERIAL_COUNT;i++){su_destroy(su_get(lv_materials[i]));su_drop(&lv_materials[i]);}for(unsigned i=0;i<3;i++){su_destroy(su_get(lv_textures[i]));su_drop(&lv_textures[i]);}su_destroy(su_get(lv_label_material));su_drop(&lv_label_material);
    for(unsigned i=0;i<sw.support_count;i++)su_drop(&sw.supports[i].transform);sw.support_count=0;su_drop(&sw.world);ready=0;world_frames=0;save_root[0]=0;helper_sim_tick=0;helper_replica_reset();event("world-teardown");
}
#include "test_tools.h"
static int before(int id,Context *context){
    (void)id;if(failed)return 0;if(!thread_id){thread_id=(int)GetCurrentThreadId();event("main-thread");}if((DWORD)thread_id!=GetCurrentThreadId())return 0;ticks++;
    PTR walker=(PTR)context->rcx,world=0;field_value(walker,sw.world_field,&world);if(sw.world&&(!su_alive(su_get(sw.world))||su_get(sw.world)!=world))teardown();if(!world)return 0;
    int authoritative=authority()&&helper_units->host();
    int ok=1;if(boolean(sw.restoring,world,&ok)||!ok){world_frames=0;return 0;}
    if(!ready){
        if(++world_frames<120)return 0;sw.world=su_hold(world);sw.center=su_vector(su.tr_get_position,sw_transform(world));PTR path=call(sw.save_root,world,0,&ok);
        if(!ok||!su_string(path,save_root,sizeof(save_root))||!lv_resources()){teardown();failed=1;event("world-begin-failed");return 0;}
        dirty=0;lh_init(&helpers);store_failed=authoritative&&!load_helpers();if(store_failed){event("invalid-helper-state");teardown();failed=1;return 0;}sp_init(&navigation,sw_hash(save_root),1);lh_zero(routes,sizeof(routes));if(test_tools&&helper_route_probe)lhp_zero(helper_route_probes,sizeof(helper_route_probes));ld_load();ready=1;event("world-ready");
    }
    float elapsed=su_number(helper_real_delta,0);if(!lh_finite(elapsed)||elapsed<0)return 0;
    live_dt=su_number(su.time_delta,0);if(!lh_finite(live_dt)||live_dt<0)return 0;if(live_dt>.1f)live_dt=.1f;
    if(authoritative)for(unsigned i=0;i<helpers.owned;i++)if(!helpers.workers[i].active){SPVec3 pos,up,heading;if(spawn_point(walker,i,&pos,&up,&heading))activate(i,pos,up,heading);}
    if(ticks%120==0){make_terminal(walker);if(!authoritative)update_prices();}
    float render_dt=live_dt;
    if(authoritative){
        ld_tick(walker);int frame=integer(helper_frame,0,&ok);
        unsigned steps=ok&&frame>=0?helper_units->steps(2,(U64)frame,su_number(su.time_delta,0)):0;
        LHWorld callbacks={0,contact,scan,move,brush,dust,clean};live_dt=.05f;
        for(unsigned step=0;step<steps;step++){helper_sim_tick++;lh_tick(&helpers,live_dt,&callbacks);observe_dust(live_dt);}
        for(unsigned i=0;i<LH_MAX;i++){
            LHWorker *p=&helpers.workers[i];
            if(i<helpers.owned&&p->active){
                if(!helper_units->publish(2,i+1,2,(MoonUnitVector){p->position.x,p->position.y,p->position.z},.2f)&&!helper_registry_failed){helper_registry_failed=1;event("unit-publish-failed");}
            }
            else helper_units->remove(2,i+1);
        }
    }else if(helper_replica.valid){
        lhr_present(&helper_replica,&helpers,elapsed);
        if(helper_units->session())ld_tick(walker);
    }
    else {helpers.owned=0;helpers.capacity=1;}
    live_dt=authoritative?render_dt:elapsed;if(live_dt>.1f)live_dt=.1f;
    for(unsigned i=0;i<LH_MAX;i++){
        if(i>=helpers.owned||!helpers.workers[i].active){lha_stop(i);if(lv_helpers[i].made)lv_drop(&lv_helpers[i]);continue;}
        if(!lv_helpers[i].made&&!lv_make(&lv_helpers[i])){lv_drop(&lv_helpers[i]);failed=1;event("helper-visual-failed");return 0;}lv_pose(&lv_helpers[i],&helpers.workers[i],live_dt);
        lha_frame(i,&helpers.workers[i],&lv_helpers[i],render_dt>0?live_dt:0);
    }
    save_timer+=elapsed;if(save_timer>2){save_timer=0;save_helpers();}
    DWORD pid=0;window_pid(foreground_window(),&pid);int allow=pid==GetCurrentProcessId()&&boolean(focus,0,&ok)&&!boolean(sw.blocked,walker,&ok)&&!boolean(sw.typing,0,&ok)&&boolean(sw.firstperson,walker,&ok)&&ok;
    if(allow&&authoritative)test_input(walker,world);if(ticks%120==0)test_sample(world);
    MoonInteractionState interaction=moon_interaction_sample(lh_interact);terminal_refresh_hint();lh_interaction_hint(walker,interaction);
    if(allow&&interaction.input.pressed&&moon_interaction_take(lh_interact,walker)){
        MoonInteractionTarget current={0};
        if(lh_interaction_query(0,walker,&current)&&current.identity==interaction.target&&current.token==interaction.token){
            if(interaction.token<2)purchase(walker,world,interaction.token?LH_UPGRADE:LH_BUY);
            else {unsigned i=interaction.token-2;if(i<helpers.owned&&lv_helpers[i].made){lv_helpers[i].greet_remaining=2;event("helper-greet");}}
        }
    }return 0;
}
static void helper_unit_frame(PTR walker){Context context={0};context.rcx=(U64)walker;before(0,&context);}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    char folder[1000],path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(!n||n>=sizeof(folder))return 0;
    cat(cat(path,folder),"\\little-helpers.jsonl");helper_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(helper_log==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)||!su_bind(game)||!sw_bind()||!ss_bind()){event("bind-failed");return 0;}
    helper_units=mu_connect(domain,game);helper_contacts=mu_connect_contacts();helper_frame=su_method("UnityEngine","Time","get_frameCount","");helper_real_delta=su_method("UnityEngine","Time","get_unscaledDeltaTime","");
    if(!helper_units||!helper_frame||!helper_real_delta){event("unit-runtime-unavailable");return 0;}
    event(helper_contacts?"shared-contacts-ready":"shared-contacts-fallback");
    if(!lha_setup(domain,game)){event("sound-framework-unavailable");return 0;}
    clean_stroke=su_method("MoonCleaners","MoonWorld","CleanStroke","UnityEngine.Vector3;UnityEngine.Vector3;System.Single;System.Single");
    local_id=su_method("Steamworks","SteamUser","GetSteamID","");host_id=su_method("MoonCleaners","MoonCrew","get_HostId","");joined=su_method("MoonCleaners","MoonCrew","get_Joined","");focus=method("UnityEngine","Application","get_isFocused",0);
    if(!clean_stroke||method_token(clean_stroke)!=100664816||!local_id||!host_id||!joined||!focus){event("game-api-mismatch");return 0;}
    PTR user=LoadLibraryA("user32.dll");key_state=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");foreground_window=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");
    if(!key_state||!foreground_window||!window_pid)return 0;sw.support_in_use=support_used;
    if(!lh_interactions_bind(domain,game)){event("interaction-framework-unavailable");return 0;}
    test_api_info(game);
    ld_ready=ld_bind(game);if(!ld_ready)event("door-interface-unavailable");
    char test[8];DWORD test_n=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test,sizeof(test));test_tools=test_n==1&&test[0]=='1';
    test_n=GetEnvironmentVariableA("MOONLAB_HELPER_ROUTE_PROBE",test,sizeof(test));helper_route_probe=test_tools&&test_n==1&&test[0]=='1';
    test_n=GetEnvironmentVariableA("MOONLAB_HELPER_PHYSICS_IDS",test,sizeof(test));helper_physics_ids=helper_route_probe&&test_n==1&&test[0]=='1';
    MoonUnitProvider provider={sizeof(provider),2,helper_snapshot,helper_receive,helper_replica_reset,helper_unit_frame,helper_command,helper_result};
    if(!helper_units->register_provider(&provider)){event("unit-runtime-unavailable");return 0;}
    /* The dependency owns the one Update hook in standalone and loader runs. */
    (void)prepare_hook;(void)install_hooks;event("ready");return 1;
}
