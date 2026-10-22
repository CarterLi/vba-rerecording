#if !defined(AFX_FILEDLG_H__7E4F8B92_1B63_4126_8261_D9334C645940__INCLUDED_)
#define AFX_FILEDLG_H__7E4F8B92_1B63_4126_8261_D9334C645940__INCLUDED_

#pragma once
// FileDlg.h : header file
//
// Thin MFC wrapper around the file dialogs of ModernFileDialog.h.

#include <string>

/////////////////////////////////////////////////////////////////////////////
// FileDlg dialog

class FileDlg
{
public:
	int DoModal();
	LPCTSTR GetPathName();
	virtual int getFilterIndex();

	FileDlg(CWnd *parent, LPCTSTR file, LPCTSTR filter,
	        int filterIndex, LPCTSTR ext, LPCTSTR *exts, LPCTSTR initialDir,
	        LPCTSTR title, bool save, bool noReadOnly = false);
	virtual ~FileDlg();

protected:
	CWnd     *m_parent;
	bool      isSave;
	bool      noReadOnly;
	LPCTSTR  *extensions;
	int       m_filterIndex;
	CString m_path;

private:
	CString m_file;
	CString m_filter;
	CString m_ext;
	CString m_initialDir;
	CString m_title;
};

#endif // !defined(AFX_FILEDLG_H__7E4F8B92_1B63_4126_8261_D9334C645940__INCLUDED_)
