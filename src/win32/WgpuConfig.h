// WgpuConfig.h: the compile time switch for the wgpu display backend.
//
// VBA_ENABLE_WGPU is what the "#if" in the places the renderer touches asks
// about: set to 1, the backend is built and the Video menu carries the
// "WebGPU" entry with its two filter options; set to 0, none of it is
// compiled and the menu items leave the resource with it.
//
// The backend itself builds on every platform the project has, x86, x64 and
// ARM64: nothing is linked against the wgpu library, wgpu_native.dll is
// resolved at run time the way D3D11.DLL is, and each platform looks for its
// own DLL next to the executable.  Drop the matching build of the library in
// when it is there and the menu entry starts up on its own.

#ifndef VBA_WIN32_WGPUCONFIG_H
#define VBA_WIN32_WGPUCONFIG_H

#pragma once

#define VBA_ENABLE_WGPU 1

#endif // VBA_WIN32_WGPUCONFIG_H
