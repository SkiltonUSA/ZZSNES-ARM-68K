#pragma once
#include "pch.h"
class SnesConsole;
// Deterministic two-pad serial adapter. No mouse/multitap or physical input yet.
class SnesControlManager {
    uint16_t buttons[2]={},latched[2]={};
    uint8_t pos[2]={}; bool manual=false,automatic=false;
    void latch(){for(int i=0;i<2;i++){latched[i]=buttons[i];pos[i]=0;}}
public:
    SnesControlManager(SnesConsole*){}
    void SetButtons(unsigned port,uint16_t b){assert(port<2);buttons[port]=b;}
    void SetAutoReadStrobe(bool v){automatic=v;if(v)latch();}
    void SetInputReadFlag(){}
    void Write(uint16_t,uint8_t v){manual=v&1;if(manual)latch();}
    uint8_t Read(uint16_t addr,bool =false){unsigned p=addr&1;if(manual||automatic)latch();
        unsigned n=pos[p];if(n<16)pos[p]++;return n<16?((latched[p]>>(15-n))&1):1;}
};
