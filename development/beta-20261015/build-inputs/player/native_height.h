/* GPL-3.0-or-later. Experimental unscaled vertical presentation.
 * 224-line core pictures begin at row 8; 239-line pictures at row 0.
 * Keep horizontal geometry and source-coordinate overscan masks unchanged.
 * Selected only by build_pocketsnes_tearing.py, not the normal player. */
#pragma once
static inline int ps_native_y(unsigned y, unsigned opt) {
 unsigned height=(opt&256u)?239u:224u;
 unsigned top=(240u-height)/2u, mask=((opt>>6)&3u)*8u;
 if(y<top || y>=top+height)return -1;
 unsigned source=y-top;
 if(source<mask || source>=height-mask)return -1;
 return (int)(source+(height==224u?8u:0u));
}
