#ifndef VBA_CORE_OPTIONS_H
#define VBA_CORE_OPTIONS_H

// ---------------------------------------------------------------------------
// Emulator options that used to live on CVBAApp.
//
// movie.cpp needs 18 plain bool / int settings to honour a .vbm header: skip
// BIOS intro, use BIOS file, GB border, save type / flash size, the autofire
// and autohold masks, the frame counter, and what to do when a movie ends.
// All of them were read and written straight through theApp, which is what
// kept the TAS core tied to MFC.
//
// They are emulator state more than frontend state: a movie sets them, the
// core reads them, and the frontend only mirrors them in its menus. So the
// core owns them here, and each frontend binds to the same storage:
//
//   MFC - CVBAApp declares the same names as reference members (win32/VBA.h),
//         so the ~48k lines of `theApp.frameCounter` keep compiling unchanged.
//   wx  - reads and writes g_core directly.
//
// Deliberately a plain aggregate with no user-declared constructors and no
// default member initialisers: it is zero-initialised during static
// initialisation, so a global CVBAApp whose reference members bind to it is
// safe no matter which translation unit gets initialised first. The frontend
// constructor still assigns every field (it always did), it now just assigns
// through a reference.
// ---------------------------------------------------------------------------

struct CoreOptions
{
	bool frameCounter;
	bool frameSearching;
	bool frameSearchSkipping;
	bool inputDisplay;
	bool nextInputDisplay;
	bool lagCounter;
	bool extraCounter;
	bool hideMovieBorder;
	bool movieOnEndPause;
	bool skipBiosIntro;
	bool useBiosFile;

	int autoFire;
	int autoFire2;
	int autoHold;
	int movieOnEndBehavior;
	int winFlashSize;
	int winGbBorderOn;
	int winSaveType;
};

extern CoreOptions g_core;

#endif // VBA_CORE_OPTIONS_H
