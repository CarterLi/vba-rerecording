#include "stdafx.h"
#include "resource.h"
#include "MainWnd.h"
#include "VBA.h"
#include "Dialogs/AboutDialog.h"
#include "Dialogs/BugReport.h"

void MainWnd::OnHelpAbout()
{
	AboutDialog dlg;

	dlg.DoModal();
}

void MainWnd::OnHelpFaq()
{
	::ShellExecute(0, _T("open"), _T("http://code.google.com/p/vba-rerecording/w/list"),
	               0, 0, SW_SHOWNORMAL);
}

void MainWnd::OnHelpBugreport()
{
	BugReport dlg(theApp.m_pMainWnd);

	dlg.DoModal();
}

