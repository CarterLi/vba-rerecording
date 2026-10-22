// FileDlg.cpp: implementation of the FileDlg class.
//
// The dialog itself is the shell dialog of Windows Vista and later, see
// ModernFileDialog.cpp; this class only carries the parameters the rest of
// the emulator passes around and remembers the picked path.
//
// That dialog layer speaks the ANSI code page (it is the one place where the
// parameters are handed to OPENFILENAMEA in the Windows XP build), so the
// TCHAR the rest of the Win32 layer uses is converted here, once.
//
//////////////////////////////////////////////////////////////////////
#include "../stdafx.h"
#include "FileDlg.h"
#include "ModernFileDialog.h"
#include "../Sound.h"
#include "../../common/System.h"

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// FileDlg

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FileDlg::FileDlg(CWnd *parent, LPCTSTR file, LPCTSTR filter,
                 int filterIndex, LPCTSTR ext, LPCTSTR *exts, LPCTSTR initialDir,
                 LPCTSTR title, bool save, bool noReadOnly)
	: m_parent(parent), isSave(save), noReadOnly(noReadOnly), extensions(exts),
	  m_filterIndex(filterIndex)
{
	if (file)
		m_file = file;
	if (filter)
		m_filter = filter;
	if (ext)
		m_ext = ext;
	if (initialDir)
		m_initialDir = initialDir;
	if (title)
		m_title = title;
}

FileDlg::~FileDlg()
{
}

int FileDlg::DoModal()
{
	systemSoundClearBuffer();

	// The narrow copies outlive the call: the parameter struct points into
	// them, and it is used for the whole of winFileDialog().
	const CStringA title  = winAnsiFromWide(m_title);
	const CStringA dir    = winAnsiFromWide(m_initialDir);
	const CStringA file   = winAnsiFromWide(m_file);
	const CStringA filter = winAnsiFromWide(m_filter);
	const CStringA ext    = winAnsiFromWide(m_ext);

	std::vector<CStringA>     extensionStorage;
	std::vector<const char *> extensionPointers;

	if (extensions != NULL)
	{
		for (int i = 0; extensions[i] != NULL; ++i)
			extensionStorage.push_back(winAnsiFromWide(extensions[i]));
		for (size_t i = 0; i < extensionStorage.size(); ++i)
			extensionPointers.push_back(extensionStorage[i].GetString());
		extensionPointers.push_back(NULL);
	}

	WinFileDialogParams params;
	params.owner        = m_parent ? m_parent->GetSafeHwnd() : NULL;
	params.save         = isSave;
	params.mustExist    = !isSave;
	params.hideReadOnly = noReadOnly;
	params.title        = m_title.IsEmpty() ? NULL : title.GetString();
	params.initialDir   = m_initialDir.IsEmpty() ? NULL : dir.GetString();
	params.fileName     = file.GetString();
	params.filter       = m_filter.IsEmpty() ? NULL : filter.GetString();
	params.filterIndex  = m_filterIndex;
	params.defaultExt   = m_ext.IsEmpty() ? NULL : ext.GetString();
	params.extensions   = extensionPointers.empty() ? NULL : extensionPointers.data();

	int         index  = m_filterIndex;
	std::string picked;
	int         result = winFileDialog(params, picked, &index);

	if (result == IDOK)
		m_path = winWideFromAnsi(picked);

	m_filterIndex = index;
	return result;
}

int FileDlg::getFilterIndex()
{
	return m_filterIndex;
}

LPCTSTR FileDlg::GetPathName()
{
	return m_path;
}
