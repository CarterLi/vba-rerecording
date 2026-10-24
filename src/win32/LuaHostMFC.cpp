// ---------------------------------------------------------------------------
// MFC implementation of common/LuaHost.h
//
// This is the only place where the Lua engine's UI needs touch MFC/theApp.
// Moving it out of lua-engine.cpp is what lets that file be compiled into the
// portable core (see documentation/decouple-core-from-mfc.md, step 2).
//
// The wx frontend provides its own implementation; the two must stay
// behaviourally identical, which is why every method below is a straight
// one-to-one replacement for the code it replaced (no "improvements" here).
// ---------------------------------------------------------------------------

#include "stdafx.h"

#include "../common/LuaHost.h"

#include "MainWnd.h"
#include "VBA.h"
#include "AVIWrite.h"
#include "Dialogs/LuaOpenDialog.h"

// Defined in WinMiscUtil.cpp; declared locally on purpose - including that
// header drags in more of the frontend than this file needs.
extern CString winGetSavestateFilename(const CString &LogicalRomName, int nID);

namespace
{
	// The main window may not exist yet when Lua fails very early (e.g. a
	// missing lua51.dll during startup), so every accessor falls back to the
	// raw Win32 call rather than dereferencing a null CWnd.
	CWnd *mainWnd()
	{
		CWinApp *app = AfxGetApp();
		return app ? app->m_pMainWnd : NULL;
	}
}

namespace lua_host
{
	MsgBoxResult messageBox(const char *text, const char *caption, MsgBoxType type,
		bool errorIcon)
	{
		UINT flags;
		switch (type)
		{
		case MsgBoxType::YesNo:
			flags = MB_YESNO;
			break;
		case MsgBoxType::YesNoCancel:
			flags = MB_YESNOCANCEL;
			break;
		case MsgBoxType::AbortRetryIgnore:
			flags = MB_ABORTRETRYIGNORE;
			break;
		case MsgBoxType::Ok:
		default:
			flags = MB_OK;
			break;
		}
		if (errorIcon)
			flags |= MB_ICONERROR;

		int ret;
		CWnd *wnd = mainWnd();
		if (wnd && wnd->GetSafeHwnd())
			ret = wnd->MessageBox(winWideFromAnsi(text), winWideFromAnsi(caption), flags);
		else
			ret = ::MessageBox(NULL, winWideFromAnsi(text), winWideFromAnsi(caption), flags);

		switch (ret)
		{
		case IDYES:
			return MsgBoxResult::Yes;
		case IDNO:
			return MsgBoxResult::No;
		case IDCANCEL:
			return MsgBoxResult::Cancel;
		case IDABORT:
			return MsgBoxResult::Abort;
		case IDRETRY:
			return MsgBoxResult::Retry;
		case IDIGNORE:
			return MsgBoxResult::Ignore;
		case IDOK:
		default:
			return MsgBoxResult::Ok;
		}
	}

	void getMouseClientPos(int &x, int &y)
	{
		POINT pt;
		::GetCursorPos(&pt);

		CWnd *wnd = mainWnd();
		if (wnd && wnd->GetSafeHwnd())
			wnd->ScreenToClient(&pt);

		x = (int)pt.x;
		y = (int)pt.y;
	}

	void getClientSize(int &w, int &h)
	{
		CRect rc(0, 0, 0, 0);

		CWnd *wnd = mainWnd();
		if (wnd && wnd->GetSafeHwnd())
			wnd->GetClientRect(&rc);

		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
	}

	void checkFullscreen()
	{
	}

	int filterWidth()
	{
		return theApp.filterWidth;
	}

	bool aviActive()
	{
		return theApp.aviRecorder != NULL;
	}

	int aviVideoFrames()
	{
		return theApp.aviRecorder ? theApp.aviRecorder->videoFrames() : 0;
	}

	void aviPause(bool pause)
	{
		if (theApp.aviRecorder)
			theApp.aviRecorder->Pause(pause);
	}

	std::string savestateFilename(int which)
	{
		CString name = winGetSavestateFilename(theApp.gameFilename, which);
		return std::string(winAnsiFromWide(name).GetString());
	}

	std::intptr_t openConsole()
	{
		if (!LuaConsoleHWnd)
		{
			CWnd *wnd = mainWnd();
			LuaConsoleHWnd = ::CreateDialog(AfxGetResourceHandle(), MAKEINTRESOURCE(IDD_LUA),
				wnd ? wnd->GetSafeHwnd() : NULL, (DLGPROC)DlgLuaScriptDialog);
		}
		return (std::intptr_t)LuaConsoleHWnd;
	}

	ConsoleSinks consoleSinks()
	{
		ConsoleSinks sinks;
		sinks.print   = PrintToWindowConsole;
		sinks.onStart = WinLuaOnStart;
		sinks.onStop  = WinLuaOnStop;
		return sinks;
	}
}
