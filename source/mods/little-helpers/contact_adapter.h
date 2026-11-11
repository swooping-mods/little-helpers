#ifndef LITTLE_HELPERS_CONTACT_ADAPTER_H
#define LITTLE_HELPERS_CONTACT_ADAPTER_H
static int lhc_contact(SPCore *navigation,unsigned slot,float dt,const SPWorld *world){
    /* The reused pet floor/carry code also rejects peer overlap. Plushes use
     * soft steering, so querying that code with the whole herd can deadlock
     * touching workers before they get a chance to separate. Keep every real
     * world probe, but give it only the current worker for contact refresh. */
    static SPCore contact;
    contact.players[0]=navigation->players[slot];
    sp_carry(&contact,&contact.players[0],world);
    int grounded=sp_settle(&contact,&contact.players[0],dt,world);
    navigation->players[slot]=contact.players[0];
    return grounded;
}
#endif
