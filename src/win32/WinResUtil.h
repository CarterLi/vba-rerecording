#ifndef VBA_WIN32_WINRESUTIL_H
#define VBA_WIN32_WINRESUTIL_H

#pragma once

extern HMENU winResLoadMenu(LPCTSTR menuName);
extern int winResDialogBox(LPCTSTR boxName, HWND parent, DLGPROC dlgProc);
extern int winResDialogBox(LPCTSTR boxName, HWND parent, DLGPROC dlgProc, LPARAM lParam);
extern CString winResLoadString(UINT id);
extern CString winResLoadFilter(UINT id);

// The save dialog for an image capture.  It offers every format WIC can write
// (see ImageWrite.h), with the capture format picked in the Options menu
// selected, and the user may save in any of them.  The pick becomes the capture
// format so the extension of the file and what the writer puts in it agree.
// `initialName` may or may not carry the extension yet.  Returns the picked
// path, or an empty string when the user cancelled.
extern CString winImageSaveDlg(CWnd *parent, const CString &initialName,
                               const CString &initialDir, const CString &title);

#endif // VBA_WIN32_WINRESUTIL_H
