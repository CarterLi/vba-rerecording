// Joypad.cpp : implementation file
//

#include "../stdafx.h"
#include "../resource.h"
#include "Joypad.h"
#include "../Input.h"
#include "../VBA.h"

extern USHORT joypad[4][13];
extern USHORT autoFireKeys[4][13];
extern USHORT motion[4];

/////////////////////////////////////////////////////////////////////////////
// JoypadEditControl

JoypadEditControl::JoypadEditControl()
{}

JoypadEditControl::~JoypadEditControl()
{}

BEGIN_MESSAGE_MAP(JoypadEditControl, CEdit)
//{{AFX_MSG_MAP(JoypadEditControl)
ON_WM_CHAR()
//}}AFX_MSG_MAP
ON_MESSAGE(JOYCONFIG_MESSAGE, OnJoyConfig)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// JoypadEditControl message handlers

void JoypadEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{}

LRESULT JoypadEditControl::OnJoyConfig(WPARAM wParam, LPARAM lParam)
{
	SetWindowLongPtr(GetSafeHwnd(), GWLP_USERDATA, ((wParam<<8)|lParam));
	SetWindowText(theApp.input->getKeyName((wParam<<8)|lParam));
	GetParent()->GetNextDlgTabItem(this, FALSE)->SetFocus();
	return TRUE;
}

BOOL JoypadEditControl::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN))
		return TRUE;

	return CEdit::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// JoypadConfig dialog

JoypadConfig::JoypadConfig(int w, CWnd*pParent /*=NULL*/)
	: CDialog(JoypadConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(JoypadConfig)
	//}}AFX_DATA_INIT
	timerId = 0;
	lastKeyId = IDC_EDIT_UP;
	which   = w;
	if (which < 0 || which > 3)
		which = 0;
}

void JoypadConfig::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(JoypadConfig)
	DDX_Control(pDX, IDC_EDIT_UP, up);
	DDX_Control(pDX, IDC_EDIT_SPEED, speed);
	DDX_Control(pDX, IDC_EDIT_RIGHT, right);
	DDX_Control(pDX, IDC_EDIT_LEFT, left);
	DDX_Control(pDX, IDC_EDIT_DOWN, down);
	DDX_Control(pDX, IDC_EDIT_CAPTURE, capture);
	DDX_Control(pDX, IDC_EDIT_BUTTON_START, buttonStart);
	DDX_Control(pDX, IDC_EDIT_BUTTON_SELECT, buttonSelect);
	DDX_Control(pDX, IDC_EDIT_BUTTON_R, buttonR);
	DDX_Control(pDX, IDC_EDIT_BUTTON_L, buttonL);
	DDX_Control(pDX, IDC_EDIT_BUTTON_GS, buttonGS);
	DDX_Control(pDX, IDC_EDIT_BUTTON_B, buttonB);
	DDX_Control(pDX, IDC_EDIT_BUTTON_A, buttonA);
	DDX_Control(pDX, IDC_JOYPAD_TAB, tab);
	DDX_Control(pDX, IDC_GAMEPAD_LIST, gamepadList);
	DDX_Control(pDX, IDC_EDIT_AF_UP, afUp);
	DDX_Control(pDX, IDC_EDIT_AF_DOWN, afDown);
	DDX_Control(pDX, IDC_EDIT_AF_LEFT, afLeft);
	DDX_Control(pDX, IDC_EDIT_AF_RIGHT, afRight);
	DDX_Control(pDX, IDC_EDIT_AF_A, afA);
	DDX_Control(pDX, IDC_EDIT_AF_B, afB);
	DDX_Control(pDX, IDC_EDIT_AF_L, afL);
	DDX_Control(pDX, IDC_EDIT_AF_R, afR);
	DDX_Control(pDX, IDC_EDIT_AF_SELECT, afSelect);
	DDX_Control(pDX, IDC_EDIT_AF_START, afStart);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(JoypadConfig, CDialog)
