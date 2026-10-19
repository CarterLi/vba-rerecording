#ifndef VBA_GB_H
#define VBA_GB_H

#pragma once

#include "zlib.h"
#include "../Port.h"

typedef union
{
	struct
	{
#ifdef WORDS_BIGENDIAN
		u8 B1, B0;
#else
		u8 B0, B1;
#endif
	} B;
	u16 W;
} gbRegister;

bool gbLoadRom(const char *);
void gbEmulate(int);
void gbWriteMemory(register u16, register u8);
void gbGetHardwareType();
void gbInit();
void gbReset();
void gbCleanUp();
void gbLoadInternalBios();
bool gbWriteBatteryFile(const char *);
bool gbWriteBatteryFile(const char *, bool);
bool gbReadBatteryFile(const char *);
bool gbWriteSaveState(const char *);
bool gbWriteMemSaveState(const char *, int);
bool gbReadSaveState(const char *);
bool gbReadMemSaveState(char *, int);
void gbSgbRenderBorder();
bool gbWriteImageFile(const char *);
bool gbReadGSASnapshot(const char *);

extern struct EmulatedSystem GBSystem;

#endif // VBA_GB_H
