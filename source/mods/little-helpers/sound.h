#ifndef LITTLE_HELPERS_SOUND_H
#define LITTLE_HELPERS_SOUND_H
#include "../sound-framework/sdk/client.h"
#include "audio/generated/helper_sounds.h"
#include "sound_events.h"
static const MoonSounds *lha_api;
static unsigned lha_cues[4];
static LHASync lha_sync[LH_MAX];
static U64 lha_id(unsigned slot){return 0x4c48415544000000ULL+slot+1;}
static int lha_setup(PTR domain,PTR game){
    lha_api=ma_connect(domain,game);if(!lha_api)return 0;
    static const MoonSoundCue cues[]={
        {MA_ABI,sizeof(MoonSoundCue),"little-helpers.footstep",ms_footstep_pcm,MS_FOOTSTEP_SAMPLES,MS_FOOTSTEP_RATE,.25f,.7f,9,.4f},
        {MA_ABI,sizeof(MoonSoundCue),"little-helpers.brush-lower",ms_brush_lower_pcm,MS_BRUSH_LOWER_SAMPLES,MS_BRUSH_LOWER_RATE,.4f,.7f,9,.5f},
        {MA_ABI,sizeof(MoonSoundCue),"little-helpers.brush-sweep",ms_brush_sweep_pcm,MS_BRUSH_SWEEP_SAMPLES,MS_BRUSH_SWEEP_RATE,.5f,.7f,9,1.2f},
        {MA_ABI,sizeof(MoonSoundCue),"little-helpers.happy",ms_happy_pcm,MS_HAPPY_SAMPLES,MS_HAPPY_RATE,.35f,.7f,10,1.f}
    };
    for(unsigned i=0;i<4;i++){lha_cues[i]=lha_api->register_cue(&cues[i]);if(!lha_cues[i])return 0;}return 1;
}
static void lha_stop(unsigned slot){
    if(lha_api&&lha_sync[slot].valid)lha_api->stop(lha_id(slot));lh_zero(&lha_sync[slot],sizeof(lha_sync[slot]));
}
static void lha_frame(unsigned slot,const LHWorker *p,const LHVisual *v,float dt){
    if(!lha_api)return;float point[]={p->position.x,p->position.y,p->position.z};
    unsigned events=lha_events(&lha_sync[slot],p,v->time,(lh_clips[1].frames-1)/30.f,dt);
    lha_api->move(lha_id(slot),point);
    for(unsigned i=0;i<4;i++)if(events&(1u<<i))lha_api->play(lha_cues[i],lha_id(slot),point,1,1);
}
#endif
