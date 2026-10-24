// CheatCodeEdit.h : a multiline edit control for cheat code entry
//

#if !defined(AFX_CHEATCODEEDIT_H__3E9B1C64_8A24_4C1B_9D77_1B0C6B2E5A11__INCLUDED_)
#define AFX_CHEATCODEEDIT_H__3E9B1C64_8A24_4C1B_9D77_1B0C6B2E5A11__INCLUDED_

#pragma once

// The Windows EDIT control only starts a new layout line on a CR LF pair.
// Text that arrives with bare LF line endings (which is what a web page
// clipboard usually holds, and what bare CR gives as well) is stored in the
// buffer unchanged but drawn as one single line, so a pasted cheat code list
// looks like "74000130 03FB82037368 0000" and cannot be checked by eye.
// This control rewrites the clipboard into CR LF before the control ever sees
// it, so the codes keep the line breaks the user pasted.
class CCheatCodeEdit : public CEdit
{
	DECLARE_DYNAMIC(CCheatCodeEdit)

public:
	CCheatCodeEdit();

	// Replace the selection with the clipboard text, with CR LF line breaks.
	// Returns false when the clipboard holds no text.
	bool pasteNormalized();

protected:
	afx_msg void OnPaste();

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_CHEATCODEEDIT_H__3E9B1C64_8A24_4C1B_9D77_1B0C6B2E5A11__INCLUDED_)
