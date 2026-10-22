// DDrawConfig.h: the compile time switch for the DirectDraw display backend.
//
// VBA_ENABLE_DDRAW is what the "#if" in the places the renderer touches asks
// about: set to 1, the backend is built and the Video menu carries the
// "DirectDraw" entry with its options; set to 0, none of it is compiled and
// the menu items leave the resource with it.
//
// DirectDraw is the oldest accelerated renderer in the tree.  It runs on
// DDRAW.DLL, which is loaded at run time and is part of every Windows, so
// nothing has to be shipped next to the executable for it.  It is below
// Direct3D in the fallback chain: a build without it falls from Direct3D to
// GDI, or ends the chain when GDI is not built either - see the chain in
// updateRenderMethod() in VBA.cpp.
//
// The backend builds on every platform the project has, x86, x64 and ARM64,
// and nothing else in the tree refers to the switch.

#ifndef VBA_WIN32_DDRAWCONFIG_H
#define VBA_WIN32_DDRAWCONFIG_H

#pragma once

#ifndef VBA_ENABLE_DDRAW
#define VBA_ENABLE_DDRAW 1
#endif

#endif // VBA_WIN32_DDRAWCONFIG_H
