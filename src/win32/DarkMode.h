#if !defined(VBA_WIN32_DARKMODE_H_INCLUDED)
#define VBA_WIN32_DARKMODE_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

// Follows the Windows "app mode" (Settings > Personalization > Colors) for
// the emulator's own windows: the title bar and the menu bar go dark, the
// dialogs keep their original light colours.
//
// Everything is looked up at run time, and a WINXP build keeps the classic
// look entirely.
namespace darkMode
{
	// Call once from InitInstance, before the first window is created.
	void init();
}

#endif // !defined(VBA_WIN32_DARKMODE_H_INCLUDED)
