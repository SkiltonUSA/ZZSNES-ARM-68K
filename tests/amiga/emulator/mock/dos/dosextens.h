#pragma once
#include <stdint.h>
typedef uintptr_t BPTR;
typedef unsigned char UBYTE;
struct FileInfoBlock {int fib_DirEntryType;char fib_FileName[64];unsigned cursor;};
struct DosList {struct DosList*next;void*dol_Task;BPTR dol_Lock,dol_Name;};
#define BADDR(x) ((void*)(x))
#define LDF_VOLUMES 1
#define LDF_READ 2
#define ACCESS_READ 0
#define DOS_FIB 0
#define ST_FILE -3
#define ST_USERDIR 2
