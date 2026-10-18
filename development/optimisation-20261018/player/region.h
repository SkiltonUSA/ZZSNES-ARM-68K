/* GPL-3.0-or-later. Shared frontend and core output cadence.
 * PAL is capped at 50 Hz; NTSC retains the pinned core's near-60 Hz period.
 * Audio sample generation uses this same period, avoiding PAL under-runs. */
#pragma once
#define PS_PAL_PERIOD_US 20000u
#define PS_NTSC_PERIOD_US 16689u
static unsigned ps_period_us(unsigned pal){return pal?PS_PAL_PERIOD_US:PS_NTSC_PERIOD_US;}
static unsigned ps_picture_height(unsigned saved){return saved?saved:224u;}
static int ps_height_valid(unsigned h){return h==224u||h==239u;}
