#include "stdafx.h"
#include "WinText.h"
#include "WinResUtil.h"

#include <atlconv.h>

CString winWideFromAnsi(const char *text)
{
	if (text == NULL)
		return CString();

	return CString(CA2W(text, CP_ACP));
}

CString winWideFromAnsi(const std::string &text)
{
	return winWideFromAnsi(text.c_str());
}

CStringA winAnsiFromWide(LPCTSTR text)
{
	if (text == NULL)
		return CStringA();

	return CStringA(CW2A(text, CP_ACP));
}

CStringA winAnsiFromWide(const CString &text)
{
	return winAnsiFromWide(text.GetString());
}
