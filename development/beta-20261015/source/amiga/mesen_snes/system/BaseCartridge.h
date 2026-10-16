#pragma once
#include "pch.h"
#include "SNES/MemoryMappings.h"
#include "SNES/RomHandler.h"
class BaseCartridge {
    uint8_t rom[32768]={};
    vector<unique_ptr<IMemoryHandler>> pages;
public:
    bool Load(const uint8_t *data,uint32_t size){
        // First system fixture: strict ordinary 32KiB LoROM, no SRAM/chips.
        if(!data||size!=32768||data[0x7fd5]!=0x20||data[0x7fd6]!=0||data[0x7fd7]!=5||data[0x7fd8]!=0)return false;
        uint16_t reset=data[0x7ffc]|data[0x7ffd]<<8;if(reset<0x8000)return false;
        memcpy(rom,data,size);
        for(uint32_t i=0;i<size;i+=4096)pages.push_back(unique_ptr<IMemoryHandler>(new RomHandler(rom,i,size,MemoryType::SnesPrgRom)));
        return true;
    }
    void Init(MemoryMappings &m){
        m.RegisterHandler(0,0x7d,0x8000,0xffff,pages);
        m.RegisterHandler(0x80,0xff,0x8000,0xffff,pages);
        m.RegisterHandler(0x40,0x7d,0,0x7fff,pages);
        m.RegisterHandler(0xc0,0xff,0,0x7fff,pages);
    }
    void SyncCoprocessors(){}
    Sa1 *GetSa1(){return nullptr;}
};
