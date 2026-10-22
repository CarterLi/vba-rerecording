////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998 by Thierry Maurel
// All rights reserved
//
// Distribute freely, except: don't remove my name from the source or
// documentation (don't take credit for my work), mark your changes (don't
// get me blamed for your possible bugs), don't alter or remove this
// notice.
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
// Send bug reports, bug fixes, enhancements, requests, flames, etc., and
// I'll try to keep a version up to date.  I can be reached as follows:
//    tmaurel@caramail.com   (or tmaurel@hol.fr)
//
////////////////////////////////////////////////////////////////////////////////
// File    : CmdAccelOb.cpp
// Project : AccelsEditor
////////////////////////////////////////////////////////////////////////////////
// Version : 1.0                       * Author : T.Maurel
// Date    : 17.08.98
//
// Remarks :
//
////////////////////////////////////////////////////////////////////////////////
// modified by the VBA-rr team

#include "stdafx.h"
#include "CmdAccelOb.h"

////////////////////////////////////////////////////////////////////////
//
//
MAPVIRTKEYS mapVirtKeys[] = {
	{ VK_LBUTTON,		 _T("LBUTTON")				},
	{ VK_RBUTTON,		 _T("RBUTTON")				},
	{ VK_CANCEL,		 _T("Cancel")				},
	{ VK_MBUTTON,		 _T("MBUTTON")				},
	{ VK_BACK,			 _T("Backspace")			},
	{ VK_TAB,			 _T("Tab")					},
	{ VK_CLEAR,			 _T("Clear")				},
	{ VK_RETURN,		 _T("Enter")				},
	{ VK_SHIFT,			 _T("Shift")				},
	{ VK_LSHIFT,		 _T("LShift")				},
	{ VK_RSHIFT,		 _T("RShift")				},
	{ VK_CONTROL,		 _T("Control")				},
	{ VK_LCONTROL,		 _T("LControl")				},
	{ VK_RCONTROL,		 _T("RControl")				},
	{ VK_MENU,			 _T("Alt")					},
	{ VK_LMENU,			 _T("LAlt")					},
	{ VK_RMENU,			 _T("RAlt")					},
	{ VK_PAUSE,			 _T("Pause")				},
	{ VK_CAPITAL,		 _T("Caps Lock")			},
	{ VK_ESCAPE,		 _T("Escape")				},
	{ VK_SPACE,			 _T("Space")				},
	{ VK_PRIOR,			 _T("Prior")				},
	{ VK_NEXT,			 _T("Next")					},
	{ VK_END,			 _T("End")					},
	{ VK_HOME,			 _T("Home")					},
	{ VK_LEFT,			 _T("Left")					},
	{ VK_UP,			 _T("Up")					},
	{ VK_RIGHT,			 _T("Right")				},
	{ VK_DOWN,			 _T("Down")					},
	{ VK_SELECT,		 _T("Select")				},
	{ VK_PRINT,			 _T("Print")				},
	{ VK_EXECUTE,		 _T("Execute")				},
	{ VK_SNAPSHOT,		 _T("Snapshot")				},
	{ VK_INSERT,		 _T("Insert")				},
	{ VK_DELETE,		 _T("Delete")				},
	{ VK_HELP,			 _T("Help")					},
	{ WORD('0'),		 _T("0")					},
	{ WORD('1'),		 _T("1")					},
	{ WORD('2'),		 _T("2")					},
	{ WORD('3'),		 _T("3")					},
	{ WORD('4'),		 _T("4")					},
	{ WORD('5'),		 _T("5")					},
	{ WORD('6'),		 _T("6")					},
	{ WORD('7'),		 _T("7")					},
	{ WORD('8'),		 _T("8")					},
	{ WORD('9'),		 _T("9")					},
	{ WORD('A'),		 _T("A")					},
	{ WORD('B'),		 _T("B")					},
	{ WORD('C'),		 _T("C")					},
	{ WORD('D'),		 _T("D")					},
	{ WORD('E'),		 _T("E")					},
	{ WORD('F'),		 _T("F")					},
	{ WORD('G'),		 _T("G")					},
	{ WORD('H'),		 _T("H")					},
	{ WORD('I'),		 _T("I")					},
	{ WORD('J'),		 _T("J")					},
	{ WORD('K'),		 _T("K")					},
	{ WORD('L'),		 _T("L")					},
	{ WORD('M'),		 _T("M")					},
	{ WORD('N'),		 _T("N")					},
	{ WORD('O'),		 _T("O")					},
	{ WORD('P'),		 _T("P")					},
	{ WORD('Q'),		 _T("Q")					},
	{ WORD('R'),		 _T("R")					},
	{ WORD('S'),		 _T("S")					},
	{ WORD('T'),		 _T("T")					},
	{ WORD('U'),		 _T("U")					},
	{ WORD('V'),		 _T("V")					},
	{ WORD('W'),		 _T("W")					},
	{ WORD('X'),		 _T("X")					},
	{ WORD('Y'),		 _T("Y")					},
	{ WORD('Z'),		 _T("Z")					},
	{ VK_LWIN,			 _T("LWIN")					},
	{ VK_RWIN,			 _T("RWIN")					},
	{ VK_APPS,			 _T("APPS")					},
	{ VK_NUMPAD0,		 _T("NumPad0")				},
	{ VK_NUMPAD1,		 _T("NumPad1")				},
	{ VK_NUMPAD2,		 _T("NumPad2")				},
	{ VK_NUMPAD3,		 _T("NumPad3")				},
	{ VK_NUMPAD4,		 _T("NumPad4")				},
	{ VK_NUMPAD5,		 _T("NumPad5")				},
	{ VK_NUMPAD6,		 _T("NumPad6")				},
	{ VK_NUMPAD7,		 _T("NumPad7")				},
	{ VK_NUMPAD8,		 _T("NumPad8")				},
	{ VK_NUMPAD9,		 _T("NumPad9")				},
	{ VK_MULTIPLY,		 _T("NumpadMultiply")		},
	{ VK_ADD,			 _T("NumpadAdd")			},
	{ VK_SEPARATOR,		 _T("Separator")			},
	{ VK_SUBTRACT,		 _T("NumpadSubtract")		},
	{ VK_DECIMAL,		 _T("NumpadDecimal")		},
	{ VK_DIVIDE,		 _T("NumpadDivide")			},
	{ VK_F1,			 _T("F1")					},
	{ VK_F2,			 _T("F2")					},
	{ VK_F3,			 _T("F3")					},
	{ VK_F4,			 _T("F4")					},
	{ VK_F5,			 _T("F5")					},
	{ VK_F6,			 _T("F6")					},
	{ VK_F7,			 _T("F7")					},
	{ VK_F8,			 _T("F8")					},
	{ VK_F9,			 _T("F9")					},
	{ VK_F10,			 _T("F10")					},
	{ VK_F11,			 _T("F11")					},
	{ VK_F12,			 _T("F12")					},
	{ VK_F13,			 _T("F13")					},
	{ VK_F14,			 _T("F14")					},
	{ VK_F15,			 _T("F15")					},
	{ VK_F16,			 _T("F16")					},
	{ VK_F17,			 _T("F17")					},
	{ VK_F18,			 _T("F18")					},
	{ VK_F19,			 _T("F19")					},
	{ VK_F20,			 _T("F20")					},
	{ VK_F21,			 _T("F21")					},
	{ VK_F22,			 _T("F22")					},
	{ VK_F23,			 _T("F23")					},
	{ VK_F24,			 _T("F24")					},
	{ VK_NUMLOCK,		 _T("Num Lock")				},
	{ VK_SCROLL,		 _T("Scroll Lock")			},
	{ VK_ATTN,			 _T("Attention")			},
	{ VK_CRSEL,			 _T("CRSEL")				},
	{ VK_EXSEL,			 _T("EXSEL")				},
	{ VK_EREOF,			 _T("EREOF")				},
	{ VK_PLAY,			 _T("Play")					},
	{ VK_ZOOM,			 _T("Zoom")					},
	{ VK_NONAME,		 _T("No Name")				},
	{ VK_PA1,			 _T("PA1")					},
	{ VK_OEM_CLEAR,		 _T("Clear2")				},
	{ VK_OEM_1,			 _T("Semicolon;")			},
	{ VK_OEM_2,			 _T("Slash/")				},
	{ VK_OEM_3,			 _T("Tilde~")				},
	{ VK_OEM_4,			 _T("LBracket[")			},
	{ VK_OEM_5,			 _T("Backslash\\")			},
	{ VK_OEM_6,			 _T("RBracket]")			},
	{ VK_OEM_7,			 _T("Apostrophe'")			},
	{ VK_OEM_8,			 _T("OEM8")					},
	{ VK_OEM_PLUS,		 _T("Plus+")				},
	{ VK_OEM_MINUS,		 _T("Minus-")				},
	{ VK_OEM_COMMA,		 _T("Comma,")				},
	{ VK_OEM_PERIOD,	 _T("Period.")				},
	{ VK_OEM_AX,		 _T("Apostrophe`")			},
	{ VK_OEM_102,		 _T("<> or \\|")			},
	{ VK_ICO_HELP,		 _T("ICO Help")				},
	{ VK_ICO_00,		 _T("ICO 00")				},
	{ VK_OEM_FJ_JISHO,	 _T("JISHO")				},
	{ VK_OEM_FJ_MASSHOU, _T("MASSHOU")				},
	{ VK_OEM_FJ_TOUROKU, _T("TOUROKU")				},
	{ VK_OEM_FJ_LOYA,	 _T("LOYA")					},
	{ VK_OEM_FJ_ROYA,	 _T("ROYA")					},
	{ VK_OEM_NEC_EQUAL,	 _T("Numpad Equals")		},
};

