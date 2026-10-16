// GPL-3.0-or-later. Same-category nesting is free; cross-category time excluded.
#include "profile.h"
#ifdef SS_PROFILE
SsProfile ss_profile;
void ss_profile_reset(uint32_t(*clock)()) { ss_profile={};ss_profile.clock=clock;ss_profile.category=2; }
SsProfileScope::SsProfileScope(unsigned c):previous(ss_profile.category),category(c),start(0),children(0),enabled(ss_profile.active&&ss_profile.clock&&c!=previous){
    if(enabled){ss_profile.calls[c]++;children=ss_profile.ticks[0]+ss_profile.ticks[1];ss_profile.category=c;start=ss_profile.clock();}
}
SsProfileScope::~SsProfileScope(){
    if(enabled){uint32_t elapsed=ss_profile.clock()-start;ss_profile.ticks[category]+=elapsed-(ss_profile.ticks[0]+ss_profile.ticks[1]-children);ss_profile.category=previous;}
}
#endif
