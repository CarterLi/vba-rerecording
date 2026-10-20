// MFC implementation of the host hooks that common/movie.cpp needs.
//
// This file is the only place where the "hard reset requested by a movie"
// path is allowed to know about the main window. It was moved out of
// movie.cpp verbatim: the behaviour must stay identical, because a .vbm can
// depend on it (a TAS that clears SRAM and restarts mid-movie).

#include "stdafx.h"
#include "VBA.h"
#include "MainWnd.h"
#include "WinMiscUtil.h"

#include "../common/MovieHost.h"

namespace movie_host
{
	bool hardResetAndClearSram()
	{
		MainWnd *wnd = static_cast<MainWnd *>(theApp.m_pMainWnd);
		if (!wnd)
			return false;   // no window yet (startup, batch mode): let the core reset

		// Delete the SRAM file first, so it cannot be resurrected from RAM.
		winEraseBatteryFile();

		// Then reload the ROM, which is a full restart.
		if (!wnd->winFileRun(true))
			wnd->winFileClose();

		return true;
	}
}
