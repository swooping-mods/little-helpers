#ifndef LITTLE_HELPERS_TERMINAL_H
#define LITTLE_HELPERS_TERMINAL_H
#include "terminal_controls.h"
static SURoot lt_materials[4],lt_meshes[6],lt_title_ink;
static LTControls lt_controls;
static void lt_tick(float dt){
    if(!terminal)return;lt_controls_step(&lt_controls,dt);
    for(unsigned i=0;i<2;i++)if(buttons[i]){
        LTPoint p=lt_controls_position(&lt_controls,i);
        su_set_vector(su.tr_local_position,su_transform(su_get(buttons[i])),su_v(p.x,p.y,p.z));
    }
}
static void lt_press(unsigned index){lt_controls_press(&lt_controls,index);}
static void lt_emission(PTR material,SUColor color){
    int ok=1;void *keyword[]={su.string("_EMISSION")},*value[]={su.string("_EmissionColor"),&color};
    call(su.material_keyword,material,keyword,&ok);call(su_method("UnityEngine","Material","SetColor","System.String;UnityEngine.Color"),material,value,&ok);
}
static LTMesh lt_buffer;
static int lt_native_mesh_check(PTR mesh){
    if(!test_tools)return 1;const char *names[]={"get_vertices","get_normals","get_triangles"};SURoot arrays[3]={0};int ok=1;unsigned positive=0,count=0;
    for(unsigned i=0;i<3;i++){arrays[i]=su_hold(call(su_method("UnityEngine","Mesh",names[i],""),mesh,0,&ok));if(!ok||!arrays[i])goto done;}
    count=(unsigned)su.array_length(su_get(arrays[2]));unsigned vertices=(unsigned)su.array_length(su_get(arrays[0]));
    if(count!=lt_buffer.indices_count||vertices!=lt_buffer.vertices||su.array_length(su_get(arrays[1]))!=vertices){ok=0;goto done;}
    SPVec3 *v=su.array_address(su_get(arrays[0]),12,0),*n=su.array_address(su_get(arrays[1]),12,0);int *indices=su.array_address(su_get(arrays[2]),4,0);
    for(unsigned i=0;i<count;i+=3){unsigned a=(unsigned)indices[i],b=(unsigned)indices[i+1],c=(unsigned)indices[i+2];
        if(a>=vertices||b>=vertices||c>=vertices){ok=0;goto done;}
        if(su_dot(su_cross(su_sub(v[b],v[a]),su_sub(v[c],v[a])),n[a])>0)positive++;
    }
    ok=positive==count/3;
done:
    for(unsigned i=0;i<3;i++)su_drop(&arrays[i]);
    char line[192],*p=cat(line,"{\"event\":\"terminal-mesh-");p=cat(p,ok?"checked":"failed");p=cat(p,"\",\"triangles\":");p=num(p,count/3);p=cat(p,",\"positive\":");p=num(p,positive);cat(p,"}\n");record(line);return ok;
}
static PTR lt_render(PTR parent,const char *name,SURoot material,unsigned mesh_slot){
    int ok=1;PTR go=su_gameobject(name,parent),mesh=su_new("UnityEngine","Mesh");if(!go||!mesh)return 0;lt_meshes[mesh_slot]=su_hold(mesh);call(su.mesh_ctor,mesh,0,&ok);
    PTR vertices=su_array(su.vector_class,lt_buffer.vertices,lt_buffer.positions,12),normals=su_array(su.vector_class,lt_buffer.vertices,lt_buffer.normals,12),indices=su_array(su.int_class,lt_buffer.indices_count,lt_buffer.indices,4);
    void *va[]={vertices},*na[]={normals};call(su.mesh_vertices,mesh,va,&ok);call(su.mesh_normals,mesh,na,&ok);su_int(su.mesh_subcount,mesh,1);
    int index=0;unsigned char bounds=1;void *ia[]={indices,&index,&bounds};call(su.mesh_indices,mesh,ia,&ok);
    PTR filter=su_add_component(go,"MeshFilter"),renderer=su_add_component(go,"MeshRenderer");void *ma[]={mesh};call(su.meshfilter_set,filter,ma,&ok);lv_assign(renderer,&material,1);return ok&&lt_native_mesh_check(mesh)?go:0;
}
static int lt_anchor(SPVec3 *point,SPVec3 *normal,SPVec3 *up){
    int ok=1;PTR stations=call(sw.stations,su_get(sw.world),0,&ok);int count=sw_list_count(stations);
    if(!ok||count<1||count>16)return 0;
    for(int i=0;i<count;i++){
        PTR station=sw_list_item(stations,i);if(!su_alive(station))continue;void *a[]={station};PTR id=call(sw.station_id,0,a,&ok);char name[64];
        if(!ok||!su_string(id,name,sizeof(name))||!su_eq(name,"station/north"))continue;
        PTR tr=sw_transform(station);SPVec3 origin=su_vector(su.tr_get_position,tr),right=su_vector(su.tr_right,tr),forward=su_vector(su.tr_forward,tr);*up=su_vector(su.tr_up,tr);
        /* Pinned Collector layout: the panel immediately to the whiteboard's
         * right. Fit to the actual solid wall, never the player's spawn pose. */
        SPVec3 query=su_add(origin,su_add(su_mul(right,-5),su_add(su_mul(*up,2.65f),su_mul(forward,-4.55f))));
        union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));if(!sw_ray(query,su_mul(right,-1),2.5f,hit.bytes))continue;
        SPVec3 p,n;su_copy(&p,hit.bytes+su.hit_point,12);su_copy(&n,hit.bytes+su.hit_normal,12);
        float wall_x=su_dot(su_sub(p,origin),right);
        if(wall_x< -7.25f||wall_x> -6.65f||su_dot(n,right)<.97f)continue;
        *point=su_add(p,su_mul(n,.008f));*normal=n;return 1;
    }return 0;
}
static void make_terminal(PTR walker){
    (void)walker;if(terminal)return;SPVec3 point,normal,up;if(!lt_anchor(&point,&normal,&up))return;
    lt_controls_init(&lt_controls);
    static const SUColor colors[]={{.78f,.79f,.76f,1},{.015f,.020f,.026f,1},{.003f,.008f,.012f,1},{.95f,.30f,.035f,1}};
    for(unsigned i=0;i<4;i++){lt_materials[i]=su_hold(lv_material(colors[i]));if(!lt_materials[i])goto fail;}
    terminal_button_ink=lt_materials[1];
    lt_title_ink=su_hold(lv_material((SUColor){.9f,.95f,1,1}));if(!lt_title_ink)goto fail;
    lt_emission(su_get(lt_title_ink),(SUColor){.7f,.75f,.8f,1});
    lt_emission(su_get(lv_label_material),(SUColor){.12f,.65f,.46f,1});
    PTR root=su_gameobject("Little Helpers wall terminal",0),tr=su_transform(root);terminal=su_hold(root);if(!terminal)goto fail;
    su_set_vector(su.tr_position,tr,point);su_set_quat(su.tr_rotation,tr,su_rotation(normal,up));
    {PTR body=su_add_component(root,"BoxCollider");SPVec3 center=su_v(0,0,.094f),size=su_v(1.85f,1.40f,.13f);int ok=1;void *c[]={&center},*s[]={&size};
    call(su_method("UnityEngine","BoxCollider","set_center","UnityEngine.Vector3"),body,c,&ok);call(su_method("UnityEngine","BoxCollider","set_size","UnityEngine.Vector3"),body,s,&ok);if(!ok)goto fail;}
    for(unsigned material=0;material<4;material++){
        lt_buffer.vertices=lt_buffer.indices_count=0;
        for(unsigned i=0;i<sizeof(lt_parts)/sizeof(lt_parts[0]);i++)if(lt_parts[i].material==material&&!lt_shape(&lt_buffer,lt_parts[i]))goto fail;
        if(!lt_render(tr,"Helper terminal casing",lt_materials[material],material))goto fail;
    }
    for(unsigned i=0;i<2;i++){
        LTPart p=lt_buttons[i];p.center=(LTPoint){0,0,0};lt_buffer.vertices=lt_buffer.indices_count=0;if(!lt_shape(&lt_buffer,p))goto fail;
        PTR go=lt_render(tr,p.name,lt_materials[3],i+4);if(!go)goto fail;buttons[i]=su_hold(go);
        su_set_vector(su.tr_local_position,su_transform(go),su_v(lt_buttons[i].center.x,lt_buttons[i].center.y,lt_buttons[i].center.z));
        PTR collider=su_add_component(go,"BoxCollider");SPVec3 size=su_v(p.size.x,p.size.y,p.size.z);void *s[]={&size};int ok=1;call(su_method("UnityEngine","BoxCollider","set_size","UnityEngine.Vector3"),collider,s,&ok);if(!ok)goto fail;
    }
    const char *names[]={"Buy label","Upgrade label","Helper shop status","Helper terminal title","Helper terminal interaction"};
    const SPVec3 positions[]={{.43f,-.322f,.254f},{-.43f,-.322f,.254f},{0,.120f,.201f},{0,.340f,.201f},{0,-.515f,.204f}};
    for(unsigned i=0;i<5;i++){
        /* Lettering and ray collider ride the same cap transform. */
        PTR parent=i<2?su_transform(su_get(buttons[i])):tr;
        SPVec3 position=positions[i];if(i<2)position=su_sub(position,su_v(lt_buttons[i].center.x,lt_buttons[i].center.y,lt_buttons[i].center.z));
        PTR label=su_gameobject(names[i],parent);labels[i]=su_hold(label);if(!labels[i])goto fail;su_set_vector(su.tr_local_position,su_transform(label),position);
    }
    text(su_get(labels[0]),"BUY $75");text(su_get(labels[3]),"LITTLE HELPERS");terminal_hint[0]=0;terminal_refresh_hint();
    {char status[64],*p=cat(status,"HELPERS ");p=num(p,helpers.owned);p=cat(p," / ");num(p,helpers.capacity);terminal_status(status);}
    update_prices();
    for(unsigned i=0;i<2;i++)lv_assign(su_component(su_get(labels[i]),"Renderer"),&lt_materials[1],1);
    lv_assign(su_component(su_get(labels[3]),"Renderer"),&lt_title_ink,1);
    event("terminal-ready");
    if(test_tools){char line[256],*p=cat(line,"{\"event\":\"terminal-wall-anchor\",\"position\":[");p=measure(p,point.x);*p++=',';p=measure(p,point.y);*p++=',';p=measure(p,point.z);cat(p,"],\"width\":1.85,\"height\":1.4,\"depth\":0.26}\n");record(line);}
    return;
fail:
    failed=1;event("terminal-model-failed");
}
static void lt_drop(void){
    lt_controls_init(&lt_controls);
    terminal_button_ink=0;su_destroy(su_get(lt_title_ink));su_drop(&lt_title_ink);
    for(unsigned i=0;i<6;i++){su_destroy(su_get(lt_meshes[i]));su_drop(&lt_meshes[i]);}
    for(unsigned i=0;i<4;i++){su_destroy(su_get(lt_materials[i]));su_drop(&lt_materials[i]);}
}
#endif
