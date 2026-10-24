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
// GDI, which is the one that is on by default; see the chain in
// updateRenderMethod() in VBA.cpp.
//
// It is off by default.  On Windows 8 and later DDRAW.DLL is itself a
// compatibility layer over DXGI, so it fails wherever Direct3D fails - no
// display driver, a remote desktop session, a virtual machine with the basic
// adapter - and pays for the layer wherever it does not.  The source is kept
// and the switch puts it back for a build that wants it.
//
// It only ever runs windowed (DDSCL_NORMAL).  The exclusive full screen mode
// it used to have took the display over, which no longer fits the shell: the
// picture is a child window that is sized and centred inside the main window,
// the main window paints the bars around it, and the menu bar can be shown
// over it.  Full screen is therefore the same borderless window the Direct3D
// 11 and Direct3D 9 renderers use, and the frame is blitted to the screen
// through a clipper that follows the picture child.
//
// The backend builds on every platform the project has, x86, x64 and ARM64,
// and nothing else in the tree refers to the switch.

#ifndef VBA_WIN32_DDRAWCONFIG_H
#define VBA_WIN32_DDRAWCONFIG_H

#pragma once

#ifndef VBA_ENABLE_DDRAW
#define VBA_ENABLE_DDRAW 0
#endif

#endif // VBA_WIN32_DDRAWCONFIG_H
