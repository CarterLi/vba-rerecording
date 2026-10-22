// GdiConfig.h: the compile time switch for the GDI display backend.
//
// VBA_ENABLE_GDI is what the "#if" in the places the renderer touches asks
// about: set to 1, the backend is built and the Video menu carries the "GDI"
// entry; set to 0, none of it is compiled and the menu item leaves the
// resource with it.
//
// The GDI renderer is the oldest one in the tree and the slowest of them all:
// it blits the frame into a DIB and hands it to StretchDIBits, so the picture
// is scaled by the CPU.  It is the last renderer of the fallback chain, so a
// build without it still falls back through every other one before it gives
// up - see the chain in updateRenderMethod() in VBA.cpp.
//
// Nothing has to be shipped next to the executable for it: GDI is part of
// every Windows and the renderer is linked against gdi32.dll, which is in the
// system directory.
//
// The backend builds on every platform the project has, x86, x64 and ARM64,
// and nothing else in the tree refers to the switch.

#ifndef VBA_WIN32_GDICONFIG_H
#define VBA_WIN32_GDICONFIG_H

#pragma once

#ifndef VBA_ENABLE_GDI
#define VBA_ENABLE_GDI 0
#endif

#endif // VBA_WIN32_GDICONFIG_H
