#include <assert.h>
#include <stdint.h>
typedef uint32_t ULONG;
typedef int32_t LONG;
#include "pace.h"
int main(void){
 assert(fc_next_deadline(100000,0)==116639);
 assert(fc_next_deadline(118639,116639)==133278); /* 2ms overshoot recovered */
 assert(fc_next_deadline(200000,116639)==216639); /* long stall resets */
 assert(fc_next_deadline(120000,0)==136639); /* pause resets */
 assert(fc_next_deadline(0xfffffff0u,0xffffffe0u)==0x40dfu);
 return 0;
}