//{{AFX_MSG_MAP(JoypadConfig)
ON_BN_CLICKED(ID_CANCEL, OnCancel)
ON_BN_CLICKED(ID_OK, OnOk)
ON_WM_CHAR()
ON_WM_DESTROY()
ON_WM_TIMER()
ON_WM_KEYDOWN()
ON_WM_CTLCOLOR()
ON_NOTIFY(TCN_SELCHANGE, IDC_JOYPAD_TAB, OnSelChangeTab)
ON_CONTROL_RANGE(EN_SETFOCUS, IDC_EDIT_UP, IDC_EDIT_BUTTON_GS, OnKeyFocus)
ON_CONTROL_RANGE(EN_SETFOCUS, IDC_EDIT_AF_UP, IDC_EDIT_AF_START, OnKeyFocus)
ON_BN_CLICKED(IDC_CLEAR_KEY, OnClearKey)
ON_EN_CHANGE(IDC_EDIT_DEADZONE, OnDeadzoneChanged)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// JoypadConfig message handlers

void JoypadConfig::OnCancel()
{
	EndDialog(FALSE);
}

void JoypadConfig::OnOk()
{
	assignKeys();
	theApp.input->checkKeys();
	EndDialog(TRUE);
}

void JoypadConfig::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{}

void JoypadConfig::OnDestroy()
{
	CDialog::OnDestroy();

	KillTimer(timerId);
}

void JoypadConfig::OnTimer(UINT_PTR nIDEvent)
{
	theApp.input->checkDevices();

	CDialog::OnTimer(nIDEvent);
}

void JoypadConfig::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{}

// The labels of this dialog are siblings of the tab control and are drawn on
// top of it, but a static control fills its rectangle with the dialog
// background - COLOR_3DFACE, #f0f0f0 - while the tab paints its page with the
// surface colour of the current visual style, which on Windows 11 is a card a
// few shades lighter than the dialog.  Left to itself every label therefore
// shows up as a grey box.  Returning a hollow brush and a transparent text
// mode lets the tab page show through instead, so the label sits on the same
// surface as the rest of the page.  The labels carry a fixed text, so nothing
// ever needs the background erased underneath them.
HBRUSH JoypadConfig::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void JoypadConfig::OnKeyFocus(UINT nID)
{
	lastKeyId = nID;
}

// Clearing a binding stores 0: device 0, scan code 0, a code no key produces,
// so the button stays released.  There is no other way back from an assigned
// key to "unassigned", and the Auto-Fire toggles need one as much as the
// game buttons do - a toggle key that is set by accident keeps toggling.
void JoypadConfig::OnClearKey()
{
	CWnd *wnd = GetDlgItem(lastKeyId);

	if (wnd == NULL)
		return;

	SetWindowLongPtr(wnd->m_hWnd, GWLP_USERDATA, 0);
	wnd->SetWindowText("");
	wnd->SetFocus();
}

