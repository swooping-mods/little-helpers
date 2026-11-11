#ifndef LITTLE_HELPERS_UNIT_PURCHASE_H
#define LITTLE_HELPERS_UNIT_PURCHASE_H
static unsigned purchase_for(PTR walker,PTR world,PTR player,unsigned action,unsigned quote){
    if(!ready||!helper_units->host()||store_failed||!player||helpers.watermark==~(U64)0)return LHC_NOT_READY;int ok=1;
    if(lhc_quote_check(&helpers,action,quote)!=LH_OK)return LHC_PRICE_CHANGED;
    unsigned balance=sw_balance(world,player,&ok);if(!ok)return LHC_PAYMENT;
    SPVec3 point={0},up={0},heading={0};if(action==LH_BUY&&helpers.owned<helpers.capacity&&!spawn_point(walker,helpers.owned,&point,&up,&heading))return LHC_CLEAR_SPACE;
    LHCore pending=helpers;LHResult result=lh_purchase(&pending,action,helpers.watermark+1,balance);
    if(result.code!=LH_OK)return result.code;
    unsigned added=pending.owned>helpers.owned;
    /* Reserve contact admission before payment; a full registry cannot charge
     * for a worker that other providers cannot see. New slots have no marker. */
    if(added&&!helper_units->publish(2,helpers.owned+1,2,(MoonUnitVector){point.x,point.y,point.z},.2f)){event("unit-publish-failed");return LHC_NOT_READY;}
    if(!sw_debit(world,player,result.debit)){if(added)helper_units->remove(2,helpers.owned+1);return LHC_PAYMENT;}
    unsigned previous=helpers.owned;helpers=pending;if(helpers.owned>previous)activate(previous,point,up,heading);dirty=1;save_helpers();call(sw.request_save,world,0,&ok);
    update_prices();event(action==LH_BUY?"helper-bought":"capacity-upgraded");return LH_OK;
}
#endif
