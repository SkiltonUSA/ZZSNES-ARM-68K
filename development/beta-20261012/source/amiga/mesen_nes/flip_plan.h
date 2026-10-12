/* GPL-3.0-or-later. Three slots: front, pending scanout, next render.
 * The 16-bit firmware ACK is the authority for recycling the old front. */
#pragma once
#include <stdint.h>
struct nf_slots { unsigned front,render; int pending; uint16_t ack; uint32_t flips; };
static inline void nf_init(struct nf_slots *s){s->front=0;s->render=1;s->pending=-1;s->ack=0;s->flips=0;}
static inline int nf_queue(struct nf_slots *s,uint16_t ack){
 if(s->pending>=0 || s->front>2 || s->render>2 || s->front==s->render)return 0;
 s->pending=(int)s->render;s->render=3-s->front-(unsigned)s->pending;s->ack=ack;return 1;
}
/* 0 pending; 1 committed; -1 unexpected counter change / invalid state. */
static inline int nf_ack(struct nf_slots *s,uint16_t ack){
 if(s->pending<0)return -1;
 if(ack==s->ack)return 0;
 if((uint16_t)(ack-s->ack)!=1)return -1;
 s->front=(unsigned)s->pending;s->pending=-1;s->flips++;return 1;
}