BOOL JoypadConfig::OnInitDialog()
{
	CDialog::OnInitDialog();

	timerId = SetTimer(0, 200, NULL);

	// A controller that was asleep, was plugged in, or was paired after the
	// emulator started is not in the device list yet: the list is built once
	// at startup.  Without this the dialog simply ignores that pad.
	if (theApp.input)
		theApp.input->refreshDevices();

	lastKeyId = IDC_EDIT_UP;

	SetWindowLongPtr(up, GWLP_USERDATA, joypad[which][KEY_UP]);
	up.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_UP]));

	SetWindowLongPtr(down, GWLP_USERDATA, joypad[which][KEY_DOWN]);
	down.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_DOWN]));

	SetWindowLongPtr(left, GWLP_USERDATA, joypad[which][KEY_LEFT]);
	left.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_LEFT]));

	SetWindowLongPtr(right, GWLP_USERDATA, joypad[which][KEY_RIGHT]);
	right.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_RIGHT]));

	SetWindowLongPtr(buttonA, GWLP_USERDATA, joypad[which][KEY_BUTTON_A]);
	buttonA.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_A]));

	SetWindowLongPtr(buttonB, GWLP_USERDATA, joypad[which][KEY_BUTTON_B]);
	buttonB.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_B]));

	SetWindowLongPtr(buttonL, GWLP_USERDATA, joypad[which][KEY_BUTTON_L]);
	buttonL.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_L]));

	SetWindowLongPtr(buttonR, GWLP_USERDATA, joypad[which][KEY_BUTTON_R]);
	buttonR.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_R]));

	SetWindowLongPtr(buttonSelect, GWLP_USERDATA, joypad[which][KEY_BUTTON_SELECT]);
	buttonSelect.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_SELECT]));

	SetWindowLongPtr(buttonStart, GWLP_USERDATA, joypad[which][KEY_BUTTON_START]);
	buttonStart.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_START]));

	SetWindowLongPtr(speed, GWLP_USERDATA, joypad[which][KEY_BUTTON_SPEED]);
	speed.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_SPEED]));

	SetWindowLongPtr(capture, GWLP_USERDATA, joypad[which][KEY_BUTTON_CAPTURE]);
	capture.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_CAPTURE]));

	SetWindowLongPtr(buttonGS, GWLP_USERDATA, joypad[which][KEY_BUTTON_GS]);
	buttonGS.SetWindowText(theApp.input->getKeyName(joypad[which][KEY_BUTTON_GS]));

	tab.InsertItem(0, _T("Buttons"));
	tab.InsertItem(1, _T("Auto-Fire"));

	// the page only exists when there is something to put on it
	if (theApp.input->gamepadCount() > 0)
	{
		tab.InsertItem(2, _T("Gamepads"));

		CString deadzone;
		deadzone.Format("%d", theApp.input->getDeadzone());
		SetDlgItemText(IDC_EDIT_DEADZONE, deadzone);

		fillGamepadList();
	}
	else
	{
		gamepadList.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_DEADZONE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_DEADZONE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_DEADZONE_UNIT)->ShowWindow(SW_HIDE);
	}

	SetWindowLongPtr(afUp, GWLP_USERDATA, autoFireKeys[which][KEY_UP]);
	afUp.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_UP]));

	SetWindowLongPtr(afDown, GWLP_USERDATA, autoFireKeys[which][KEY_DOWN]);
	afDown.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_DOWN]));

	SetWindowLongPtr(afLeft, GWLP_USERDATA, autoFireKeys[which][KEY_LEFT]);
	afLeft.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_LEFT]));

	SetWindowLongPtr(afRight, GWLP_USERDATA, autoFireKeys[which][KEY_RIGHT]);
	afRight.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_RIGHT]));

	SetWindowLongPtr(afA, GWLP_USERDATA, autoFireKeys[which][KEY_BUTTON_A]);
	afA.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_BUTTON_A]));

	SetWindowLongPtr(afB, GWLP_USERDATA, autoFireKeys[which][KEY_BUTTON_B]);
	afB.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_BUTTON_B]));

	SetWindowLongPtr(afL, GWLP_USERDATA, autoFireKeys[which][KEY_BUTTON_L]);
	afL.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_BUTTON_L]));

	SetWindowLongPtr(afR, GWLP_USERDATA, autoFireKeys[which][KEY_BUTTON_R]);
	afR.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_BUTTON_R]));

	SetWindowLongPtr(afSelect, GWLP_USERDATA, autoFireKeys[which][KEY_BUTTON_SELECT]);
	afSelect.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_BUTTON_SELECT]));

	SetWindowLongPtr(afStart, GWLP_USERDATA, autoFireKeys[which][KEY_BUTTON_START]);
	afStart.SetWindowText(theApp.input->getKeyName(autoFireKeys[which][KEY_BUTTON_START]));

	showPage(0);

	CenterWindow();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

void JoypadConfig::OnSelChangeTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	showPage(tab.GetCurSel());
	*pResult = 0;
}

