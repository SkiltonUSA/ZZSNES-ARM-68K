// GPL-3.0-or-later. Memory mapping adapter for unmodified Mesen mapper logic.
#pragma once
class BaseMapper {
protected:
 Console *_console=nullptr;
 uint32_t _prgSize=0,_saveRamSize=0,_workRamSize=0;
 struct {unsigned MapperID=0,SubMapperID=0;} _romInfo;
 const uint8_t *prg=nullptr,*chr=nullptr;uint32_t chrSize=0;
 uint8_t chrRam[8192]{},nt[4096]{},ram[32768]{};
 uint32_t prgMap[8]{},chrMap[8]{},ramBank=0;uint8_t ramAccess=3;
 bool ramIsRom=false,registerReads=false;uint16_t readRegisters=0xff00,writeRegisters=0xff00;
 const uint8_t *_chrRom=nullptr;uint32_t _chrRomSize=0;
 MirroringType mirror=MirroringType::Horizontal;bool battery=false;
 virtual uint16_t GetPRGPageSize()=0;virtual uint16_t GetCHRPageSize()=0;
 virtual uint32_t GetSaveRamSize(){return 8192;}virtual uint32_t GetSaveRamPageSize(){return 8192;}
 virtual void InitMapper()=0;virtual void WriteRegister(uint16_t,uint8_t){}
 virtual bool AllowRegisterRead(){return false;}
 virtual uint8_t ReadRegister(uint16_t){return 0;}
 // The supported boards register whole 4 KiB CPU ranges. Reject finer ones.
 void AddRegisterRange(uint16_t first,uint16_t last,MemoryOperation op=MemoryOperation::Any){
  if((first&4095)||(last&4095)!=4095||first>last)__builtin_trap();
  for(unsigned p=first>>12;p<=last>>12;p++){if((int)op&1)readRegisters|=1u<<p;if((int)op&2)writeRegisters|=1u<<p;}
 }
 void RemoveRegisterRange(uint16_t first,uint16_t last,MemoryOperation op=MemoryOperation::Any){
  if((first&4095)||(last&4095)!=4095||first>last)__builtin_trap();
  for(unsigned p=first>>12;p<=last>>12;p++){if((int)op&1)readRegisters&=~(1u<<p);if((int)op&2)writeRegisters&=~(1u<<p);}
 }
 virtual bool HasBusConflicts(){return false;}
 uint8_t GetPowerOnByte(uint8_t v=0){return v;}
 bool HasBattery(){return battery;}
 MirroringType GetMirroringType(){return mirror;}
 void SetMirroringType(MirroringType v){mirror=v;}
 void SelectPRGPage(unsigned slot,int page){
  unsigned size=GetPRGPageSize(),count=((_prgSize+size-1)>>__builtin_ctz(size));
  unsigned bank=(page<0?(int)count+page:page)&(count-1);
  for(unsigned i=0;i<size/4096;i++)prgMap[slot*(size/4096)+i]=(bank*size+i*4096)&(_prgSize-1);
 }
 void SelectPrgPage2x(unsigned slot,int page){SelectPRGPage(slot*2,page);SelectPRGPage(slot*2+1,page+1);}
 void SelectPrgPage4x(unsigned slot,int page){SelectPrgPage2x(slot*2,page);SelectPrgPage2x(slot*2+1,page+2);}
 void SelectCHRPage(unsigned slot,int page){
  unsigned size=GetCHRPageSize(),count=((chrSize?chrSize:8192)>>__builtin_ctz(size));
  unsigned bank=(page<0?(int)count+page:page)&(count-1);
  for(unsigned i=0;i<size/1024;i++)chrMap[slot*(size/1024)+i]=bank*size+i*1024;
 }
 void SelectChrPage2x(unsigned slot,int page){SelectCHRPage(slot*2,page);SelectCHRPage(slot*2+1,page+1);}
 void SelectChrPage4x(unsigned slot,int page){SelectChrPage2x(slot*2,page);SelectChrPage2x(slot*2+1,page+2);}
 void SelectChrPage8x(unsigned slot,int page){SelectChrPage4x(slot*2,page);SelectChrPage4x(slot*2+1,page+4);}
 void SetNametable(unsigned,unsigned){__builtin_trap();} // Mapper 209/211 are not accepted.
 void RemoveCpuMemoryMapping(uint16_t first,uint16_t last){
  if(first!=0x6000||last!=0x7fff)__builtin_trap();ramAccess=0;
 }
 void RemovePpuMemoryMapping(uint16_t,uint16_t){__builtin_trap();} // unsupported mapper185 never constructed
 void SetCpuMemoryMapping(uint16_t first,uint16_t last,int page,PrgMemoryType type,int access=MemoryAccessType::Unspecified){
  if(first!=0x6000||last!=0x7fff)__builtin_trap(); // NES2/MMC6 rejected by loader
  ramBank=(unsigned)page*8192;ramIsRom=type==PrgMemoryType::PrgRom;
  ramAccess=access==MemoryAccessType::Unspecified?(ramIsRom?1:3):access;
 }
 unsigned ntIndex(unsigned a){unsigned table=(a>>10)&3;switch(mirror){case MirroringType::Vertical:table&=1;break;case MirroringType::Horizontal:table>>=1;break;case MirroringType::ScreenAOnly:table=0;break;case MirroringType::ScreenBOnly:table=1;break;default:break;}return table*1024+(a&1023);}
public:
 void Setup(Console *c,const uint8_t *r,unsigned id){
  _console=c;_romInfo.MapperID=id;_prgSize=r[4]*16384u;chrSize=r[5]*8192u;prg=r+16;chr=prg+_prgSize;_chrRom=chr;_chrRomSize=chrSize;registerReads=AllowRegisterRead();
  battery=r[6]&2;unsigned n=(r[8]?r[8]:1)*8192u;_saveRamSize=battery?n:0;_workRamSize=battery?0:n;
  mirror=(r[6]&8)?MirroringType::FourScreens:(r[6]&1)?MirroringType::Vertical:MirroringType::Horizontal;InitMapper();
 }
 uint8_t ReadCPU(uint16_t a,uint8_t bus){if(registerReads&&(readRegisters&(1u<<(a>>12))))return ReadRegister(a);if(a>=0x8000)return prg[prgMap[(a-0x8000)>>12]+(a&4095)];if(a>=0x6000&&(ramAccess&1))return ramIsRom?prg[(ramBank+(a&8191))&(_prgSize-1)]:ram[(ramBank+(a&8191))&(_saveRamSize+_workRamSize-1)];return bus;}
 void WriteCPU(uint16_t a,uint8_t v){if(writeRegisters&(1u<<(a>>12))){if(HasBusConflicts())v&=ReadCPU(a,0);WriteRegister(a,v);}else if(a>=0x6000&&!ramIsRom&&(ramAccess&2))ram[(ramBank+(a&8191))&(_saveRamSize+_workRamSize-1)]=v;}
 virtual uint8_t MapperReadVRAM(uint16_t a,MemoryOperationType){a&=0x3fff;if(a>=8192)return nt[ntIndex(a)];unsigned off=chrMap[a>>10]+(a&1023);return chrSize?chr[off]:chrRam[off];}
 uint8_t ReadVRAM(uint16_t a,MemoryOperationType type=MemoryOperationType::PpuRenderingRead){return MapperReadVRAM(a,type);}
 void WriteVRAM(uint16_t a,uint8_t v){a&=0x3fff;if(a>=8192)nt[ntIndex(a)]=v;else if(!chrSize)chrRam[chrMap[a>>10]+(a&1023)]=v;}
 uint8_t DebugReadVRAM(uint16_t a){return BaseMapper::MapperReadVRAM(a,MemoryOperationType::Read);}int ToAbsoluteChrAddress(uint16_t a){return chrMap[(a&8191)>>10]+(a&1023);}
 virtual void ProcessCpuClock(){}
 virtual void NotifyVRAMAddressChange(uint16_t){}
 unsigned SaveSize(){return _saveRamSize;}uint8_t *SaveData(){return ram;}
};
