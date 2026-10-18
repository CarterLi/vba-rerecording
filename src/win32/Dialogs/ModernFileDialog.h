#if !defined(VBA_WIN32_MODERNFILEDIALOG_H_INCLUDED)
#define VBA_WIN32_MODERNFILEDIALOG_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <string>

// The file and folder dialogs of Windows Vista and later.
//
// The shell dialogs (IFileOpenDialog / IFileSaveDialog / FOS_PICKFOLDERS) are
// the ones every other application shows since Windows Vista; the legacy
// GetOpenFileName family is only kept as a fall-back for hosts where the
// shell dialogs cannot be created at all.

struct WinFileDialogParams
{
	WinFileDialogParams()
		: owner(NULL), save(false), mustExist(true), hideReadOnly(true),
		  title(NULL), initialDir(NULL), fileName(NULL), filter(NULL),
		  filterIndex(1), defaultExt(NULL), extensions(NULL)
	{
	}

	HWND        owner;         // window the dialog is centered on, may be NULL
	bool        save;          // save dialog instead of an open dialog
	bool        mustExist;     // open dialogs: the picked file has to exist
	bool        hideReadOnly;  // legacy dialogs only: no read-only check box
	const char *title;         // may be NULL
	const char *initialDir;    // may be NULL
	const char *fileName;      // initial name, may be NULL or empty
	const char *filter;        // "Description|pattern|Description|pattern|"
	int         filterIndex;   // 1 based, as OPENFILENAME.nFilterIndex
	const char *defaultExt;    // without the leading dot, may be NULL
	const char *const *extensions; // see below
};

// Shows the dialog and waits for the user.  Returns IDOK when a file was
// picked (path then holds it, filterIndex the selected filter) and IDCANCEL
// when the user cancelled.
//
// filterIndex may be NULL.  extensions, when given, is a NULL terminated
// array of the extensions matching the entries of filter (".sgm" style); the
// save dialog keeps the extension of an already typed name in sync with the
// selected file type, the way the legacy common dialog did.
int winFileDialog(const WinFileDialogParams &params, std::string &path,
                  int *filterIndex = NULL);

// Shows the folder picker.  Returns false when the user cancelled.
bool winBrowseForFolder(HWND owner, const char *title, const char *initialDir,
                        std::string &path);

#endif // !defined(VBA_WIN32_MODERNFILEDIALOG_H_INCLUDED)
