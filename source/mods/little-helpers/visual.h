#ifndef LITTLE_HELPERS_VISUAL_H
#define LITTLE_HELPERS_VISUAL_H
#include "animation_core.h"
#include "eyes_core.h"
#include "adapters/glyphs.h"
#define LH_PLUSH_METALLIC .02f
#define LH_PLUSH_SMOOTHNESS .12f
typedef struct {
    SURoot root,transform,mesh,vertices,normals,tangents;
    float time,sample_time,blend,speed,skin_min,greet_remaining;unsigned mode,clip,updates,changes;
    float last[LH_BONE_COUNT*12],eye_weights[3],eye_from[3];LHPose pose[LH_NODE_COUNT],from[LH_NODE_COUNT];
    SPVec3 observed,position,heading,up;int made,placed,greeting;
} LHVisual;
static LHVisual lv_helpers[LH_MAX];
static int lv_override_clip=-1;
static SURoot lv_materials[LH_MATERIAL_COUNT],lv_textures[3],lv_label_material;
static PTR lv_texture_ctor,lv_load_image,lv_set_texture,lv_set_tangents,lv_vector4;
static inline void lv_float(PTR m,const char *name,float v){int ok=1;void *a[]={su.string(name),&v};call(su.material_float,m,a,&ok);}
static inline PTR lv_material(SUColor c){
    int ok=1;void *a[]={su.string("Universal Render Pipeline/Lit")};PTR shader=call(su.shader_find,0,a,&ok);if(!shader)return 0;
    PTR m=su_new("UnityEngine","Material");void *ctor[]={shader};call(su.material_ctor,m,ctor,&ok);
    void *color[]={&c};call(su.material_color,m,color,&ok);void *base[]={su.string("_BaseColor"),&c};call(su_method("UnityEngine","Material","SetColor","System.String;UnityEngine.Color"),m,base,&ok);
    lv_float(m,"_Smoothness",.16f);return ok?m:0;
}
static inline void lv_assign(PTR renderer,const SURoot *materials,unsigned count){
    PTR array=su.new_array(su.material_class,count);PTR *items=su.array_address(array,8,0);
    for(unsigned i=0;i<count;i++)items[i]=su_get(materials[i]);int ok=1;void *a[]={array};call(su.renderer_materials,renderer,a,&ok);
}
static inline int lv_resources(void){
    lv_texture_ctor=su_method("UnityEngine","Texture2D",".ctor","System.Int32;System.Int32;UnityEngine.TextureFormat;System.Boolean;System.Boolean");
    lv_load_image=su_method("UnityEngine","ImageConversion","LoadImage","UnityEngine.Texture2D;System.Byte[]");
    lv_set_texture=su_method("UnityEngine","Material","SetTexture","System.String;UnityEngine.Texture");
    lv_set_tangents=su_method("UnityEngine","Mesh","set_tangents","UnityEngine.Vector4[]");lv_vector4=find_class("UnityEngine","Vector4");
    if(!lv_texture_ctor||!lv_load_image||!lv_set_texture||!lv_set_tangents||!lv_vector4)return 0;
    for(unsigned i=0;i<LH_MATERIAL_COUNT;i++){const float *c=lh_material_colors[i];lv_materials[i]=su_hold(lv_material((SUColor){c[0],c[1],c[2],c[3]}));if(!lv_materials[i])return 0;}
    lv_label_material=su_hold(lv_material((SUColor){.25f,.92f,.80f,1}));
    if(!lv_label_material)return 0;
    const unsigned char *data[]={lh_data_albedo,lh_data_normal,lh_data_mask};const unsigned sizes[]={sizeof(lh_data_albedo)-1,sizeof(lh_data_normal)-1,sizeof(lh_data_mask)-1};
    for(unsigned i=0;i<3;i++){
        int ok=1,size=2,format=4;unsigned char mip=1,linear=(unsigned char)(i!=0);PTR texture=su_new("UnityEngine","Texture2D");
        void *a[]={&size,&size,&format,&mip,&linear};call(lv_texture_ctor,texture,a,&ok);
        PTR bytes=su_array(su.byte_class,sizes[i],data[i],1);void *load[]={texture,bytes};PTR loaded=call(lv_load_image,0,load,&ok);
        if(!ok||!loaded||!*(unsigned char *)unbox(loaded)){su_destroy(texture);return 0;}lv_textures[i]=su_hold(texture);
    }
    const char *names[]={"_BaseMap","_BumpMap","_MetallicGlossMap","_OcclusionMap"};unsigned tex[]={0,1,2,2};int ok=1;
    for(unsigned i=0;i<4;i++){void *a[]={su.string(names[i]),su_get(lv_textures[tex[i]])};call(lv_set_texture,su_get(lv_materials[0]),a,&ok);}
    /* Keep sensible cloth response when the player's stripped URP build falls
     * back from a requested map variant. All-one constants produced chrome. */
    lv_float(su_get(lv_materials[0]),"_Smoothness",LH_PLUSH_SMOOTHNESS);lv_float(su_get(lv_materials[0]),"_Metallic",LH_PLUSH_METALLIC);lv_float(su_get(lv_materials[0]),"_BumpScale",1);
    const char *keywords[]={"_NORMALMAP","_METALLICSPECGLOSSMAP","_OCCLUSIONMAP"};
    for(unsigned i=0;i<3;i++){void *a[]={su.string(keywords[i])};call(su.material_keyword,su_get(lv_materials[0]),a,&ok);}
    return ok;
}
static inline int lv_make(LHVisual *v){
    int ok=1;PTR go=su_gameobject("Little plush sweep helper",0);if(!go)return 0;v->root=su_hold(go);v->transform=su_hold(su_transform(go));
    PTR mesh=su_new("UnityEngine","Mesh");call(su.mesh_ctor,mesh,0,&ok);v->mesh=su_hold(mesh);
    v->vertices=su_hold(su_array(su.vector_class,LH_VERTEX_COUNT,lh_data_positions,12));v->normals=su_hold(su_array(su.vector_class,LH_VERTEX_COUNT,lh_data_normals,12));
    v->tangents=su_hold(su_array(lv_vector4,LH_VERTEX_COUNT,lh_data_tangents,16));
    void *va[]={su_get(v->vertices)},*na[]={su_get(v->normals)},*ta[]={su_get(v->tangents)};
    call(su.mesh_vertices,mesh,va,&ok);call(su.mesh_normals,mesh,na,&ok);call(lv_set_tangents,mesh,ta,&ok);
    PTR uv=su_array(su.uv_class,LH_VERTEX_COUNT,lh_data_uv,8);void *ua[]={uv};call(su.mesh_uv,mesh,ua,&ok);
    unsigned count=sizeof(lh_submeshes)/sizeof(lh_submeshes[0]);su_int(su.mesh_subcount,mesh,(int)count);SURoot materials[32];if(count>32)return 0;
    for(unsigned i=0;i<count;i++){
        const LHSubmesh *s=&lh_submeshes[i];PTR indices=su_array(su.int_class,s->count,lh_data_indices+s->offset*4,4);int index=(int)i;unsigned char bounds=1;
        void *a[]={indices,&index,&bounds};call(su.mesh_indices,mesh,a,&ok);materials[i]=lv_materials[s->material];
    }
    PTR filter=su_add_component(go,"MeshFilter"),renderer=su_add_component(go,"MeshRenderer");void *ma[]={mesh};call(su.meshfilter_set,filter,ma,&ok);lv_assign(renderer,materials,count);
    v->made=ok;v->mode=~0u;v->clip=0;v->blend=1;
    lva_sample(LHV_IDLE,0,v->pose);lva_palette(v->pose,v->last);return ok;
}
static inline void lv_drop(LHVisual *v){
    su_destroy(su_get(v->root));su_destroy(su_get(v->mesh));su_drop(&v->vertices);su_drop(&v->normals);su_drop(&v->tangents);su_drop(&v->transform);su_drop(&v->root);su_drop(&v->mesh);su_zero(v,sizeof(*v));
}
static inline void lv_pose(LHVisual *v,const LHWorker *p,float dt){
    if(!lh_finite(dt)||dt<0)return;if(dt>.1f)dt=.1f;
    SPVec3 delta=su_sub(p->position,v->observed);float travelled=su_sqrt(su_dot(delta,delta));
    if(!v->placed||travelled>1){v->position=p->position;v->heading=p->heading;v->up=p->up;v->placed=1;travelled=0;}
    v->observed=p->position;float speed=dt>.00001f?travelled/dt:0;if(speed>20)speed=20;
    v->speed+=(speed-v->speed)*(dt/(.10f+dt));
    /* Smooth the host's 20Hz contact positions without changing simulation.
     * The short display lag stays on the just-verified movement segments. */
    v->position=su_add(v->position,su_mul(su_sub(p->position,v->position),dt/(.035f+dt)));
    v->up=su_normal(su_add(v->up,su_mul(su_sub(p->up,v->up),dt/(.06f+dt))));
    float facing=su_dot(v->heading,p->heading);SPVec3 side=su_cross(v->up,v->heading);
    if(facing<-.9f){float sign=su_dot(side,p->heading)<0?-1.f:1.f;v->heading=su_normal(su_add(v->heading,su_mul(side,sign*dt*7)));}
    else v->heading=su_normal(su_add(v->heading,su_mul(su_sub(p->heading,v->heading),dt/(.08f+dt))));
    v->greeting=p->mode==LH_CHEER&&p->strokes==0&&p->clock<3.2f;
    unsigned clip=LHV_IDLE;
    if(p->mode==LH_WALK&&v->speed>.035f)clip=v->speed>(v->clip==LHV_RUN?.40f:.46f)?LHV_RUN:LHV_WALK;
    else if(p->mode==LH_LOWER)clip=LHV_BRUSH_READY;
    else if(p->mode==LH_SWEEP)clip=v->speed>.08f?LHV_BRUSH_RUN:LHV_BRUSH_SWEEP;
    else if(p->mode==LH_CHEER)clip=v->greeting?LHV_WAVE:LHV_HAPPY;
    if((p->mode==LH_IDLE||p->mode==LH_BLOCKED)&&facing<.4f)clip=su_dot(side,p->heading)>0?LHV_TURN_LEFT:LHV_TURN_RIGHT;
    /* A greeting never cancels a job or substitutes idle feet while moving.
     * Stationary helpers wave; working helpers acknowledge with happy eyes. */
    int greeted=v->greet_remaining>0;if(greeted&&(p->mode==LH_IDLE||p->mode==LH_BLOCKED||p->mode==LH_CHEER)&&v->speed<.08f)clip=LHV_WAVE;
    if(greeted){v->greet_remaining-=dt;if(v->greet_remaining<0)v->greet_remaining=0;}
    int forced=lv_override_clip>=0&&v==&lv_helpers[0];if(forced)clip=(unsigned)lv_override_clip;
    if(v->clip!=clip||(v->mode!=p->mode&&!forced)){
        su_copy(v->eye_from,v->eye_weights,sizeof(v->eye_from));
        float phase=(v->clip==LHV_WALK||v->clip==LHV_RUN)&&(clip==LHV_WALK||clip==LHV_RUN)?v->sample_time/lh_clips[v->clip].duration:0;
        su_copy(v->from,v->pose,sizeof(v->from));v->blend=v->mode==~0u?1:0;v->sample_time=phase*lh_clips[clip].duration;v->clip=clip;v->mode=p->mode;v->changes++;
        if(!forced&&p->mode==LH_CHEER)v->sample_time=p->clock;
    }
    v->mode=p->mode;
    const LHClip *c=&lh_clips[clip];float rate=1;
    if(!forced&&(clip==LHV_WALK||clip==LHV_RUN||clip==LHV_BRUSH_RUN)&&c->distance>.01f){rate=v->speed*c->duration/c->distance;if(rate<.35f)rate=.35f;if(rate>1.9f)rate=1.9f;}
    v->sample_time+=dt*rate;v->blend+=dt/.18f;if(v->blend>1)v->blend=1;
    LHPose goal[LH_NODE_COUNT];lva_sample(clip,v->sample_time,goal);lva_blend(v->from,goal,v->blend,v->pose);lva_palette(v->pose,v->last);
    float expression[3];lve_sample(clip,v->sample_time,expression);
    if(greeted&&!forced)lve_sample(LHV_HAPPY,v->sample_time,expression);
    for(unsigned k=0;k<3;k++)v->eye_weights[k]=v->eye_from[k]+(expression[k]-v->eye_from[k])*lva_ease(v->blend);
    /* The sound adapter's walk contact clock is expressed in walk periods,
     * including when the displayed gait changes to the faster run cycle. */
    v->time=p->mode==LH_WALK?v->sample_time*lh_clips[LHV_WALK].duration/c->duration:v->sample_time;
    const float *matrix=v->last;v->skin_min=1e20f;v->updates++;
    const SPVec3 *pos=(const SPVec3 *)lh_data_positions,*normal=(const SPVec3 *)lh_data_normals;
    const SUQuat *tangent=(const SUQuat *)lh_data_tangents;const unsigned short *joints=(const unsigned short *)lh_data_joints;const float *weights=(const float *)lh_data_weights;
    SPVec3 *out=su.array_address(su_get(v->vertices),12,0),*ns=su.array_address(su_get(v->normals),12,0);SUQuat *ts=su.array_address(su_get(v->tangents),16,0);
    unsigned morph=0;(void)morph;
    for(unsigned i=0;i<LH_VERTEX_COUNT;i++){
        SPVec3 point=pos[i],surface=normal[i];
#if LH_MORPH_COUNT
        if(morph<LH_MORPH_VERTEX_COUNT&&lh_morph_vertices[morph]==i)lve_deform(morph++,v->eye_weights,(float *)&point,(float *)&surface);
#endif
        SPVec3 value={0},n={0},t={0};
        for(unsigned k=0;k<4;k++){
            float weight=weights[i*4+k];if(weight<=0)continue;const float *m=matrix+joints[i*4+k]*12;
            value.x+=weight*(m[0]*point.x+m[1]*point.y+m[2]*point.z+m[3]);value.y+=weight*(m[4]*point.x+m[5]*point.y+m[6]*point.z+m[7]);value.z+=weight*(m[8]*point.x+m[9]*point.y+m[10]*point.z+m[11]);
            n.x+=weight*(m[0]*surface.x+m[1]*surface.y+m[2]*surface.z);n.y+=weight*(m[4]*surface.x+m[5]*surface.y+m[6]*surface.z);n.z+=weight*(m[8]*surface.x+m[9]*surface.y+m[10]*surface.z);
            t.x+=weight*(m[0]*tangent[i].x+m[1]*tangent[i].y+m[2]*tangent[i].z);t.y+=weight*(m[4]*tangent[i].x+m[5]*tangent[i].y+m[6]*tangent[i].z);t.z+=weight*(m[8]*tangent[i].x+m[9]*tangent[i].y+m[10]*tangent[i].z);
        }
        out[i]=value;if(i<LH_BODY_VERTEX_COUNT&&value.y<v->skin_min)v->skin_min=value.y;ns[i]=su_normal(n);t=su_normal(su_sub(t,su_mul(ns[i],su_dot(t,ns[i]))));ts[i]=(SUQuat){t.x,t.y,t.z,tangent[i].w};
    }
    int ok=1;void *va[]={su_get(v->vertices)},*na[]={su_get(v->normals)},*ta[]={su_get(v->tangents)};
    call(su.mesh_vertices,su_get(v->mesh),va,&ok);call(su.mesh_normals,su_get(v->mesh),na,&ok);call(lv_set_tangents,su_get(v->mesh),ta,&ok);call(su.mesh_bounds,su_get(v->mesh),0,&ok);
    su_set_vector(su.tr_position,su_get(v->transform),su_sub(v->position,su_mul(v->up,SP_HALF_HEIGHT)));su_set_quat(su.tr_rotation,su_get(v->transform),su_rotation(v->heading,v->up));
}
#endif
