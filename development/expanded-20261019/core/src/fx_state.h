/* GSU continuation state including the instruction pipeline and cache metadata. */
struct ps_fx_state { uint32 regs[16], scalars[24],source,dest; uint8 cache[512]; };
unsigned ps_fx_state(int load, unsigned char *data){
 struct ps_fx_state image; 
 if(!data)return sizeof(image);
 if(!load){memset(&image,0,sizeof(image));memcpy(image.regs,GSU.avReg,sizeof(image.regs));
 image.source=GSU.pvSreg-GSU.avReg;image.dest=GSU.pvDreg-GSU.avReg;
 memcpy(image.cache,GSU.avCacheBackup,512);
 image.scalars[0]=GSU.vColorReg;
 image.scalars[1]=GSU.vPlotOptionReg;
 image.scalars[2]=GSU.vStatusReg;
 image.scalars[3]=GSU.vPrgBankReg;
 image.scalars[4]=GSU.vRomBankReg;
 image.scalars[5]=GSU.vRamBankReg;
 image.scalars[6]=GSU.vCacheBaseReg;
 image.scalars[7]=GSU.vCacheFlags;
 image.scalars[8]=GSU.vLastRamAdr;
 image.scalars[9]=GSU.vRomBuffer;
 image.scalars[10]=GSU.vPipe;
 image.scalars[11]=GSU.vPipeAdr;
 image.scalars[12]=GSU.vSign;
 image.scalars[13]=GSU.vZero;
 image.scalars[14]=GSU.vCarry;
 image.scalars[15]=GSU.vOverflow;
 image.scalars[16]=GSU.vErrorCode;
 image.scalars[17]=GSU.vIllegalAddress;
 image.scalars[18]=GSU.bBreakPoint;
 image.scalars[19]=GSU.vBreakPoint;
 image.scalars[20]=GSU.vStepPoint;
 image.scalars[21]=GSU.bCacheActive;
 image.scalars[22]=GSU.vCounter;
 image.scalars[23]=GSU.vInstCount;
 memcpy(data,&image,sizeof(image));return sizeof(image);}
 memcpy(&image,data,sizeof(image));
 if(image.source>15||image.dest>15||image.scalars[3]>255||image.scalars[4]>255||image.scalars[5]>=GSU.nRamBanks)return 0;
 /* ROM/RAM tables belong to the freshly loaded cartridge. The register RAM
  * has already been restored by the core snapshot. Rebuild derived pointers. */
 GSU.vPlotOptionReg=image.scalars[1];GSU.vSCBRDirty=1;fx_readRegisterSpace();
 memcpy(GSU.avReg,image.regs,sizeof(image.regs));
 GSU.vColorReg=image.scalars[0];
 GSU.vPlotOptionReg=image.scalars[1];
 GSU.vStatusReg=image.scalars[2];
 GSU.vPrgBankReg=image.scalars[3];
 GSU.vRomBankReg=image.scalars[4];
 GSU.vRamBankReg=image.scalars[5];
 GSU.vCacheBaseReg=image.scalars[6];
 GSU.vCacheFlags=image.scalars[7];
 GSU.vLastRamAdr=image.scalars[8];
 GSU.vRomBuffer=image.scalars[9];
 GSU.vPipe=image.scalars[10];
 GSU.vPipeAdr=image.scalars[11];
 GSU.vSign=image.scalars[12];
 GSU.vZero=image.scalars[13];
 GSU.vCarry=image.scalars[14];
 GSU.vOverflow=image.scalars[15];
 GSU.vErrorCode=image.scalars[16];
 GSU.vIllegalAddress=image.scalars[17];
 GSU.bBreakPoint=image.scalars[18];
 GSU.vBreakPoint=image.scalars[19];
 GSU.vStepPoint=image.scalars[20];
 GSU.bCacheActive=image.scalars[21];
 GSU.vCounter=image.scalars[22];
 GSU.vInstCount=image.scalars[23];
 GSU.pvSreg=GSU.avReg+image.source;GSU.pvDreg=GSU.avReg+image.dest;
 GSU.pvRamBank=GSU.apvRamBank[GSU.vRamBankReg];GSU.pvRomBank=GSU.apvRomBank[GSU.vRomBankReg];GSU.pvPrgBank=GSU.apvRomBank[GSU.vPrgBankReg];
 memcpy(GSU.avCacheBackup,image.cache,512);return sizeof(image);
}
