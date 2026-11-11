#ifndef LITTLE_HELPERS_TEST_TOOLS_H
#define LITTLE_HELPERS_TEST_TOOLS_H
/* Only the disposable harness opts in. Production input has no setup shortcuts. */
static int test_funds_held,test_view_held,test_inspect_held;
static unsigned test_button,test_helper;
static int test_overview_held;
static int test_aim=-1,test_aim_reported;
static int test_material_held;static unsigned test_material_mode;
static int test_animation_held;
static float test_uv[LH_VERTEX_COUNT*2];
static int test_helper_observer(LHWorker *worker,SPVec3 *position,SPVec3 *facing){
    /* A worker may face a wall while routing. Fit the disposable observer to
     * real floor/capsule and sight probes instead of teleporting into that wall. */
    SPVec3 up=worker->up,forward=worker->heading,right=su_normal(su_cross(up,forward));
    const float directions[8][2]={{0,1},{.7071f,.7071f},{-.7071f,.7071f},{1,0},{-1,0},{.7071f,-.7071f},{-.7071f,-.7071f},{0,-1}};
    const float radii[3]={1.4f,2.f,2.7f};
    SPVec3 target=su_sub(worker->position,su_mul(up,.05f));
    for(unsigned ring=0;ring<3;ring++)for(unsigned i=0;i<8;i++){
        SPVec3 offset=su_mul(su_add(su_mul(right,directions[i][0]),su_mul(forward,directions[i][1])),radii[ring]);
        SPGround ground;SPVec3 query=su_add(worker->position,su_add(offset,su_mul(up,2)));
        if(!sw_ground(&sw,query,up,6,&ground)||su_dot(ground.normal,up)<SP_SLOPE_COS)continue;
        SPVec3 pos=su_add(ground.point,su_mul(up,.93f));
        if(!sw_clear(&sw,pos,pos,up,.38f,1.8f,ground.support_id))continue;
        SPVec3 eye=su_add(pos,su_mul(up,.65f)),delta=su_sub(target,eye);float length=su_sqrt(su_dot(delta,delta));
        union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));
        if(length<.2f||sw_ray(eye,su_mul(delta,1/length),length-.1f,hit.bytes))continue;
        *position=pos;*facing=su_normal(su_sub(delta,su_mul(up,su_dot(delta,up))));return 1;
    }
    return 0;
}
static void test_api_info(PTR game){
    const char *(*parameter_name)(PTR,DWORD)=(const char *(*)(PTR,DWORD))GetProcAddress(game,"il2cpp_method_get_param_name");
    PTR (*return_type)(PTR)=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_method_get_return_type");
    if(parameter_name&&return_type){
        char *type=su.type_name(return_type(clean_stroke));char line[512],*p=cat(line,"{\"event\":\"clean-api\",\"return_type\":\"");p=cat(p,type);p=cat(p,"\",\"parameters\":[");
        for(unsigned i=0;i<4;i++){if(i)*p++=',';*p++='\"';p=cat(p,parameter_name(clean_stroke,i));*p++='\"';}cat(p,"]}\n");record(line);su.free(type);
    }
    const char *names[]={"Stay","FixedUpdate"};
    for(unsigned i=0;i<2;i++){
        PTR m=method("MoonCleaners","MoonCollectorDoor",names[i],i?0:1);if(!m)continue;
        char line[512],*p=cat(line,"{\"event\":\"door-api\",\"method\":\"");p=cat(p,names[i]);p=cat(p,"\",\"token\":");p=num(p,method_token(m));p=cat(p,",\"rva\":");p=num(p,(U64)*(PTR *)m-(U64)game);cat(p,"}\n");record(line);
    }
    PTR field=get_field(find_class("MoonCleaners","MoonCollectorDoor"),"presence");PTR (*field_type)(PTR)=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_field_get_type");
    if(field&&field_type){char *name=su.type_name(field_type(field));char line[512],*p=cat(line,"{\"event\":\"door-presence-type\",\"type\":\"");p=cat(p,name);cat(p,"\"}\n");record(line);su.free(name);}
}
static int test_textures_recorded;
static void test_texture_readback(void){
    if(!test_tools||test_textures_recorded)return;test_textures_recorded=1;
    PTR pixel=su_method("UnityEngine","Texture2D","GetPixel","System.Int32;System.Int32");
    if(!pixel){event("texture-readback-unavailable");return;}
    const int coordinates[4][2]={{37,211},{433,1530},{1097,87},{2011,1701}};
    for(unsigned texture=0;texture<3;texture++)for(unsigned i=0;i<4;i++){
        int x=coordinates[i][0],y=coordinates[i][1],ok=1;void *args[]={&x,&y};PTR value=call(pixel,su_get(lv_textures[texture]),args,&ok);
        if(!ok||!value){event("texture-readback-unavailable");return;}SUColor color=*(SUColor *)unbox(value);
        char line[512],*p=cat(line,"{\"event\":\"texture-pixel\",\"texture\":");p=num(p,texture);p=cat(p,",\"x\":");p=num(p,(U64)x);p=cat(p,",\"y\":");p=num(p,(U64)y);p=cat(p,",\"rgba\":[");p=measure(p,color.r);*p++=',';p=measure(p,color.g);*p++=',';p=measure(p,color.b);*p++=',';p=measure(p,color.a);cat(p,"]}\n");record(line);
    }
}
static inline float test_eye_upload_error(LHVisual *visual){
#if LH_MORPH_COUNT
    if(!visual->made)return -1;
    PTR getter=su_method("UnityEngine","Mesh","get_vertices","");int ok=1;
    PTR array=getter?call(getter,su_get(visual->mesh),0,&ok):0;
    if(!ok||!array||su.array_length(array)!=LH_VERTEX_COUNT)return -1;
    SURoot held=su_hold(array);if(!held)return -1;
    const SPVec3 *actual=su.array_address(su_get(held),sizeof(SPVec3),0);
    const SPVec3 *expected=su.array_address(su_get(visual->vertices),sizeof(SPVec3),0);
    float error=0;
    for(unsigned i=0;i<LH_MORPH_VERTEX_COUNT;i++){
        unsigned vertex=lh_morph_vertices[i];SPVec3 delta=su_sub(actual[vertex],expected[vertex]);float squared=su_dot(delta,delta);
        if(squared>error)error=squared;
    }
    su_drop(&held);return lva_sqrt(error);
#else
    (void)visual;return -1;
#endif
}
static char *test_probe_vector(char *p,SPVec3 v){*p++='[';p=measure(p,v.x);*p++=',';p=measure(p,v.y);*p++=',';p=measure(p,v.z);*p++=']';*p=0;return p;}
static char *test_probe_string(char *p,const char *value){
    *p++='"';for(unsigned i=0;i<96&&value[i];i++){unsigned char c=(unsigned char)value[i];if(c=='"'||c=='\\')*p++='\\';*p++=c<' '?'?':(char)c;}*p++='"';*p=0;return p;
}
static void test_route_probes(void){
    static char line[4096];
    for(unsigned i=0;i<helper_door_count;i++){
        LDDoor *door=&helper_doors[i];char *p=cat(line,"{\"event\":\"helper-door-bounds\",\"door\":");p=num(p,i);p=cat(p,",\"amount\":");p=measure(p,door->amount);p=cat(p,",\"center\":");p=test_probe_vector(p,door->box.center);p=cat(p,",\"extents\":");p=test_probe_vector(p,door->box.extents);cat(p,"}\n");record(line);
    }
    for(unsigned slot=0;slot<helpers.owned;slot++){
        LHRouteProbe *probe=&helper_route_probes[slot];
        unsigned recent=probe->rejected<32?probe->rejected:32;
        for(unsigned observation=1;observation<LHP_KINDS+recent;observation++){
            int history=observation>=LHP_KINDS;unsigned kind=observation;
            LHProbeEdge *edge;
            if(history){unsigned index=(probe->rejected-recent+observation-LHP_KINDS)%32;edge=&probe->recent[index];kind=edge->rejection_kind;}
            else {if(!probe->counts[kind])continue;edge=&probe->nearest[kind];}
            char *p=cat(line,"{\"event\":\"helper-route-rejection\",\"worker\":");p=num(p,slot);
            p=cat(p,",\"history\":");p=num(p,(unsigned)history);p=cat(p,",\"retained\":");p=num(p,recent);
            p=cat(p,",\"kind\":");p=num(p,kind);p=cat(p,",\"last_callback\":");p=num(p,edge->callback);p=cat(p,",\"callback_result\":");p=num(p,edge->result);
            p=cat(p,",\"attempts\":");p=num(p,probe->attempts);p=cat(p,",\"accepted\":");p=num(p,probe->accepted);p=cat(p,",\"rejected\":");p=num(p,probe->rejected);p=cat(p,",\"kind_count\":");p=num(p,probe->counts[kind]);
            p=cat(p,",\"score\":");p=measure(p,edge->score);p=cat(p,",\"from\":");p=test_probe_vector(p,edge->from);p=cat(p,",\"to\":");p=test_probe_vector(p,edge->to);
            p=cat(p,",\"query\":");p=test_probe_vector(p,edge->query);p=cat(p,",\"clear_to\":");p=test_probe_vector(p,edge->clear_to);p=cat(p,",\"up\":");p=test_probe_vector(p,edge->up);
            p=cat(p,",\"drop\":");p=measure(p,edge->drop);p=cat(p,",\"radius\":");p=measure(p,edge->radius);p=cat(p,",\"height\":");p=measure(p,edge->height);p=cat(p,",\"support\":");p=num(p,edge->support);
            p=cat(p,",\"ground_seen\":");p=num(p,edge->ground_seen);p=cat(p,",\"ground\":");p=test_probe_vector(p,edge->ground.point);p=cat(p,",\"normal\":");p=test_probe_vector(p,edge->ground.normal);p=cat(p,",\"ground_support\":");p=num(p,edge->ground.support_id);
            p=cat(p,",\"clear_reason\":");p=num(p,edge->detail.reason);p=cat(p,",\"marker_provider\":");p=num(p,edge->detail.provider);p=cat(p,",\"marker_id\":");p=num(p,edge->detail.id);p=cat(p,",\"marker_position\":");p=test_probe_vector(p,edge->detail.position);p=cat(p,",\"marker_radius\":");p=measure(p,edge->detail.radius);
            LHProbePhysics *physics=&edge->detail.physics;
            p=cat(p,",\"physics_stage\":");p=num(p,physics->stage);p=cat(p,",\"physics_identity_enabled\":");p=num(p,physics->identity_enabled);p=cat(p,",\"physics_identity_query_ok\":");p=num(p,physics->query_ok);p=cat(p,",\"physics_hit_count\":");p=num(p,physics->count);p=cat(p,",\"physics_saturated\":");p=num(p,physics->saturated);
            p=cat(p,",\"physics_from\":");p=test_probe_vector(p,physics->from);p=cat(p,",\"physics_to\":");p=test_probe_vector(p,physics->to);p=cat(p,",\"physics_up\":");p=test_probe_vector(p,physics->up);p=cat(p,",\"physics_radius\":");p=measure(p,physics->radius);p=cat(p,",\"physics_height\":");p=measure(p,physics->height);
            p=cat(p,",\"physics_colliders\":[");
            for(unsigned j=0;j<physics->observed&&j<3;j++){
                LHProbeCollider *c=&physics->colliders[j];if(j)*p++=',';p=cat(p,"{\"id\":");p=num(p,c->id);p=cat(p,",\"name\":");p=test_probe_string(p,c->name);
                p=cat(p,",\"hit_point\":");p=test_probe_vector(p,c->point);p=cat(p,",\"hit_normal\":");p=test_probe_vector(p,c->normal);p=cat(p,",\"bounds_center\":");p=test_probe_vector(p,c->center);p=cat(p,",\"bounds_extents\":");p=test_probe_vector(p,c->extents);*p++='}';
            }
            cat(p,"]}\n");record(line);
        }
    }
}
static void test_sample(PTR world){
    if(!test_tools)return;
    if(helper_route_probe)test_route_probes();
    int ok=1;PTR player=call(method("MoonCleaners","MoonCrew","get_LocalPlayerId",0),0,0,&ok);
    unsigned balance=ok?sw_balance(world,player,&ok):0;static char line[4096];char *p=cat(line,"{\"event\":\"helper-sample\",\"thread\":");p=num(p,(U64)thread_id);
    p=cat(p,",\"owned\":");p=num(p,helpers.owned);p=cat(p,",\"capacity\":");p=num(p,helpers.capacity);
    p=cat(p,",\"balance\":");p=num(p,balance);p=cat(p,",\"balance_ok\":");p=num(p,(U64)ok);
    p=cat(p,",\"strokes\":");p=num(p,helpers.strokes);p=cat(p,",\"cells\":");p=num(p,helpers.cells);p=cat(p,",\"workers\":[");
    for(unsigned i=0;i<helpers.owned;i++){
        LHWorker *worker=&helpers.workers[i];if(i)*p++=',';p=cat(p,"{\"active\":");p=num(p,worker->active);p=cat(p,",\"mode\":");p=num(p,worker->mode);
#if LH_MORPH_COUNT
        p=cat(p,",\"eye_morphs\":");p=num(p,LH_MORPH_COUNT);p=cat(p,",\"eyes\":[");
        for(unsigned k=0;k<3;k++){if(k)*p++=',';p=measure(p,lv_helpers[i].eye_weights[k]);}
        p=cat(p,"],\"eye_upload_error\":");p=measure(p,test_eye_upload_error(&lv_helpers[i]));
#endif
        p=cat(p,",\"visual\":");p=num(p,(U64)lv_helpers[i].made);p=cat(p,",\"clip\":\"");p=cat(p,lh_clips[lv_helpers[i].clip].name);p=cat(p,"\",\"animation_time\":");p=measure(p,lv_helpers[i].sample_time);p=cat(p,",\"skin_min\":");p=measure(p,lv_helpers[i].skin_min);p=cat(p,",\"visual_updates\":");p=num(p,lv_helpers[i].updates);p=cat(p,",\"route\":");p=num(p,routes[i].state);p=cat(p,",\"expanded\":");p=num(p,routes[i].expanded);p=cat(p,",\"position\":[");p=measure(p,worker->position.x);*p++=',';p=measure(p,worker->position.y);*p++=',';p=measure(p,worker->position.z);
        p=cat(p,"],\"target\":[");p=measure(p,worker->target.x);*p++=',';p=measure(p,worker->target.y);*p++=',';p=measure(p,worker->target.z);p=cat(p,"]}");
    }
    p=cat(p,"],\"doors\":[");for(unsigned i=0;i<helper_door_count;i++){if(i)*p++=',';p=measure(p,helper_doors[i].amount);}cat(p,"]}\n");record(line);
}
static void test_input(PTR walker,PTR world){
    if(!test_tools||!authority())return;
    /* Ctrl+Shift+F6 belongs to the runtime's native crowd fixture. Consume
     * the held F6 edge even with modifiers so release cannot advance a clip. */
    int animation=(key_state(0x75)&0x8000)!=0,modified=((key_state(0x10)|key_state(0x11)|key_state(0x12))&0x8000)!=0;
    if(animation&&!modified&&!test_animation_held){lv_override_clip++;if(lv_override_clip>=(int)LH_CLIP_COUNT)lv_override_clip=-1;test_helper=0;event(lv_override_clip<0?"test-animation-restored":"test-animation-selected");}
    test_animation_held=animation;
    test_texture_readback();
    int funds=(key_state(0x76)&0x8000)!=0,view=(key_state(0x77)&0x8000)!=0,inspect=(key_state(0x78)&0x8000)!=0;
    if(funds&&!test_funds_held){
        int value=2000,ok=1;unsigned char decimal[16];su_zero(decimal,sizeof(decimal));void *ctor[]={&value};call(sw.decimal_int,decimal,ctor,&ok);
        void *args[]={decimal};call(method("MoonCleaners","MoonWorld","SetTeamDollars",1),world,args,&ok);event(ok?"test-funds-ready":"test-funds-failed");
    }
    if(view&&!test_view_held&&terminal){
        unsigned index=test_button++%2;PTR root=su_transform(su_get(terminal)),button=su_transform(su_get(buttons[index]));
        SPVec3 up=su_vector(su.tr_up,root),forward=su_vector(su.tr_forward,root),center=su_vector(su.tr_get_position,button);
        SPVec3 query=su_add(center,su_add(su_mul(forward,2.3f),su_mul(up,2))),facing=su_mul(forward,-1);SPGround ground;
        if(!sw_ground(&sw,query,up,6,&ground))return;
        SPVec3 pos=su_add(ground.point,su_mul(up,.95f));float pitch=0;int ok=1;unsigned char refresh=1;
        void *teleport[]={&pos,&refresh};call(method("MoonCleaners","MoonWalker","Teleport",2),walker,teleport,&ok);
        void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);if(!ok)event("test-terminal-view-failed");else {test_aim=(int)index;test_aim_reported=0;}
    }
    int overview=(key_state(0x7a)&0x8000)!=0;
    if(overview&&!test_overview_held&&terminal){
        PTR root=su_transform(su_get(terminal));SPVec3 up=su_vector(su.tr_up,root),normal=su_vector(su.tr_forward,root),right=su_vector(su.tr_right,root),center=su_vector(su.tr_get_position,root);
        SPVec3 query=su_add(center,su_add(su_mul(normal,5.5f),su_add(su_mul(right,2.4f),su_mul(up,2))));SPGround ground;
        if(sw_ground(&sw,query,up,7,&ground)){
            SPVec3 pos=su_add(ground.point,su_mul(up,.95f)),facing=su_mul(normal,-1);float pitch=-5;int ok=1;unsigned char refresh=1;
            void *teleport[]={&pos,&refresh};call(method("MoonCleaners","MoonWalker","Teleport",2),walker,teleport,&ok);
            void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);test_aim=-1;
            event(ok?"test-terminal-context":"test-terminal-context-failed");
        }
    }
    test_overview_held=overview;
    if(inspect&&!test_inspect_held&&helpers.owned){
        unsigned index=test_helper++%helpers.owned;
        if(lv_override_clip>=0)index=0;
        else for(unsigned i=0;i<helpers.owned;i++)if(helpers.workers[i].mode==LH_SWEEP){index=i;break;}
        SPVec3 pos,facing;float pitch=0;int ok=1;unsigned char refresh=1;
        if(test_helper_observer(&helpers.workers[index],&pos,&facing)){
            void *teleport[]={&pos,&refresh};call(method("MoonCleaners","MoonWalker","Teleport",2),walker,teleport,&ok);
            void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);
            if(ok){test_aim=(int)index+2;test_aim_reported=0;}else event("test-helper-view-failed");
        }else event("test-helper-view-failed");
    }
    if(test_aim>=0){
        /* Place positions the walker, not the camera. Fit against the actual
         * updated eye transform so capsule settling cannot move the reticle. */
        PTR camera=0;field_value(walker,sw.camera_field,&camera);
        if(camera){
            PTR root=su_transform(su_get(terminal));SPVec3 up=test_aim<2?su_vector(su.tr_up,root):helpers.workers[test_aim-2].up,eye=su_vector(su.tr_get_position,sw_transform(camera));
            SPVec3 target=test_aim<2?su_vector(su.tr_get_position,su_transform(su_get(buttons[test_aim]))):su_sub(helpers.workers[test_aim-2].position,su_mul(up,.05f)),delta=su_sub(target,eye);
            float vertical=-su_dot(delta,up);SPVec3 flat=su_sub(delta,su_mul(up,su_dot(delta,up)));float horizontal=su_sqrt(su_dot(flat,flat));
            int ok=1;void *angles[]={&vertical,&horizontal};PTR angle=call(su_method("UnityEngine","Mathf","Atan2","System.Single;System.Single"),0,angles,&ok);
            if(ok&&angle){
                float pitch=*(float *)unbox(angle)*57.2957795f;SPVec3 facing=su_normal(flat),pos=su_vector(su.tr_get_position,sw_transform(walker));
                void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);
                int aimed=test_aim<2?target_button(walker)==test_aim:su_dot(su_vector(su.tr_forward,sw_transform(camera)),su_normal(delta))>.998f;
                if(!test_aim_reported&&aimed){event(test_aim<2?(test_aim?"test-upgrade-view":"test-buy-view"):"test-helper-view");test_aim_reported=1;}
            }else if(!test_aim_reported){event("test-aim-failed");test_aim_reported=1;}
        }
    }
    int material=(key_state(0x79)&0x8000)!=0;
    if(material&&!test_material_held){
        test_material_mode=(test_material_mode+1)%3;int ok=1;
        const char *keywords[]={"_NORMALMAP","_METALLICSPECGLOSSMAP","_OCCLUSIONMAP"};
        for(unsigned i=0;i<3;i++){void *args[]={su.string(keywords[i])};call(test_material_mode?su_method("UnityEngine","Material","DisableKeyword","System.String"):su.material_keyword,su_get(lv_materials[0]),args,&ok);}
        lv_float(su_get(lv_materials[0]),"_Metallic",test_material_mode?0:LH_PLUSH_METALLIC);lv_float(su_get(lv_materials[0]),"_Smoothness",test_material_mode?0:LH_PLUSH_SMOOTHNESS);
        const float *uv=(const float *)lh_data_uv;
        for(unsigned i=0;i<LH_VERTEX_COUNT;i++){test_uv[i*2]=uv[i*2];test_uv[i*2+1]=test_material_mode==2?1-uv[i*2+1]:uv[i*2+1];}
        PTR array=su_array(su.uv_class,LH_VERTEX_COUNT,test_uv,8);
        for(unsigned i=0;i<helpers.owned;i++)if(lv_helpers[i].made){void *args[]={array};call(su.mesh_uv,su_get(lv_helpers[i].mesh),args,&ok);}
        event(ok?(test_material_mode==1?"test-material-matte":test_material_mode==2?"test-material-flipped-uv":"test-material-restored"):"test-material-control-failed");
    }
    test_material_held=material;
    test_funds_held=funds;test_view_held=view;test_inspect_held=inspect;
}
#endif
