// WgpuConfig.h: the compile time switch for the wgpu display backend.
//
// VBA_ENABLE_WGPU is what the "#if" in the places the renderer touches asks
// about: set to 1, the backend is built and the Video menu carries the
// "WebGPU" entry with its backend submenu; set to 0, none of it is compiled
// and the menu items leave the resource with it.
//
// Using the WebGPU renderer means shipping wgpu_native.dll next to the
// executable: the build does not link against the library, it is resolved at
// run time the way D3D11.DLL is, and each platform looks for its own build
// of it (win32\lib\x86, win32\lib\x64 and win32\lib\arm64).  Without the
// DLL the renderer cannot start and the menu entry is greyed out.
//
// The backend itself builds on every platform the project has, x86, x64 and
// ARM64, and nothing else in the tree refers to the switch.

#ifndef VBA_WIN32_WGPUCONFIG_H
#define VBA_WIN32_WGPUCONFIG_H

#pragma once

#ifndef VBA_ENABLE_WGPU
#define VBA_ENABLE_WGPU 0
#endif

// The backends the WebGPU renderer can be told to run on, in the order the
// backend menu lists them and in the order their menu items carry their ids,
// so the item an id stands for is the value it is less the first one, see
// OnOptionsVideoWgpubackend() in MainWndOptions.cpp.
//
// Automatic is what the renderer runs on until the menu says otherwise: the
// adapter request names no backend then, and the library picks - Direct3D 12
// on Windows, Vulkan where there is no Direct3D 12.  The rest are the backends
// wgpu has on a desktop: Metal is the one macOS runs and is listed on Windows
// too, where no adapter answers for it and the entry is greyed out, so the
// menu is the same list everywhere and the port has nothing to add to it.  The
// backends a machine has no adapter for are greyed out, see
// wgpuBackendAvailable() in Wgpu.cpp.
//
// The resource compiler reads this header for the menu, and knows nothing of
// enum declarations, so the enumeration is kept from it.
#ifndef RC_INVOKED
typedef enum VBAWgpuBackend
{
	VBA_WGPU_BACKEND_AUTO = 0,
	VBA_WGPU_BACKEND_D3D12,
	VBA_WGPU_BACKEND_VULKAN,
	VBA_WGPU_BACKEND_METAL,
	VBA_WGPU_BACKEND_OPENGL,
	VBA_WGPU_BACKEND_COUNT
} VBAWgpuBackend;
#endif // RC_INVOKED

#endif // VBA_WIN32_WGPUCONFIG_H
