#ifndef VBA_GBA_ARMDIS_H
#define VBA_GBA_ARMDIS_H

#pragma once

#define DIS_VIEW_ADDRESS 1
#define DIS_VIEW_CODE 2

int disThumb(u32 offset, char *dest, int flags);
int disArm(u32 offset, char *dest, int flags);

#endif // VBA_GBA_ARMDIS_H
