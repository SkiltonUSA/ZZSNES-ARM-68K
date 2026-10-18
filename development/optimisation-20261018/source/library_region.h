/* GPL-3.0-or-later. SNES destination byte, following header validation. */
#pragma once
static const char *snes_region_name(unsigned code){
 static const char *names[]={"Japan","USA/Canada","Europe","Sweden","Finland","Denmark","France","Netherlands","Spain","Germany","Italy","China/Hong Kong","Indonesia"};
 return code<sizeof(names)/sizeof(names[0])?names[code]:"Unknown";
}
static void snes_region_label(char *out,unsigned size,unsigned code){
 snprintf(out,size,"%s: %s (0x%02X)\n%s fps target",code>=2?"PAL":"NTSC",snes_region_name(code),code,code>=2?"50":"59.92");
}
