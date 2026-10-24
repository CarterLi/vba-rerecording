#if !defined(VBA_WIN32_DARKMODE_H_INCLUDED)
#define VBA_WIN32_DARKMODE_H_INCLUDED

#pragma once

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

	// True when the application follows the system dark theme.  Controls that
	// paint themselves use the same colours the frame and menu bar use; the
	// dialogs stay on their original light colours either way.
	bool enabled();

	// Give a status bar the colours it needs in dark mode.  The common
	// controls status bar has no dark theme of its own, so the bar is owner
	// drawn; these are the same colours the system uses for dark panels.
	COLORREF panelColor();
	COLORREF panelTextColor();

	// The menu bar is drawn with a font built for the window's DPI; call this
	// when that DPI changes so the next paint picks up the new size.
	void resetFont();
}

#endif // !defined(VBA_WIN32_DARKMODE_H_INCLUDED)
