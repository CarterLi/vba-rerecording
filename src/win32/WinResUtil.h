#ifndef VBA_WIN32_WINRESUTIL_H
#define VBA_WIN32_WINRESUTIL_H

#pragma once

extern HMENU winResLoadMenu(LPCTSTR menuName);
extern int winResDialogBox(LPCTSTR boxName, HWND parent, DLGPROC dlgProc);
extern int winResDialogBox(LPCTSTR boxName, HWND parent, DLGPROC dlgProc, LPARAM lParam);
extern CString winResLoadString(UINT id);
extern CString winResLoadFilter(UINT id);

#endif // VBA_WIN32_WINRESUTIL_H
