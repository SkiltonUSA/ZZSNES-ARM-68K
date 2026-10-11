static int nes_cartridge(const unsigned char *p,unsigned n){unsigned h=2166136261u;if(n!=524288)return -1;for(unsigned i=0;i<n;i++)h=(h^p[i])*16777619u;return h==0xae8466a1u?0:-1;}
