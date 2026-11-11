#ifndef LITTLE_HELPERS_UNIT_REPLICA_H
#define LITTLE_HELPERS_UNIT_REPLICA_H
#include "../unit-runtime/protocol.h"
#define LHR_BYTES (16u+LH_MAX*48u)
typedef struct {LHCore previous,current;float age;unsigned valid;} LHReplica;
static inline void lhr_float(unsigned char *b,float value){unsigned bits;lh_zero(&bits,sizeof(bits));unsigned char *p=(unsigned char*)&value;for(unsigned i=0;i<4;i++)((unsigned char*)&bits)[i]=p[i];mup_w32(b,bits);}
static inline float lhr_read_float(const unsigned char *b){unsigned bits=mup_r32(b);float value;unsigned char *p=(unsigned char*)&value;for(unsigned i=0;i<4;i++)((unsigned char*)&value)[i]=((unsigned char*)&bits)[i];(void)p;return value;}
static inline void lhr_vector(unsigned char *b,SPVec3 p){lhr_float(b,p.x);lhr_float(b+4,p.y);lhr_float(b+8,p.z);}
static inline SPVec3 lhr_read_vector(const unsigned char *b){return lh_v(lhr_read_float(b),lhr_read_float(b+4),lhr_read_float(b+8));}
static inline unsigned lhr_write(const LHCore *c,unsigned char *b,unsigned capacity){
    if(!c||!b||capacity<LHR_BYTES||c->capacity<1||c->capacity>LH_MAX||c->owned>c->capacity)return 0;
    lh_zero(b,LHR_BYTES);mup_w32(b,1);mup_w32(b+4,c->capacity);mup_w32(b+8,c->owned);
    for(unsigned i=0;i<c->owned;i++){
        const LHWorker *p=&c->workers[i];unsigned char *out=b+16+i*48;
        mup_w32(out,p->active);mup_w32(out+4,p->mode);lhr_vector(out+8,p->position);lhr_vector(out+20,p->up);lhr_vector(out+32,p->heading);lhr_float(out+44,p->clock);
    }
    return LHR_BYTES;
}
static inline int lhr_receive(LHReplica *r,const unsigned char *b,unsigned n){
    if(!r||!b||n!=LHR_BYTES||mup_r32(b)!=1||mup_r32(b+12))return 0;
    LHCore pending;lh_init(&pending);pending.capacity=mup_r32(b+4);pending.owned=mup_r32(b+8);
    if(pending.capacity<1||pending.capacity>LH_MAX||pending.owned>pending.capacity)return 0;
    for(unsigned i=0;i<LH_MAX;i++){
        const unsigned char *in=b+16+i*48;LHWorker *p=&pending.workers[i];
        if(i>=pending.owned){for(unsigned j=0;j<48;j++)if(in[j])return 0;continue;}
        p->active=mup_r32(in);p->mode=mup_r32(in+4);p->position=lhr_read_vector(in+8);p->up=lhr_read_vector(in+20);p->heading=lhr_read_vector(in+32);p->clock=lhr_read_float(in+44);
        if(p->active>1||p->mode>LH_CHEER||!lh_valid(p->position)||!lh_valid(p->up)||!lh_valid(p->heading)||!lh_finite(p->clock)||p->clock<0)return 0;
        if(p->active&&(lh_dot(p->up,p->up)<.8f||lh_dot(p->up,p->up)>1.2f||lh_dot(p->heading,p->heading)<.8f||lh_dot(p->heading,p->heading)>1.2f))return 0;
    }
    r->previous=r->valid?r->current:pending;r->current=pending;r->age=0;r->valid=1;return 1;
}
static inline void lhr_present(LHReplica *r,LHCore *out,float dt){
    if(!r->valid||!lh_finite(dt)||dt<0)return;r->age+=dt;if(r->age>3)r->age=3;*out=r->current;
    if(r->age>2){for(unsigned i=0;i<out->owned;i++)out->workers[i].active=0;return;}
    float alpha=r->age/.1f;if(alpha>1)alpha=1;
    for(unsigned i=0;i<out->owned;i++){
        LHWorker *p=&out->workers[i];const LHWorker *old=&r->previous.workers[i];
        if(!old->active||!p->active||lh_distance2(old->position,p->position)>16)continue;
        p->position=lh_add(old->position,lh_mul(lh_sub(p->position,old->position),alpha));
    }
}
#endif
