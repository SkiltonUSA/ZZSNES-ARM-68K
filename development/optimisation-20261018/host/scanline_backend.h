/* GPL-3.0-or-later. Xanxi FPGA in fullscreen, Soft ARM effect in windows. */
#ifndef EM_SCANLINE_BACKEND_H
#define EM_SCANLINE_BACKEND_H
static unsigned em_scan_normalize(unsigned mode){return mode==4?2:mode;}
static unsigned em_scan_software(unsigned windowed,unsigned mode){return windowed&&mode?2:0;}
static unsigned em_scan_hardware(unsigned windowed,unsigned mode){return windowed?0:em_scan_normalize(mode);}
#endif
