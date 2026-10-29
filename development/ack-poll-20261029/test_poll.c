#include <assert.h>
#include <stdint.h>
#include "poll_interval.h"
int main(void){
 uint32_t last=0;
 assert(ack_poll_due(0,&last,0));assert(ack_poll_due(0,&last,0));
 last=1000;assert(!ack_poll_due(1249,&last,250));assert(last==1000);
 assert(ack_poll_due(1250,&last,250));assert(last==1250);
 assert(!ack_poll_due(1251,&last,250));
 last=UINT32_MAX-100;assert(!ack_poll_due(100,&last,250));assert(ack_poll_due(149,&last,250));
 last=0;unsigned reads=0;for(uint32_t now=0;now<=100000;now++)reads+=ack_poll_due(now,&last,250);
 assert(reads==400&&last==100000); /* deadline still sampled, no cumulative drift */
}
