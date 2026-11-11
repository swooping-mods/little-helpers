#ifndef LITTLE_HELPERS_SOUND_EVENTS_H
#define LITTLE_HELPERS_SOUND_EVENTS_H
/* Presentation clocks never change jobs or cleaning. Successful host/replica
 * stroke counters gate sweeping, so pending paths and dry brushes stay quiet. */
enum {LHA_STEP=1,LHA_LOWER=2,LHA_SWEEP=4,LHA_HAPPY=8};
typedef struct {unsigned valid,mode,strokes;float time,sweep_age;SPVec3 position;} LHASync;
static inline unsigned lha_events(LHASync *s,const LHWorker *p,float time,float walk_duration,float dt){
    if(!p->active){lh_zero(s,sizeof(*s));return 0;}
    if(!lh_finite(dt)||dt<=0||!lh_finite(time)||time<0||walk_duration<=0)return 0;
    unsigned result=0,entered=!s->valid||s->mode!=p->mode;
    s->sweep_age+=dt;
    if(p->mode==LH_WALK&&s->valid&&lh_distance2(s->position,p->position)>.000001f){
        /* Two padded contacts in the actual displayed walk loop. No catch-up
         * bursts: a delayed frame can emit at most one contact. */
        float interval=walk_duration*.5f;
        if((unsigned)(time/interval)!=(unsigned)((entered?0:s->time)/interval))result|=LHA_STEP;
    }
    if(p->mode==LH_LOWER&&(entered||s->time<.1f)&&time>=.1f)result|=LHA_LOWER;
    if(p->mode==LH_SWEEP&&s->valid&&p->strokes>s->strokes&&s->sweep_age>=1.2f){result|=LHA_SWEEP;s->sweep_age=0;}
    if(p->mode==LH_CHEER&&s->valid&&entered)result|=LHA_HAPPY;
    s->valid=1;s->mode=p->mode;s->strokes=p->strokes;s->time=time;s->position=p->position;
    return result;
}
#endif
