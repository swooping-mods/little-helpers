#ifndef LITTLE_HELPERS_UNIT_COMMANDS_H
#define LITTLE_HELPERS_UNIT_COMMANDS_H
#include "../unit-runtime/protocol.h"
#include "helper_core.h"
enum {LHC_NOT_READY=6,LHC_CLEAR_SPACE,LHC_PAYMENT,LHC_TOO_FAR,LHC_PRICE_CHANGED};
static inline unsigned lhc_price(const LHCore *c,unsigned action){return action==LH_BUY?LH_PRICE:action==LH_UPGRADE?lh_upgrade_price(c):0;}
static inline unsigned lhc_quote_check(const LHCore *c,unsigned action,unsigned quote){
    if(action!=LH_BUY&&action!=LH_UPGRADE)return LH_INVALID;
    if(!c||c->capacity<1||c->capacity>LH_MAX||c->owned>c->capacity)return LHC_NOT_READY;
    return quote==lhc_price(c,action)?LH_OK:LHC_PRICE_CHANGED;
}
static inline unsigned lhc_request(unsigned char *out,unsigned action,unsigned quote){
    if(action!=LH_BUY&&action!=LH_UPGRADE)return 0;mup_w32(out,2);mup_w32(out+4,action);mup_w32(out+8,quote);return 12;
}
static inline unsigned lhc_action(const unsigned char *b,unsigned n){
    if(!b||!((n==8&&mup_r32(b)==1)||(n==12&&mup_r32(b)==2)))return 0;unsigned action=mup_r32(b+4);return action==LH_BUY||action==LH_UPGRADE?action:0;
}
static inline unsigned lhc_quote(const unsigned char *b,unsigned n){return n==12?mup_r32(b+8):LH_PRICE;}
static inline unsigned lhc_request_quote(const LHCore *c,const unsigned char *b,unsigned n){
    unsigned action=lhc_action(b,n);if(!action||(n==8&&action!=LH_BUY))return LH_INVALID;
    return lhc_quote_check(c,action,lhc_quote(b,n));
}
static inline unsigned lhc_response(unsigned char *b,unsigned code,const LHCore *c){
    unsigned capacity=c&&c->capacity>=1&&c->capacity<=LH_MAX?c->capacity:1,owned=c&&c->owned<=capacity?c->owned:0;
    mup_w32(b,1);mup_w32(b+4,code);mup_w32(b+8,owned);mup_w32(b+12,capacity);return 16;
}
static inline int lhc_response_valid(const unsigned char *b,unsigned n){
    return b&&n==16&&mup_r32(b)==1&&mup_r32(b+4)<=LHC_PRICE_CHANGED&&mup_r32(b+12)>=1&&mup_r32(b+12)<=LH_MAX&&mup_r32(b+8)<=mup_r32(b+12);
}
#endif
