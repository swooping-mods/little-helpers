#ifndef LITTLE_HELPERS_NAVIGATION_H
#define LITTLE_HELPERS_NAVIGATION_H
/* Native movement orchestration; engine queries are supplied by the owning adapter. */
static int move(void *ctx,unsigned slot,SPVec3 goal,float speed,float dt,SPVec3 *point,SPVec3 *up,SPVec3 *heading){
    (void)ctx;SPWorld w=helper_unit_world();SPPet *p=&navigation.players[slot];SPVec3 end,new_up;SPGround g;
    LHRoute *route=&routes[slot];
    SPVec3 requested=goal;goal=su_sub(goal,su_mul(p->up,su_dot(su_sub(goal,p->position),p->up)));
    /* Visibility of the old dirt job must not cancel a checked door crossing.
     * A portal wait or blocked step restarts toward the same far-side goal. */
    unsigned exit_latched=route->exit_route&&lh_distance2(route->job_goal,requested)<=.25f;
    if(exit_latched||!sp_edge(&navigation,p,&w,p->position,goal,&end,&new_up,&g,0)){
        if(!route->state||lh_distance2(route->job_goal,requested)>.25f){
            SPVec3 route_goal=exit_latched?route->goal:requested;
            unsigned exit_route=exit_latched||(!helper_exit_done[slot]&&p->support_id&&ld_exit_goal(p->position,p->up,&route_goal));
            lr_begin(route,p->position,su_normal(su_cross(p->up,p->heading)),p->heading,route_goal);route->job_goal=requested;route->exit_route=exit_route;
            if(test_tools&&helper_route_probe)lhp_zero(&helper_route_probes[slot],sizeof(helper_route_probes[slot]));helper_portal[slot]=0;
        }
        unsigned progressed=0;
        if(route->state==LR_SEARCHING){
            /* One selected worker shares four bounded expansions per tick.
             * Door-guided heuristics avoid scanning the whole room toward dirt
             * behind another wall; active searches report their real progress. */
            if(helper_sim_tick%helpers.owned==slot){
                LREdgeContext edge={p,w};unsigned before=route->expanded;lr_step(route,route_edge,&edge,LR_SHARED_EXPANSIONS);progressed=route->expanded>before;
                if(route->state==LR_SEARCHING){
                    /* Radial scans may nominate dirt behind a station. Prefer
                     * real dirty terrain encountered on a verified corridor. */
                    SPVec3 reached=route->nodes[route->last].point;SPGround ground;
                    if(sw_ground(&sw,su_add(reached,su_mul(p->up,.1f)),p->up,SP_HALF_HEIGHT+.2f,&ground)&&!ground.support_id&&dust(0,ground.point)>.002f&&!lh_reserved(&helpers,slot,ground.point)){
                        helpers.workers[slot].target=ground.point;route->job_goal=route->goal=ground.point;route->exit_route=0;lr_accept(route,route->last,reached);helper_portal[slot]=0;
                        if(test_tools)event("reachable-dirt-job");
                    }else helper_portal[slot]=ld_route_portal(route,route_edge,&edge,p->up);
                }
            }
            if(route->state==LR_SEARCHING)return progressed?LH_MOVE_PROGRESS:LH_MOVE_PENDING;
        }
        if(route->state!=LR_READY)return LH_MOVE_BLOCKED;
        goal=lr_waypoint(route,p->position);
        if(route->exit_route&&!helper_portal[slot]&&route->index==route->count&&lh_distance2(goal,p->position)<.18f*.18f){
            helper_exit_done[slot]=1;route->state=route->exit_route=0;
            /* The pre-exit radial job may point behind the platform. Release
             * its reservation only here, after contact-verified arrival, and
             * use the existing bounded backoff before scanning this new floor. */
            lh_avoid(&helpers.workers[slot]);helpers.workers[slot].seek_timer=0;
            return LH_MOVE_PROGRESS;
        }
        if(helper_portal[slot]&&lh_distance2(goal,p->position)<.18f*.18f&&route->index==route->count){
            if(helper_doors[helper_portal[slot]-1].amount>.85f){route->state=0;helper_portal[slot]=0;}
            return LH_MOVE_PENDING;
        }
    }else {route->state=route->exit_route=0;helper_portal[slot]=0;}
    SPVec3 delta=sp_tangent(sp_sub(goal,p->position),p->up);float distance=sp_length(sp_sub(goal,p->position));if(distance>speed*dt)distance=speed*dt;
    if(distance<.0001f)return 1;SPVec3 steered=su_normal(lh_steer(&helpers,slot,delta)),desired=su_add(p->position,su_mul(steered,distance));
    if(!sp_edge(&navigation,p,&w,p->position,desired,&end,&new_up,&g,0)){
        desired=su_add(p->position,su_mul(delta,distance));
        if(!sp_edge(&navigation,p,&w,p->position,desired,&end,&new_up,&g,0)){routes[slot].state=0;return 0;}steered=delta;
    }
    p->position=end;p->up=new_up;p->heading=sp_tangent(steered,new_up);sp_contact(p,&w,&g);*point=p->position;*up=p->up;*heading=p->heading;return 1;
}
#endif
