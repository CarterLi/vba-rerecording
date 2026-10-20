#ifndef VBA_LUA_HOST_H
#define VBA_LUA_HOST_H

// ---------------------------------------------------------------------------
// Host services required by common/lua-engine.cpp
//
// lua-engine.cpp used to include ../win32/{stdafx,MainWnd,VBA}.h and reach into
// theApp / AfxGetApp() directly, which pinned the entire Lua engine to MFC.
// Everything it actually needed is a UI concern, so it goes through this small
// interface instead. Each frontend provides exactly one implementation:
//
//   src/win32/LuaHostMFC.cpp        MFC frontend (this fork's shipping build)
//   frontend-impl/LuaHostWx.cpp     wxWidgets frontend (migration target)
//
// Nothing here owns emulator state: these are pure queries plus a few UI
// actions. No Windows or MFC types cross this boundary.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <string>

namespace lua_host
{
	// ---- message boxes ----------------------------------------------------
	// The Lua API exposes "ok" / "yesno" / "yesnocancel" pop-ups plus the
	// abort/retry/ignore prompt used by the runaway-script guard. These are
	// modelled with our own enums: neither the core nor the wx frontend should
	// have to include windows.h just to ask a question.
	enum class MsgBoxType
	{
		Ok,
		YesNo,
		YesNoCancel,
		AbortRetryIgnore
	};

	enum class MsgBoxResult
	{
		Ok,
		Yes,
		No,
		Cancel,
		Abort,
		Retry,
		Ignore
	};

	MsgBoxResult messageBox(const char *text, const char *caption, MsgBoxType type,
		bool errorIcon = false);

	// ---- window geometry, in physical client pixels -----------------------
	void getMouseClientPos(int &x, int &y);
	void getClientSize(int &w, int &h);

	// ---- main window ------------------------------------------------------
	void checkFullscreen();
	int  filterWidth(); // width of the filtered frame the frontend renders

	// ---- AVI recording (optional; report "absent" when unsupported) --------
	bool aviActive();
	int  aviVideoFrames();
	void aviPause(bool pause);

	// ---- savestates -------------------------------------------------------
	// The frontend owns the naming scheme (ROM directory, "state1.sgm", ...).
	std::string savestateFilename(int which);

	// ---- Lua console (modeless; return 0 when the frontend has none) ------
	//
	// The engine owns the function pointers (info_print / info_onstart /
	// info_onstop in lua-engine.cpp); the frontend supplies the three sinks
	// that write into its console window. A frontend without a console
	// returns 0 from openConsole() and leaves every sink null.
	struct ConsoleSinks
	{
		void (*print)(std::intptr_t handle, const char *text);
		void (*onStart)(std::intptr_t handle);
		void (*onStop)(std::intptr_t handle);
	};

	std::intptr_t openConsole();
	ConsoleSinks consoleSinks();
}

#endif // VBA_LUA_HOST_H
