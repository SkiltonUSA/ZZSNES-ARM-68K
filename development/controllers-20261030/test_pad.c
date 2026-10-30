#include <assert.h>
#include <stdio.h>
#include "pad_decode.h"
#include "../../tests/controllers/hid_fixture.h"
int main(void){
 struct pad_hid h;unsigned char report[]={1,0,0,8,128,128};unsigned result;
 assert(pad_hid_parse(&h,descriptor,sizeof(descriptor)));
 assert(pad_sn30_profile(&h));
 assert(pad_hid_read(&h,report,sizeof(report),&result)&&result==0);
 const unsigned usages[]={2,5,11,12,1,4,7,8};
 const unsigned targets[]={0,1,2,3,8,9,10,11};
 for(unsigned i=0;i<8;i++){
  unsigned b=1u<<(usages[i]-1);report[1]=b;report[2]=b>>8;
  assert(pad_hid_read(&h,report,sizeof(report),&result)&&result==(1u<<targets[i]));
 }
 report[1]=report[2]=0;report[3]=1;
 assert(pad_hid_read(&h,report,sizeof(report),&result)&&result==(16|128));
 report[3]=8;report[4]=0;
 assert(pad_hid_read(&h,report,sizeof(report),&result)&&result==64);
 report[4]=128;report[5]=255;
 assert(pad_hid_read(&h,report,sizeof(report),&result)&&result==32);
 assert(!pad_hid_read(&h,report,4,&result));
 report[0]=2;assert(!pad_hid_read(&h,report,sizeof(report),&result));
 for(unsigned n=0;n<sizeof(descriptor);n++)assert(!pad_hid_parse(&h,descriptor,n));
 assert(pad_hid_parse(&h,descriptor,sizeof(descriptor)));
 h.fields[0].usage=31;assert(!pad_sn30_profile(&h));
 assert(pad_hid_parse(&h,descriptor,sizeof(descriptor)));
 h.fields[0].id=2;assert(!pad_sn30_profile(&h));
 /* Phase samples are active-high buttons: high normal, low A/Start,
    third low signature, fourth high extension, fourth low signature. */
 assert(pad_sega(0,12,15,0,0)==0);
 assert(pad_sega(16,12,15,0,0)==1); /* B -> SNES B */
 assert(pad_sega(32,12,15,0,0)==256); /* C -> A */
 assert(pad_sega(0,28,15,0,0)==2); /* A -> Y */
 assert(pad_sega(0,44,15,0,0)==8); /* Start */
 assert(pad_sega(0,12,15,4,0)==512); /* X -> X */
 assert(pad_sega(0,12,15,2,0)==1024); /* Y -> L */
 assert(pad_sega(0,12,15,1,0)==2048); /* Z -> R */
 assert(pad_sega(0,12,15,8,0)==4); /* Mode -> Select */
 assert(pad_sega(15,12,15,0,0)==(16|32|64|128));
 assert(pad_sega(0,0,15,15,0)==0); /* invalid normal signature */
 assert(pad_sega(16,12,12,15,0)==1); /* 3-button, no extension */
 puts("PASS SN30 HID buttons/hat/axis, short reports, malformed descriptors; Sega phases and all SNES controls");
 return 0;
}
