#ifndef VBA_MOVIE_HOST_H
#define VBA_MOVIE_HOST_H

// ---------------------------------------------------------------------------
// Host services required by common/movie.cpp
//
// movie.cpp used to include ../win32/{stdafx,MainWnd,VBA,WinMiscUtil}.h for a
// single piece of logic: a .vbm can request a hard reset, and on Windows the
// frontend implements that as "erase the battery file, then reload the ROM"
// (winEraseBatteryFile + MainWnd::winFileRun). Doing it in that order is what
// keeps a freshly deleted SRAM file from being written back out of RAM, so the
// core must not simply call emuReset() instead.
//
// That is a frontend responsibility, so it goes through this hook and common/
// no longer needs a single Windows or MFC header.
//
//   src/win32/MovieHostMFC.cpp      MFC frontend (this fork's shipping build)
//   frontend-impl/MovieHostWx.cpp   wxWidgets frontend (migration target)
// ---------------------------------------------------------------------------

namespace movie_host
{
	// Returns true when the frontend restarted the game itself.
	//
	// Returning false means "I cannot do that here" and lets the core fall back
	// to the portable path (drop the .sav by name, then emuReset), which is all
	// a headless build can do.
	bool hardResetAndClearSram();
}

#endif // VBA_MOVIE_HOST_H
