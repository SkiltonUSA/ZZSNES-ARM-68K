/* Match the pinned core clock, including its PAL approximation (50.31974 Hz).
 * Round frame periods up; never run PAL through the NTSC host cadence. */
#pragma once
static unsigned ps_period_us(unsigned pal){return pal?19873u:16689u;}
static unsigned ps_picture_height(unsigned saved){return saved?saved:224u;}
static int ps_height_valid(unsigned h){return h==224u||h==239u;}
