#pragma once
/* Existing low six bits retain their 0.6.4 meaning. Higher fields are optional;
 * zero encodes the old defaults, keeping existing version-3 states readable. */
#define PS_AUTO_SHIFT 6
#define PS_NO_TRANSPARENCY 256u
#define PS_FILTER_SHIFT 9
#define PS_THRESHOLD_SHIFT 16
static unsigned ps_filter_strength(unsigned v){unsigned n=(v>>9)&127;return n?n-1:60;}
static unsigned ps_skip_threshold(unsigned v){unsigned n=(v>>16)&127;return n?n-1:33;}
static unsigned ps_skip_mode(unsigned v){unsigned n=(v>>6)&3;return n?n+3:v&3;}
static int ps_options_valid(unsigned v){return !(v&~0x7fffffu)&&((v>>3)&3u)<3&&((v>>6)&3u)<3&&((v>>9)&127)<=96&&((v>>16)&127)<=101;}
