// CheatCodeEdit.cpp : implementation file
//

#include "../stdafx.h"
#include "CheatCodeEdit.h"

#include "../WinText.h"

IMPLEMENT_DYNAMIC(CCheatCodeEdit, CEdit)

BEGIN_MESSAGE_MAP(CCheatCodeEdit, CEdit)
	ON_WM_PASTE()
END_MESSAGE_MAP()

CCheatCodeEdit::CCheatCodeEdit()
{
}

namespace
{

bool readClipboardText(HWND owner, CString &text)
{
	if (!::OpenClipboard(owner))
		return false;

	bool found = false;

	if (HANDLE handle = ::GetClipboardData(CF_UNICODETEXT))
	{
		if (const TCHAR *data = (const TCHAR *)::GlobalLock(handle))
		{
			text  = data;
			found = true;
			::GlobalUnlock(handle);
		}
	}

	// An older app may offer ANSI text only.
	if (!found)
	{
		if (HANDLE handle = ::GetClipboardData(CF_TEXT))
		{
			if (const char *data = (const char *)::GlobalLock(handle))
			{
				text  = winWideFromAnsi(data);
				found = true;
				::GlobalUnlock(handle);
			}
		}
	}

	::CloseClipboard();
	return found;
}

// CR LF, LF and lone CR all mean "new line" to a text editor, but only CR LF
// means "new line" to the EDIT control.
CString withCrLfLineBreaks(const CString &text)
{
	CString result;
	result.Preallocate(text.GetLength() + 32);

	for (int i = 0; i < text.GetLength(); i++)
	{
		TCHAR c = text[i];
		if (c == _T('\r'))
		{
			result += _T("\r\n");
			if (i + 1 < text.GetLength() && text[i + 1] == _T('\n'))
				i++;
		}
		else if (c == _T('\n'))
			result += _T("\r\n");
		else
			result += c;
	}

	return result;
}

} // namespace

bool CCheatCodeEdit::pasteNormalized()
{
	CString text;
	if (!readClipboardText(GetSafeHwnd(), text) || text.IsEmpty())
		return false;

	text = withCrLfLineBreaks(text);

	// EM_REPLACESEL replaces the selection, keeps undo working and still lets
	// the control apply ES_UPPERCASE and its text limit.
	SendMessage(EM_REPLACESEL, TRUE, (LPARAM)(LPCTSTR)text);
	return true;
}

void CCheatCodeEdit::OnPaste()
{
	// Ctrl+V reaches the control as WM_CHAR 0x16, which the control turns into
	// a WM_PASTE of its own, so handling WM_PASTE covers both the keyboard
	// shortcut and the context menu entry.
	if (!pasteNormalized())
		Default();
}
