/* GPL-3.0-or-later. NES lowlevel.library route labels, no pin/protocol changes. */
static Object *input_route;
static const char *input_routes[]={"Amiga / CD32 gameport","CD32 USB (Poseidon)","Sega Mega Drive (adapter)",NULL};
static unsigned input_route_load(void){
 unsigned n=0;FILE*f=fopen("PROGDIR:InputRoute.cfg","r");
 if(f){if(fscanf(f,"ZZNESINPUT1 %u",&n)!=1||n>2)n=0;fclose(f);}return n;
}
static int input_route_save(unsigned n){
 if(n>2)return 0;
 FILE*f=fopen("PROGDIR:InputRoute.cfg","w");if(!f)return 0;
 int ok=fprintf(f,"ZZNESINPUT1 %u\n",n)>0;if(fclose(f))ok=0;return ok;
}