// The list takes the rectangle the tab control keeps for its page, so it looks
// like part of the page instead of a control that happens to sit on it.
// The dead zone is applied where the axes are compared, because the pads that
// drift are the ones whose driver ignores DirectInput's own dead zone.
void JoypadConfig::OnDeadzoneChanged()
{
	CString text;

	GetDlgItemText(IDC_EDIT_DEADZONE, text);

	if (text.IsEmpty())
		return;

	int percent = atoi(text);

	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	theApp.input->setDeadzone(percent);
}

void JoypadConfig::fillGamepadList()
{
	CRect tabRect;
	tab.GetWindowRect(&tabRect);
	ScreenToClient(&tabRect);

	CRect page;
	tab.GetClientRect(&page);
	tab.AdjustRect(FALSE, &page);
	page.OffsetRect(tabRect.left, tabRect.top);
	page.DeflateRect(3, 3);

	// the dead zone sits at the bottom of the page, so the list stops above it
	if (CWnd *deadzone = GetDlgItem(IDC_EDIT_DEADZONE))
	{
		CRect row;
		deadzone->GetWindowRect(&row);
		ScreenToClient(&row);
		page.bottom = row.top - 6;
	}

	gamepadList.MoveWindow(&page);

	gamepadList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	gamepadList.InsertColumn(0, _T("No"), LVCFMT_LEFT, 32);
	// leave room for the border and a vertical scrollbar, or the name column
	// alone adds a horizontal one
	gamepadList.InsertColumn(1, _T("Name"), LVCFMT_LEFT, page.Width() - 32 - 24);

	int count = theApp.input->gamepadCount();

	for (int i = 0, row = 0; i < count; i++)
	{
		int     device = 0;
		CString name;

		if (!theApp.input->gamepadInfo(i, device, name))
			continue;

		// the number is the one the captured key shows as "Joy <n>"
		CString number;
		number.Format("%d", device);

		int item = gamepadList.InsertItem(row++, number);
		gamepadList.SetItemText(item, 1, name);
	}
}

