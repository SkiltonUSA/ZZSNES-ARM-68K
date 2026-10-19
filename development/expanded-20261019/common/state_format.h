#pragma once
#include <stdint.h>
#include <string.h>
#define PS_STATE_MAX 0x200000u
#define PS_HEADER 128u
#define PS_THUMB_W 128u
#define PS_THUMB_H 112u
#define PS_THUMB_BYTES (PS_THUMB_W*PS_THUMB_H*2u)
#define PS_THUMB_OFFSET PS_HEADER
#define PS_PAYLOAD_OFFSET (PS_HEADER+PS_THUMB_BYTES)
static uint32_t ps_get(const uint8_t*p,unsigned n){return (uint32_t)p[n]<<24|(uint32_t)p[n+1]<<16|(uint32_t)p[n+2]<<8|p[n+3];}
static void ps_put(uint8_t*p,unsigned n,uint32_t v){p[n]=v>>24;p[n+1]=v>>16;p[n+2]=v>>8;p[n+3]=v;}
static uint32_t ps_checksum(const uint8_t*p,unsigned n){uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^(i>=28&&i<32?0:p[i]))*16777619u;return h;}
static int ps_valid(const uint8_t*p,unsigned n){return n>=PS_PAYLOAD_OFFSET&&n<=PS_STATE_MAX&&!memcmp(p,"ZZSNES2",8)&&(ps_get(p,8)==2||ps_get(p,8)==3)&&ps_get(p,12)==n&&ps_get(p,32)==128&&ps_get(p,36)==112&&ps_get(p,28)==ps_checksum(p,n);}