////////////////////////////////////////////////////////////////////////
//
//
MAPVIRTKEYS mapVirtSysKeys[] = {
	{ FCONTROL, _T("Ctrl")	  },
	{ FALT,		_T("Alt")	  },
	{ FSHIFT,	_T("Shift")	  },
};

////////////////////////////////////////////////////////////////////////
// helper fct for external access
////////////////////////////////////////////////////////////////////////
//
//
const TCHAR *mapVirtKeysStringFromWORD(WORD wKey)
{
	for (int index = 0; index < sizeof(mapVirtKeys) / sizeof(mapVirtKeys[0]); index++)
	{
		if (mapVirtKeys[index].wKey == wKey)
		{
			return mapVirtKeys[index].szKey;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////
//
#define DEFAULT_ACCEL   0x01
#define USER_ACCEL      0x02

#ifndef _countof
#define sizetable(table) (sizeof(table)/sizeof(table[0]))
#else
#define sizetable(table) _countof(table)
#endif

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
//
//
CAccelsOb::CAccelsOb()
{
	m_cVirt	  = 0;
	m_wKey	  = 0;
	m_bLocked = false;
}

////////////////////////////////////////////////////////////////////////
//
//
CAccelsOb::CAccelsOb(CAccelsOb *pFrom)
{
	ASSERT(pFrom != NULL);

	m_cVirt	  = pFrom->m_cVirt;
	m_wKey	  = pFrom->m_wKey;
	m_bLocked = pFrom->m_bLocked;
}

////////////////////////////////////////////////////////////////////////
//
//
CAccelsOb::CAccelsOb(BYTE cVirt, WORD wKey, bool bLocked)
{
	m_cVirt	  = cVirt;
	m_wKey	  = wKey;
	m_bLocked = bLocked;
}

////////////////////////////////////////////////////////////////////////
//
//
CAccelsOb::CAccelsOb(LPACCEL pACCEL)
{
	ASSERT(pACCEL != NULL);

	m_cVirt	  = pACCEL->fVirt;
	m_wKey	  = pACCEL->key;
	m_bLocked = false;
}

////////////////////////////////////////////////////////////////////////
//
//
CAccelsOb & CAccelsOb::operator=(const CAccelsOb &from)
{
	m_cVirt	  = from.m_cVirt;
	m_wKey	  = from.m_wKey;
	m_bLocked = from.m_bLocked;

	return *this;
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
//
//
void CAccelsOb::GetString(CString &szBuffer)
{
	szBuffer = "";

	// modifiers part
	for (int i = 0; i < sizetable(mapVirtSysKeys); i++)
	{
		if (m_cVirt & mapVirtSysKeys[i].wKey)
		{
			szBuffer += mapVirtSysKeys[i].szKey;
			if (m_wKey)
				szBuffer += "+";
		}
	}

	// in case of the object is not assigned, we avoid error messages
	if (m_wKey == 0)
		return;

	// and virtual key part
	for (int i = 0; i < sizetable(mapVirtKeys); i++)
	{
		if (m_wKey == mapVirtKeys[i].wKey)
		{
			szBuffer += mapVirtKeys[i].szKey;
			return;
		}
	}

//	systemMessage(0, "%d", m_wKey);
	//AfxMessageBox("Internal error : (CAccelsOb::GetString) m_wKey invalid");
}

////////////////////////////////////////////////////////////////////////
//
//
bool CAccelsOb::IsEqual(WORD wKey, bool bCtrl, bool bAlt, bool bShift)
{
	bool m_bCtrl  = (m_cVirt & FCONTROL) ? true : false;
	bool m_bAlt	  = (m_cVirt & FALT) ? true : false;
	bool m_bShift = (m_cVirt & FSHIFT) ? true : false;

	bool bRet = (bCtrl == m_bCtrl) && (bAlt == m_bAlt) && (bShift == m_bShift) && (m_wKey == wKey);

	return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//
DWORD CAccelsOb::GetData()
{
	BYTE cLocalCodes = 0;
	if (m_bLocked)
		cLocalCodes = DEFAULT_ACCEL;
	else
		cLocalCodes = USER_ACCEL;

	WORD bCodes = MAKEWORD(m_cVirt, cLocalCodes);
	return MAKELONG(m_wKey, bCodes);
}

////////////////////////////////////////////////////////////////////////
//
//
bool CAccelsOb::SetData(DWORD dwDatas)
{
	m_wKey = LOWORD(dwDatas);

	WORD bCodes = HIWORD(dwDatas);
	m_cVirt = LOBYTE(bCodes);

	BYTE cLocalCodes = HIBYTE(bCodes);
	m_bLocked = (cLocalCodes == DEFAULT_ACCEL);

	return true;
}

////////////////////////////////////////////////////////////////////////
//
#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////
//
//
void CAccelsOb::AssertValid() const
{
	CObject::AssertValid();
}

////////////////////////////////////////////////////////////////////////
//
//
void CAccelsOb::Dump(CDumpContext &dc) const
{
	dc << "\t\t";
	CObject::Dump(dc);
	dc << "\t\tlocked=" << m_bLocked << ", cVirt=" << m_cVirt << ", wKey=" << m_wKey << "\n\n";
}

#endif

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
//
//
CCmdAccelOb::CCmdAccelOb()
{}

////////////////////////////////////////////////////////////////////////
//
//
CCmdAccelOb::CCmdAccelOb(WORD wIDCommand, LPCTSTR szCommand)
{
	ASSERT(szCommand != NULL);

	m_wIDCommand = wIDCommand;
	m_szCommand	 = szCommand;
}

////////////////////////////////////////////////////////////////////////
//
//
CCmdAccelOb::CCmdAccelOb(BYTE cVirt, WORD wIDCommand, WORD wKey, LPCTSTR szCommand, bool bLocked)
{
	ASSERT(szCommand != NULL);

	m_wIDCommand = wIDCommand;
	m_szCommand	 = szCommand;

	CAccelsOb *pAccel = new CAccelsOb(cVirt, wKey, bLocked);
	ASSERT(pAccel != NULL);
	m_Accels.AddTail(pAccel);
}

////////////////////////////////////////////////////////////////////////
//
//
CCmdAccelOb::~CCmdAccelOb()
{
	POSITION pos = m_Accels.GetHeadPosition();
	while (pos != NULL)
		delete m_Accels.GetNext(pos);
	m_Accels.RemoveAll();
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
//
//
void CCmdAccelOb::Add(BYTE cVirt, WORD wKey, bool bLocked)
{
	CAccelsOb *pAccel = new CAccelsOb(cVirt, wKey, bLocked);
	ASSERT(pAccel != NULL);
	m_Accels.AddTail(pAccel);
}

////////////////////////////////////////////////////////////////////////
//
//
void CCmdAccelOb::Add(CAccelsOb *pAccel)
{
	ASSERT(pAccel != NULL);
	m_Accels.AddTail(pAccel);
}

////////////////////////////////////////////////////////////////////////
//
//
CCmdAccelOb & CCmdAccelOb::operator=(const CCmdAccelOb &from)
{
	Reset();

	m_wIDCommand = from.m_wIDCommand;
	m_szCommand	 = from.m_szCommand;

	CAccelsOb *pAccel;
	POSITION   pos = from.m_Accels.GetHeadPosition();
	while (pos != NULL)
	{
		pAccel = new CAccelsOb(from.m_Accels.GetNext(pos));
		ASSERT(pAccel != NULL);
		m_Accels.AddTail(pAccel);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////
//
//
void CCmdAccelOb::DeleteUserAccels()
{
	CAccelsOb *pAccel;
	POSITION   prevPos;
	POSITION   pos = m_Accels.GetHeadPosition();
	while (pos != NULL)
	{
		prevPos = pos;
		pAccel	= m_Accels.GetNext(pos);
		if (!pAccel->m_bLocked)
		{
			delete pAccel;
			m_Accels.RemoveAt(prevPos);
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
//
void CCmdAccelOb::Reset()
{
	m_wIDCommand = 0;
	m_szCommand	 = "Empty command";

	CAccelsOb *pAccel;
	POSITION   pos = m_Accels.GetHeadPosition();
	while (pos != NULL)
	{
		pAccel = m_Accels.GetNext(pos);
		delete pAccel;
	}
}

////////////////////////////////////////////////////////////////////////
//
#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////
//
//
void CCmdAccelOb::AssertValid() const
{
	// call base class function first
	CObject::AssertValid();
}

////////////////////////////////////////////////////////////////////////
//
//
void CCmdAccelOb::Dump(CDumpContext &dc) const
{
	// call base class function first
	dc << "\t";
	CObject::Dump(dc);

	// now do the stuff for our specific class
	dc << "\tIDCommand = " << m_wIDCommand;
	dc << "\n\tszCommand = " << m_szCommand;
	dc << "\n\tAccelerators = {\n";

	CAccelsOb *pAccel;
	POSITION   pos = m_Accels.GetHeadPosition();
	while (pos != NULL)
	{
		pAccel = m_Accels.GetNext(pos);
		dc << pAccel;
	}
	dc << "\t}\n";
}

#endif
