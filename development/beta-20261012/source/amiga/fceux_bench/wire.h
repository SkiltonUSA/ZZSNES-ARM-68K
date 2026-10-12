#pragma once
#include "protocol.h"
#include "layout.h"
#define MB_HOST (ZZ_CONTROL+0x800)
#define MB_REPLY (ZZ_CONTROL+0x900)
#define FC_ROM 0x100000u
#define FC_ROM_LIMIT 1048592u
enum { FC_CLOCK=1,FC_INIT=5,FC_FRAME=6 };