void JoypadConfig::showPage(int page)
{
	const int joyIds[] = {
		IDC_EDIT_UP, IDC_EDIT_DOWN, IDC_EDIT_LEFT, IDC_EDIT_RIGHT,
		IDC_EDIT_BUTTON_A, IDC_EDIT_BUTTON_B, IDC_EDIT_BUTTON_L, IDC_EDIT_BUTTON_R,
		IDC_EDIT_BUTTON_SELECT, IDC_EDIT_BUTTON_START,
		IDC_EDIT_SPEED, IDC_EDIT_CAPTURE, IDC_EDIT_BUTTON_GS
	};
	const int afIds[] = {
		IDC_EDIT_AF_UP, IDC_EDIT_AF_DOWN, IDC_EDIT_AF_LEFT, IDC_EDIT_AF_RIGHT,
		IDC_EDIT_AF_A, IDC_EDIT_AF_B, IDC_EDIT_AF_L, IDC_EDIT_AF_R,
		IDC_EDIT_AF_SELECT, IDC_EDIT_AF_START
	};
	// the labels used to share IDC_STATIC, which GetDlgItem() cannot find, so
	// they stayed on screen over every page; they carry their own ids now
	const int sharedLabels[] = {
		IDC_STATIC_UP, IDC_STATIC_DOWN, IDC_STATIC_LEFT, IDC_STATIC_RIGHT,
		IDC_STATIC_BUTTON_A, IDC_STATIC_BUTTON_B, IDC_STATIC_BUTTON_L, IDC_STATIC_BUTTON_R,
		IDC_STATIC_SELECT, IDC_STATIC_START
	};
	const int joyOnlyLabels[] = {
		IDC_STATIC_SPEED, IDC_STATIC_CAPTURE, IDC_STATIC_GS
	};

	bool autoFirePage = (page == 1);
	bool gamepadPage  = (page == 2);
	bool buttonsPage  = !autoFirePage && !gamepadPage;

	// the Clear button works on a field of the page that is showing
	if (!gamepadPage)
		lastKeyId = autoFirePage ? IDC_EDIT_AF_UP : IDC_EDIT_UP;

	for (int i = 0; i < sizeof(joyIds) / sizeof(joyIds[0]); i++)
	{
		CWnd *wnd = GetDlgItem(joyIds[i]);
		if (wnd)
			wnd->ShowWindow(buttonsPage ? SW_SHOW : SW_HIDE);
	}
	for (int i = 0; i < sizeof(afIds) / sizeof(afIds[0]); i++)
	{
		CWnd *wnd = GetDlgItem(afIds[i]);
		if (wnd)
			wnd->ShowWindow(autoFirePage ? SW_SHOW : SW_HIDE);
	}
	// the same ten labels name the fields of both pages, so they only go away
	// on the page that has no fields
	for (int i = 0; i < sizeof(sharedLabels) / sizeof(sharedLabels[0]); i++)
	{
		CWnd *wnd = GetDlgItem(sharedLabels[i]);
		if (wnd)
			wnd->ShowWindow(gamepadPage ? SW_HIDE : SW_SHOW);
	}
	for (int i = 0; i < sizeof(joyOnlyLabels) / sizeof(joyOnlyLabels[0]); i++)
	{
		CWnd *wnd = GetDlgItem(joyOnlyLabels[i]);
		if (wnd)
			wnd->ShowWindow(buttonsPage ? SW_SHOW : SW_HIDE);
	}

	gamepadList.ShowWindow(gamepadPage ? SW_SHOW : SW_HIDE);

	const int deadzoneIds[] = {
		IDC_STATIC_DEADZONE, IDC_EDIT_DEADZONE, IDC_STATIC_DEADZONE_UNIT
	};

	for (int i = 0; i < sizeof(deadzoneIds) / sizeof(deadzoneIds[0]); i++)
	{
		CWnd *wnd = GetDlgItem(deadzoneIds[i]);
		if (wnd)
			wnd->ShowWindow(gamepadPage ? SW_SHOW : SW_HIDE);
	}

	if (CWnd *clear = GetDlgItem(IDC_CLEAR_KEY))
		clear->ShowWindow(gamepadPage ? SW_HIDE : SW_SHOW);

	// A control that is hidden or shown leaves its pixels behind: the tab
	// paints its page once and does not know a sibling on top of it moved, and
	// a list that was just resized paints only the part it thinks is dirty.
	// Repainting the dialog together with its children is what clears it.
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void JoypadConfig::assignKey(int id, int key)
{
	switch (id)
	{
	case IDC_EDIT_LEFT:
		joypad[which][KEY_LEFT] = key;
		break;
	case IDC_EDIT_RIGHT:
		joypad[which][KEY_RIGHT] = key;
		break;
	case IDC_EDIT_UP:
		joypad[which][KEY_UP] = key;
		break;
	case IDC_EDIT_SPEED:
		joypad[which][KEY_BUTTON_SPEED] = key;
		break;
	case IDC_EDIT_CAPTURE:
		joypad[which][KEY_BUTTON_CAPTURE] = key;
		break;
	case IDC_EDIT_DOWN:
		joypad[which][KEY_DOWN] = key;
		break;
	case IDC_EDIT_BUTTON_A:
		joypad[which][KEY_BUTTON_A] = key;
		break;
	case IDC_EDIT_BUTTON_B:
		joypad[which][KEY_BUTTON_B] = key;
		break;
	case IDC_EDIT_BUTTON_L:
		joypad[which][KEY_BUTTON_L] = key;
		break;
	case IDC_EDIT_BUTTON_R:
		joypad[which][KEY_BUTTON_R] = key;
		break;
	case IDC_EDIT_BUTTON_START:
		joypad[which][KEY_BUTTON_START] = key;
		break;
	case IDC_EDIT_BUTTON_SELECT:
		joypad[which][KEY_BUTTON_SELECT] = key;
		break;
	case IDC_EDIT_BUTTON_GS:
		joypad[which][KEY_BUTTON_GS] = key;
		break;
	case IDC_EDIT_AF_UP:
		autoFireKeys[which][KEY_UP] = key;
		break;
	case IDC_EDIT_AF_DOWN:
		autoFireKeys[which][KEY_DOWN] = key;
		break;
	case IDC_EDIT_AF_LEFT:
		autoFireKeys[which][KEY_LEFT] = key;
		break;
	case IDC_EDIT_AF_RIGHT:
		autoFireKeys[which][KEY_RIGHT] = key;
		break;
	case IDC_EDIT_AF_A:
		autoFireKeys[which][KEY_BUTTON_A] = key;
		break;
	case IDC_EDIT_AF_B:
		autoFireKeys[which][KEY_BUTTON_B] = key;
		break;
	case IDC_EDIT_AF_L:
		autoFireKeys[which][KEY_BUTTON_L] = key;
		break;
	case IDC_EDIT_AF_R:
		autoFireKeys[which][KEY_BUTTON_R] = key;
		break;
	case IDC_EDIT_AF_SELECT:
		autoFireKeys[which][KEY_BUTTON_SELECT] = key;
		break;
	case IDC_EDIT_AF_START:
		autoFireKeys[which][KEY_BUTTON_START] = key;
		break;
	}
}

void JoypadConfig::assignKeys()
{
	int id;

	id = IDC_EDIT_UP;
	assignKey(id, (int)GetWindowLongPtr(up, GWLP_USERDATA));

	id = IDC_EDIT_DOWN;
	assignKey(id, (int)GetWindowLongPtr(down, GWLP_USERDATA));

	id = IDC_EDIT_LEFT;
	assignKey(id, (int)GetWindowLongPtr(left, GWLP_USERDATA));

	id = IDC_EDIT_RIGHT;
	assignKey(id, (int)GetWindowLongPtr(right, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_A;
	assignKey(id, (int)GetWindowLongPtr(buttonA, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_B;
	assignKey(id, (int)GetWindowLongPtr(buttonB, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_L;
	assignKey(id, (int)GetWindowLongPtr(buttonL, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_R;
	assignKey(id, (int)GetWindowLongPtr(buttonR, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_SELECT;
	assignKey(id, (int)GetWindowLongPtr(buttonSelect, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_START;
	assignKey(id, (int)GetWindowLongPtr(buttonStart, GWLP_USERDATA));

	id = IDC_EDIT_SPEED;
	assignKey(id, (int)GetWindowLongPtr(speed, GWLP_USERDATA));

	id = IDC_EDIT_CAPTURE;
	assignKey(id, (int)GetWindowLongPtr(capture, GWLP_USERDATA));

	id = IDC_EDIT_BUTTON_GS;
	assignKey(id, (int)GetWindowLongPtr(buttonGS, GWLP_USERDATA));

	id = IDC_EDIT_AF_UP;
	assignKey(id, (int)GetWindowLongPtr(afUp, GWLP_USERDATA));

	id = IDC_EDIT_AF_DOWN;
	assignKey(id, (int)GetWindowLongPtr(afDown, GWLP_USERDATA));

	id = IDC_EDIT_AF_LEFT;
	assignKey(id, (int)GetWindowLongPtr(afLeft, GWLP_USERDATA));

	id = IDC_EDIT_AF_RIGHT;
	assignKey(id, (int)GetWindowLongPtr(afRight, GWLP_USERDATA));

	id = IDC_EDIT_AF_A;
	assignKey(id, (int)GetWindowLongPtr(afA, GWLP_USERDATA));

	id = IDC_EDIT_AF_B;
	assignKey(id, (int)GetWindowLongPtr(afB, GWLP_USERDATA));

	id = IDC_EDIT_AF_L;
	assignKey(id, (int)GetWindowLongPtr(afL, GWLP_USERDATA));

	id = IDC_EDIT_AF_R;
	assignKey(id, (int)GetWindowLongPtr(afR, GWLP_USERDATA));

	id = IDC_EDIT_AF_SELECT;
	assignKey(id, (int)GetWindowLongPtr(afSelect, GWLP_USERDATA));

	id = IDC_EDIT_AF_START;
	assignKey(id, (int)GetWindowLongPtr(afStart, GWLP_USERDATA));

	//  winSaveKeys();
}

/////////////////////////////////////////////////////////////////////////////
// MotionConfig dialog

MotionConfig::MotionConfig(CWnd*pParent /*=NULL*/)
	: CDialog(MotionConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(MotionConfig)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	timerId = 0;
}

void MotionConfig::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MotionConfig)
	DDX_Control(pDX, IDC_EDIT_UP, up);
	DDX_Control(pDX, IDC_EDIT_RIGHT, right);
	DDX_Control(pDX, IDC_EDIT_LEFT, left);
	DDX_Control(pDX, IDC_EDIT_DOWN, down);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(MotionConfig, CDialog)
//{{AFX_MSG_MAP(MotionConfig)
ON_BN_CLICKED(ID_CANCEL, OnCancel)
ON_BN_CLICKED(ID_OK, OnOk)
ON_WM_CHAR()
ON_WM_DESTROY()
ON_WM_KEYDOWN()
ON_WM_TIMER()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MotionConfig message handlers

void MotionConfig::OnCancel()
{
	EndDialog(FALSE);
}

void MotionConfig::OnOk()
{
	assignKeys();
	theApp.input->checkKeys();
	EndDialog(TRUE);
}

void MotionConfig::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{}

void MotionConfig::OnDestroy()
{
	CDialog::OnDestroy();

	KillTimer(timerId);
}

BOOL MotionConfig::OnInitDialog()
{
	CDialog::OnInitDialog();

	timerId = SetTimer(0, 200, NULL);

	if (theApp.input)
		theApp.input->refreshDevices();

	SetWindowLongPtr(up, GWLP_USERDATA, motion[KEY_UP]);
	up.SetWindowText(theApp.input->getKeyName(motion[KEY_UP]));

	SetWindowLongPtr(down, GWLP_USERDATA, motion[KEY_DOWN]);
	down.SetWindowText(theApp.input->getKeyName(motion[KEY_DOWN]));

	SetWindowLongPtr(left, GWLP_USERDATA, motion[KEY_LEFT]);
	left.SetWindowText(theApp.input->getKeyName(motion[KEY_LEFT]));

	SetWindowLongPtr(right, GWLP_USERDATA, motion[KEY_RIGHT]);
	right.SetWindowText(theApp.input->getKeyName(motion[KEY_RIGHT]));

	CenterWindow();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

void MotionConfig::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{}

void MotionConfig::OnTimer(UINT_PTR nIDEvent)
{
	theApp.input->checkDevices();

	CDialog::OnTimer(nIDEvent);
}

void MotionConfig::assignKey(int id, int key)
{
	switch (id)
	{
	case IDC_EDIT_LEFT:
		motion[KEY_LEFT] = key;
		break;
	case IDC_EDIT_RIGHT:
		motion[KEY_RIGHT] = key;
		break;
	case IDC_EDIT_UP:
		motion[KEY_UP] = key;
		break;
	case IDC_EDIT_DOWN:
		motion[KEY_DOWN] = key;
		break;
	}
}

void MotionConfig::assignKeys()
{
	int id;

	id = IDC_EDIT_UP;
	assignKey(id, (int)GetWindowLongPtr(up, GWLP_USERDATA));

	id = IDC_EDIT_DOWN;
	assignKey(id, (int)GetWindowLongPtr(down, GWLP_USERDATA));

	id = IDC_EDIT_LEFT;
	assignKey(id, (int)GetWindowLongPtr(left, GWLP_USERDATA));

	id = IDC_EDIT_RIGHT;
	assignKey(id, (int)GetWindowLongPtr(right, GWLP_USERDATA));
}

