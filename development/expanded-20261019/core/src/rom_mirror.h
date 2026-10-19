/* Mirror non-power-of-two cartridges by populated address-line segments.
 * For 6 MiB, banks 6/7 mirror 4/5, rather than wrapping to banks 0/1. */
static unsigned ps_rom_mirror(unsigned address,unsigned size){
 unsigned base=0;
 while(address>=size){unsigned bit=1;while(bit<=address/2)bit<<=1;
  address-=bit;if(size>bit){base+=bit;size-=bit;}}
 return base+address;
}
