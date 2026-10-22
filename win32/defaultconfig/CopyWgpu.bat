@echo off
rem ---------------------------------------------------------------------------
rem Copies wgpu_native.dll next to the emulator, but only for a build that has
rem the WebGPU renderer in it.  The switch is VBA_ENABLE_WGPU in
rem src\win32\WgpuConfig.h; with it off the DLL is not used and is left out.
rem This is part of the post-build step, so every configuration runs it.
rem   %1  the platform (Win32, x64 or ARM64)
rem   %2  the output directory the DLL is copied to
rem ---------------------------------------------------------------------------
setlocal

set "LIBS=%~1"
if /i "%~1"=="Win32" set "LIBS=x86"

rem Read the switch from the header the C++ code and the resource compiler read
rem it from, rather than keeping a second copy of it in the project.
set "ENABLED=0"
for /f "tokens=1,2,3" %%a in ('findstr /c:"VBA_ENABLE_WGPU" "%~dp0..\..\src\win32\WgpuConfig.h" 2^>nul') do (
	if /i "%%a"=="#define" if /i "%%b"=="VBA_ENABLE_WGPU" set "ENABLED=%%c"
)
if not "%ENABLED%"=="1" goto :End

if not exist "%~dp0..\lib\%LIBS%\wgpu_native.dll" goto :End
if not exist "%~2" goto :End

echo Copying wgpu_native.dll for the WebGPU renderer
copy /y "%~dp0..\lib\%LIBS%\wgpu_native.dll" "%~2" >nul

:End
endlocal
