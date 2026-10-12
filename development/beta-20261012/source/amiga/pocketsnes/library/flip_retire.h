/* GPL-3.0-or-later. Experimental four-slot plan for XX19c.
 * Retain the previous logical front for one additional PAN acknowledgement.
 * Firmware ACK updates software state; this is NOT a physical scanout fence.
 * The extra slot allows overlap without immediately reusing that old front.
 */
#pragma once
#include <stdint.h>
struct nf_slots { unsigned front,render; int pending; uint16_t ack; uint32_t flips; unsigned retired; };
static inline void nf_init(struct nf_slots *s){s->front=0;s->render=1;s->pending=-1;s->ack=0;s->flips=0;s->retired=3;}
static inline int nf_queue(struct nf_slots *s,uint16_t ack){
 unsigned next;
 if(s->pending>=0||s->front>3||s->render>3||s->retired>3||
    s->front==s->render||s->front==s->retired||s->render==s->retired)return 0;
 for(next=0;next<4;next++)if(next!=s->front&&next!=s->render&&next!=s->retired)break;
 if(next==4)return 0;
 s->pending=(int)s->render;s->render=next;s->ack=ack;return 1;
}
static inline int nf_ack(struct nf_slots *s,uint16_t ack){
 if(s->pending<0)return -1;
 if(ack==s->ack)return 0;
 if((uint16_t)(ack-s->ack)!=1)return -1;
 s->retired=s->front;s->front=(unsigned)s->pending;s->pending=-1;s->flips++;return 1;
}
