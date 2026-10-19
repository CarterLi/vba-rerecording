#ifndef VBA_SRAM_H
#define VBA_SRAM_H

#pragma once

extern u8 sramRead(u32 address);
extern void sramDelayedWrite(u32 address, u8 byte);
extern void sramWrite(u32 address, u8 byte);

#endif // VBA_SRAM_H
