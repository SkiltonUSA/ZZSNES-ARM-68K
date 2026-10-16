#include "SNES/SnesConsole.h"
#include "SNES/SnesCpu.h"
#include "SNES/SnesPpu.h"
#include "SNES/Spc.h"
#include "SNES/SnesMemoryManager.h"
#include "SNES/InternalRegisters.h"
#include "SNES/SnesDmaController.h"
#include "SNES/SnesControlManager.h"
#include "SNES/BaseCartridge.h"
#include "SNES/RegisterHandlerA.h"
#include "SNES/RegisterHandlerB.h"
SnesConsole::SnesConsole(){}
SnesConsole::~SnesConsole(){}
bool SnesConsole::Load(const uint8_t *data,uint32_t size){
    if(cart)return false; // one cartridge per owned context
    cart.reset(new BaseCartridge());if(!cart->Load(data,size)){cart.reset();return false;}
    regs.reset(new InternalRegisters());memory.reset(new SnesMemoryManager());
    ppu.reset(new SnesPpu(&emu,this));controls.reset(new SnesControlManager(this));
    dma.reset(new SnesDmaController(memory.get()));spc.reset(new Spc(this));
    cpu.reset(new SnesCpu(this));memory->Initialize(this);regs->Initialize(this);
    ppu->PowerOn();cpu->PowerOn();memory->IncMasterClockStartup();return true;
}
int SnesConsole::RunFrame(int(*stop)()){
    if(!cpu)return 3;
    running=true;uint64_t start=GetMasterClock();
    for(unsigned i=0;running;i++){
        if(!(i&63)&&stop&&stop())return 2;
        if(i>=200000||GetMasterClock()-start>500000)return 1;
        cpu->Exec();
    }
    spc->ProcessEndFrame();return 0;
}
void SnesConsole::ProcessEndOfFrame(){spc->ProcessEndFrame();regs->SetAutoJoypadReadClock();running=false;}
uint64_t SnesConsole::GetMasterClock(){return memory->GetMasterClock();}
