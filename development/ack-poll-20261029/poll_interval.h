/* GPL-3.0-or-later. Host polling interval; unsigned subtraction handles wrap. */
#ifndef ACK_POLL_INTERVAL_H
#define ACK_POLL_INTERVAL_H
#include <stdint.h>
static inline int ack_poll_due(uint32_t now,uint32_t *last,uint32_t interval){
 if((uint32_t)(now-*last)<interval)return 0;
 *last=now;return 1;
}
#endif
