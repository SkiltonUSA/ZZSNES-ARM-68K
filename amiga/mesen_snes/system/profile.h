// GPL-3.0-or-later. Optional nested stage timing, never changes emulated clocks.
#pragma once
#include <stdint.h>
#ifdef SS_PROFILE
struct SsProfile { uint32_t ticks[2],calls[2]; uint32_t(*clock)(); unsigned category; bool active; };
extern SsProfile ss_profile;
void ss_profile_reset(uint32_t(*clock)());
class SsProfileScope {
    unsigned previous,category; uint32_t start,children; bool enabled;
public:
    explicit SsProfileScope(unsigned c);
    ~SsProfileScope();
};
#define SS_PROFILE_SCOPE(c) SsProfileScope ss_scope(c)
#else
#define SS_PROFILE_SCOPE(c) ((void)0)
#endif
