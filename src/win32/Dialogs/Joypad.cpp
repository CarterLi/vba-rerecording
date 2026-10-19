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
ON_NOTIFY(TCN_SELCHANGE, IDC_JOYPAD_TAB, OnSelChangeTab)
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

BOOL JoypadConfig::OnInitDialog()
{
	CDialog::OnInitDialog();

	timerId = SetTimer(0, 200, NULL);

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
	const int joyOnlyLabels[] = {
		IDC_STATIC_SPEED, IDC_STATIC_CAPTURE, IDC_STATIC_GS
	};

	bool autoFirePage = (page == 1);

	for (int i = 0; i < sizeof(joyIds) / sizeof(joyIds[0]); i++)
	{
		CWnd *wnd = GetDlgItem(joyIds[i]);
		if (wnd)
			wnd->ShowWindow(autoFirePage ? SW_HIDE : SW_SHOW);
	}
	for (int i = 0; i < sizeof(afIds) / sizeof(afIds[0]); i++)
	{
		CWnd *wnd = GetDlgItem(afIds[i]);
		if (wnd)
			wnd->ShowWindow(autoFirePage ? SW_SHOW : SW_HIDE);
	}
	for (int i = 0; i < sizeof(joyOnlyLabels) / sizeof(joyOnlyLabels[0]); i++)
	{
		CWnd *wnd = GetDlgItem(joyOnlyLabels[i]);
		if (wnd)
			wnd->ShowWindow(autoFirePage ? SW_HIDE : SW_SHOW);
	}
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

