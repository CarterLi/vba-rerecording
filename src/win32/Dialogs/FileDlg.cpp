// FileDlg.cpp: implementation of the FileDlg class.
//
// The dialog itself is the shell dialog of Windows Vista and later, see
// ModernFileDialog.cpp; this class only carries the parameters the rest of
// the emulator passes around and remembers the picked path.
//
//////////////////////////////////////////////////////////////////////
#include "../stdafx.h"
#include "FileDlg.h"
#include "ModernFileDialog.h"
#include "../Sound.h"
#include "../../common/System.h"

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

	WinFileDialogParams params;
	params.owner        = m_parent ? m_parent->GetSafeHwnd() : NULL;
	params.save         = isSave;
	params.mustExist    = !isSave;
	params.hideReadOnly = noReadOnly;
	params.title        = m_title;
	params.initialDir   = m_initialDir;
	params.fileName     = m_file;
	params.filter       = m_filter;
	params.filterIndex  = m_filterIndex;
	params.defaultExt   = m_ext;
	params.extensions   = extensions;

	int index  = m_filterIndex;
	int result = winFileDialog(params, m_path, &index);

	m_filterIndex = index;
	return result;
}

int FileDlg::getFilterIndex()
{
	return m_filterIndex;
}

LPCTSTR FileDlg::GetPathName()
{
	return m_path.c_str();
}
