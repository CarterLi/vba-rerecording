// TextOptions.cpp : implementation file
//

#include "../stdafx.h"
#include "../../common/Text.h"
#include "../GameText.h"
#include "../resource.h"
#include "../VBA.h"
#include "../WinResUtil.h"
#include "TextOptions.h"

#include <cstdlib>

// TextOptions dialog

IMPLEMENT_DYNAMIC(TextOptions, CDialog)
TextOptions::TextOptions(CWnd*pParent /*=NULL*/)
	: CDialog(TextOptions::IDD, pParent)
{
	m_screenSize      = 12;
	m_gameSize        = 8;
	m_gameDisplaySize = 0;
	m_currentMode     = 1;
	m_listReady       = false;
}

TextOptions::~TextOptions()
{}

// The On Screen overlay draws with GDI, so a scalable fixed pitch face is what
// it wants.  Bitmap faces are excluded here; they have their own list for the
// In Game and On Game modes.
static int CALLBACK enumScreenFont(const LOGFONT *lf, const TEXTMETRIC *tm,
                                   DWORD fontType, LPARAM lParam)
{
	if (lf->lfFaceName[0] == _T('@'))
		return 1;

	if ((fontType & RASTER_FONTTYPE) != 0)
		return 1;

	if ((tm->tmPitchAndFamily & TMPF_FIXED_PITCH) != 0)
		return 1;

	((std::set<CString> *)lParam)->insert(CString(lf->lfFaceName));
	return 1;
}

struct GameFontFamilies
{
	std::set<CString> *names;
};

struct GameFontStrikes
{
	std::set<int> *heights;
};

// The In Game and On Game modes rasterise a bitmap face into the emulated
// frame.  Only raster fonts are offered: they are pixel fonts already, so they
// stay crisp and need no antialiasing.
static int CALLBACK enumGameFamily(const LOGFONT *lf, const TEXTMETRIC *tm,
                                   DWORD fontType, LPARAM lParam)
{
	if (lf->lfFaceName[0] == _T('@'))
		return 1;

	if ((fontType & RASTER_FONTTYPE) == 0)
		return 1;

	((GameFontFamilies *)lParam)->names->insert(CString(lf->lfFaceName));
	return 1;
}

// One call per bitmap strike of the face.  lfHeight is the character height
// GDI expects, which is tmHeight without the internal leading; that is what
// CreateFont() and the game renderer ask for.
static int CALLBACK enumGameStrike(const LOGFONT *lf, const TEXTMETRIC *tm,
                                   DWORD fontType, LPARAM lParam)
{
	if ((fontType & RASTER_FONTTYPE) == 0)
		return 1;

	const int height = tm->tmHeight - tm->tmInternalLeading;
	if (height > 0)
		((GameFontStrikes *)lParam)->heights->insert(height);

	return 1;
}

// Asks GDI which strike a character height really selects.  A family can
// advertise several character sets or near-duplicate heights, and two of them
// can map to the same bitmap; the probe keeps the size list to the strikes the
// renderer can actually reach.
static int probeRasterHeight(HDC dc, const CString &face, int height)
{
	HFONT font = CreateFontW(-height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
	                         DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
	                         NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
	                         face.GetString());
	if (font == NULL)
		return 0;

	HGDIOBJ oldFont = SelectObject(dc, font);
	TEXTMETRICW tm;
	const int result = GetTextMetricsW(dc, &tm) ? tm.tmHeight : 0;
	SelectObject(dc, oldFont);
	DeleteObject(font);
	return result;
}

void TextOptions::collectFonts()
{
	m_screenFonts.clear();
	m_gameFontSizes.clear();

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfCharSet        = DEFAULT_CHARSET;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	CClientDC dc(this);
	EnumFontFamiliesEx(dc.GetSafeHdc(), &lf, enumScreenFont, (LPARAM)&m_screenFonts, 0);

	// EnumFontFamiliesEx with an empty face name stops at the first strike of
	// each raster family.  Ask once for the names, then ask again per face to
	// get every bitmap size it provides.
	std::set<CString> gameNames;
	GameFontFamilies families;
	families.names = &gameNames;
	EnumFontFamiliesEx(dc.GetSafeHdc(), &lf, enumGameFamily, (LPARAM)&families, 0);

	HDC probeDC = CreateCompatibleDC(NULL);
	for (std::set<CString>::const_iterator it = gameNames.begin(); it != gameNames.end(); ++it)
	{
		LOGFONT faceLf = lf;
		lstrcpynW(faceLf.lfFaceName, *it, LF_FACESIZE);

		std::set<int> candidates;
		GameFontStrikes strikes;
		strikes.heights = &candidates;
		EnumFontFamiliesEx(dc.GetSafeHdc(), &faceLf, enumGameStrike, (LPARAM)&strikes, 0);

		std::map<int, int> &sizes = m_gameFontSizes[*it];
		if (probeDC == NULL)
		{
			for (std::set<int>::const_iterator size = candidates.begin();
			     size != candidates.end(); ++size)
				sizes[*size] = *size;
		}
		else
		{
			// Two character heights can select the same bitmap.  Keep one
			// entry per reachable strike, preferring the height that equals
			// the strike's rendered height when the face offers it.
			std::map<int, int> renderedToHeight;
			for (std::set<int>::const_iterator size = candidates.begin();
			     size != candidates.end(); ++size)
			{
				const int rendered = probeRasterHeight(probeDC, *it, *size);
				if (rendered <= 0)
					continue;

				std::map<int, int>::iterator found = renderedToHeight.find(rendered);
				if (found == renderedToHeight.end() || *size == rendered)
					renderedToHeight[rendered] = *size;
			}

			// Label each strike with the height it actually renders at, but
			// remember the character height needed to select it.
			sizes = renderedToHeight;
		}

		if (sizes.empty())
			sizes[8] = 8;
	}
	if (probeDC != NULL)
		DeleteDC(probeDC);
}

int TextOptions::currentMode()
{
	int mode = GetCheckedRadioButton(IDC_RADIO_PREFILTER, IDC_RADIO_POSTRENDER) - IDC_RADIO_PREFILTER;
	if (mode < 0 || mode > 2)
		mode = 1;
	return mode;
}

CString TextOptions::selectedFontName()
{
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_TEXT_FONT);
	if (combo == NULL)
		return CString();

	const int index = combo->GetCurSel();
	if (index == CB_ERR)
		return CString();

	if (m_currentMode != 2 && index == 0)
		return CString(GAME_TEXT_FONT_BUILTIN);

	CString name;
	combo->GetLBText(index, name);
	return name;
}

int TextOptions::selectedSize()
{
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_TEXT_SIZE);
	if (combo != NULL)
	{
		const int index = combo->GetCurSel();
		if (index != CB_ERR)
			return (int)combo->GetItemData(index);
	}

	CString text;
	GetDlgItemText(IDC_TEXT_SIZE, text);
	return _ttoi(text);
}

int TextOptions::selectedDisplaySize()
{
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_TEXT_SIZE);
	if (combo != NULL)
	{
		const int index = combo->GetCurSel();
		if (index != CB_ERR)
		{
			CString text;
			combo->GetLBText(index, text);
			return _ttoi(text);
		}
	}

	CString text;
	GetDlgItemText(IDC_TEXT_SIZE, text);
	return _ttoi(text);
}

void TextOptions::rebuildFontList()
{
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_TEXT_FONT);
	if (combo == NULL)
		return;

	combo->ResetContent();

	if (m_currentMode == 2)
	{
		if (m_screenFonts.empty())
		{
			combo->AddString(_T("Courier New"));
		}
		else
		{
			for (std::set<CString>::const_iterator it = m_screenFonts.begin();
			     it != m_screenFonts.end(); ++it)
				combo->AddString(*it);
		}

		int index = combo->FindStringExact(-1, m_screenFont);
		if (index == CB_ERR && combo->GetCount() > 0)
			index = 0;
		if (index != CB_ERR)
			combo->SetCurSel(index);
		return;
	}

	// The built-in 8x8 font is always the first entry.  The stored value is
	// the stable GAME_TEXT_FONT_BUILTIN string, whatever the label says.
	CString builtinLabel = winResLoadString(IDS_TEXT_FONT_BUILTIN);
	if (builtinLabel.IsEmpty())
		builtinLabel = GAME_TEXT_FONT_BUILTIN;
	combo->AddString(builtinLabel);

	for (std::map<CString, std::map<int, int> >::const_iterator it = m_gameFontSizes.begin();
	     it != m_gameFontSizes.end(); ++it)
		combo->AddString(it->first);

	int index = 0;
	if (m_gameFont.Compare(GAME_TEXT_FONT_BUILTIN) != 0)
	{
		const int found = combo->FindStringExact(-1, m_gameFont);
		if (found != CB_ERR)
			index = found;
	}
	combo->SetCurSel(index);
}

void TextOptions::rebuildSizeList()
{
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_TEXT_SIZE);
	if (combo == NULL)
		return;

	combo->ResetContent();

	// On Screen uses an edit box for the TrueType height, so there is no list
	// to fill.  Only the In Game / On Game modes offer a list, because their
	// bitmap faces have a few fixed strikes.
	if (m_currentMode == 2)
		return;

	// The combo shows the rendered pixel height of each strike but stores the
	// GDI character height that selects it, so the size list stays readable.
	std::map<int, int> sizes;
	int reference;

	const CString face = selectedFontName();
	if (face.Compare(GAME_TEXT_FONT_BUILTIN) == 0)
	{
		// The built-in font only has its native 8 pixel size.
		sizes[8] = 8;
	}
	else
	{
		std::map<CString, std::map<int, int> >::const_iterator it = m_gameFontSizes.find(face);
		if (it != m_gameFontSizes.end())
			sizes = it->second;
	}

	if (sizes.empty())
		sizes[8] = 8;

	// Keep the visible size across a font switch, not the character
	// height, so "20" stays "20" even when the new face needs a
	// different height to select its 20 pixel strike.
	reference = m_gameDisplaySize;
	if (reference <= 0)
	{
		for (std::map<int, int>::const_iterator it = sizes.begin();
		     it != sizes.end(); ++it)
		{
			if (it->second == m_gameSize)
			{
				reference = it->first;
				break;
			}
		}
		if (reference <= 0)
			reference = m_gameSize;
	}

	int best = 0;
	int bestDiff = -1;
	int index = 0;
	for (std::map<int, int>::const_iterator it = sizes.begin(); it != sizes.end(); ++it, ++index)
	{
		CString text;
		text.Format(_T("%d"), it->first);
		combo->AddString(text);
		combo->SetItemData(index, (DWORD_PTR)it->second);

		const int diff = abs(it->first - reference);
		if (bestDiff < 0 || diff < bestDiff)
		{
			best = index;
			bestDiff = diff;
		}
	}

	combo->SetCurSel(best);

	const int chosen = (int)combo->GetItemData(best);
	if (chosen > 0)
	{
		CString chosenText;
		combo->GetLBText(best, chosenText);
		const int chosenDisplay = _ttoi(chosenText);

		m_gameSize = chosen;
		m_gameDisplaySize = chosenDisplay > 0 ? chosenDisplay : chosen;
	}
}

void TextOptions::storeCurrentSelection()
{
	if (!m_listReady)
		return;

	const CString font = selectedFontName();

	if (m_currentMode == 2)
	{
		if (!font.IsEmpty())
			m_screenFont = font;

		// The On Screen size is typed into an edit box; keep it inside the
		// range the overlay accepts.
		BOOL translated = FALSE;
		const int size = (int)GetDlgItemInt(IDC_TEXT_SCREEN_SIZE, &translated, FALSE);
		if (translated && size > 0)
		{
			if (size < SCREEN_TEXT_SIZE_MIN)
				m_screenSize = SCREEN_TEXT_SIZE_MIN;
			else if (size > SCREEN_TEXT_SIZE_MAX)
				m_screenSize = SCREEN_TEXT_SIZE_MAX;
			else
				m_screenSize = size;
		}
	}
	else
	{
		const int size = selectedSize();
		const int displaySize = selectedDisplaySize();

		if (!font.IsEmpty())
			m_gameFont = font;
		if (size > 0)
			m_gameSize = size;
		if (displaySize > 0)
			m_gameDisplaySize = displaySize;
	}
}

void TextOptions::updateSizeControl()
{
	const bool screen = m_currentMode == 2;

	GetDlgItem(IDC_TEXT_SIZE)->ShowWindow(screen ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_TEXT_SCREEN_SIZE)->ShowWindow(screen ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_TEXT_SCREEN_SIZE_SPIN)->ShowWindow(screen ? SW_SHOW : SW_HIDE);

	if (!screen)
		return;

	SetDlgItemInt(IDC_TEXT_SCREEN_SIZE, (UINT)m_screenSize, FALSE);

	CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_TEXT_SCREEN_SIZE_SPIN);
	if (spin != NULL)
	{
		spin->SetRange32(SCREEN_TEXT_SIZE_MIN, SCREEN_TEXT_SIZE_MAX);
		spin->SetPos32(m_screenSize);
	}
}

void TextOptions::updateMarginControls()
{
	const int edits[2] = { IDC_TEXT_MARGIN_LEFT, IDC_TEXT_MARGIN_BOTTOM };
	const int spins[2] = { IDC_TEXT_MARGIN_LEFT_SPIN, IDC_TEXT_MARGIN_BOTTOM_SPIN };

	for (int i = 0; i < 2; i++)
	{
		BOOL translated = FALSE;
		UINT value = GetDlgItemInt(edits[i], &translated, FALSE);
		if (!translated)
			value = (UINT)(i == 0 ? theApp.textMarginLeft : theApp.textMarginBottom);

		if (value > (UINT)TEXT_MARGIN_MAX)
			value = (UINT)TEXT_MARGIN_MAX;

		SetDlgItemInt(edits[i], value, FALSE);

		CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(spins[i]);
		if (spin != NULL)
		{
			spin->SetRange32(TEXT_MARGIN_MIN, TEXT_MARGIN_MAX);
			spin->SetPos32((int)value);
		}
	}
}

void TextOptions::changeMode()
{
	storeCurrentSelection();

	m_currentMode = currentMode();
	rebuildFontList();
	rebuildSizeList();
	updateSizeControl();
	updateFontTip();
}

void TextOptions::updateFontTip()
{
	if (m_toolTip.GetSafeHwnd() == NULL)
		return;

	const UINT id = m_currentMode == 2
		? IDS_TEXT_SCREEN_FONT_TIP
		: IDS_TEXT_GAME_FONT_TIP;
	m_toolTip.UpdateTipText(winResLoadString(id), GetDlgItem(IDC_TEXT_FONT));
}

void TextOptions::populateTooltips()
{
	if (!m_toolTip.Create(this))
		return;

	m_toolTip.AddTool(GetDlgItem(IDC_RADIO_PREFILTER),
	                  winResLoadString(IDS_TEXT_PREFILTER_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_RADIO_POSTFILTER),
	                  winResLoadString(IDS_TEXT_POSTFILTER_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_RADIO_POSTRENDER),
	                  winResLoadString(IDS_TEXT_POSTRENDER_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_TEXT_FONT),
	                  winResLoadString(IDS_TEXT_GAME_FONT_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_TEXT_SIZE),
	                  winResLoadString(IDS_TEXT_SIZE_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_TEXT_SCREEN_SIZE),
	                  winResLoadString(IDS_TEXT_SIZE_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_TEXT_MARGIN_LEFT),
	                  winResLoadString(IDS_TEXT_MARGIN_LEFT_TIP));
	m_toolTip.AddTool(GetDlgItem(IDC_TEXT_MARGIN_BOTTOM),
	                  winResLoadString(IDS_TEXT_MARGIN_BOTTOM_TIP));
	m_toolTip.Activate(TRUE);

	updateFontTip();
}

BOOL TextOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	CheckRadioButton(IDC_RADIO_WHITE, IDC_RADIO_BLACK, IDC_RADIO_WHITE + textColor);
	CheckRadioButton(IDC_RADIO_PREFILTER, IDC_RADIO_POSTRENDER, IDC_RADIO_PREFILTER + textMethod);
	CheckDlgButton(IDC_CHECK_OUTLINED, outlinedText);
	CheckDlgButton(IDC_CHECK_TRANSPARENT, transparentText);
	GetDlgItem(IDC_CHECK_TRANSPARENT)->EnableWindow(GetCheckedRadioButton(IDC_RADIO_PREFILTER,
	                                                                      IDC_RADIO_POSTRENDER) != IDC_RADIO_POSTRENDER);

	m_currentMode = currentMode();

	m_screenFont = theApp.screenTextFont;
	if (m_screenFont.IsEmpty())
		m_screenFont = _T("Consolas");

	m_gameFont = theApp.gameTextFont;
	if (m_gameFont.IsEmpty())
		m_gameFont = GAME_TEXT_FONT_BUILTIN;

	m_screenSize = theApp.screenTextSize;
	if (m_screenSize < SCREEN_TEXT_SIZE_MIN || m_screenSize > SCREEN_TEXT_SIZE_MAX)
		m_screenSize = 12;

	m_gameSize = theApp.gameTextSize;
	if (m_gameSize < 1 || m_gameSize > 128)
		m_gameSize = 8;
	m_gameDisplaySize = 0;

	updateMarginControls();

	collectFonts();
	rebuildFontList();
	rebuildSizeList();
	updateSizeControl();
	m_listReady = true;
	populateTooltips();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void TextOptions::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL TextOptions::PreTranslateMessage(MSG* pMsg)
{
	if (m_toolTip.GetSafeHwnd() != NULL)
		m_toolTip.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(TextOptions, CDialog)
ON_BN_CLICKED(IDOK, OnBnClickedOk)
ON_BN_CLICKED(IDC_TEXT_APPLY, OnBnClickedApply)
ON_BN_CLICKED(IDC_RADIO_PREFILTER, OnBnClickedRadioPrefilter)
ON_BN_CLICKED(IDC_RADIO_POSTFILTER, OnBnClickedRadioPostfilter)
ON_BN_CLICKED(IDC_RADIO_POSTRENDER, OnBnClickedRadioPostrender)
ON_CBN_SELCHANGE(IDC_TEXT_FONT, OnSelchangeFont)
END_MESSAGE_MAP()

// TextOptions message handlers

void TextOptions::applySettings()
{
	transparentText = IsDlgButtonChecked(IDC_CHECK_TRANSPARENT) != 0;
	outlinedText    = IsDlgButtonChecked(IDC_CHECK_OUTLINED) != 0;
	textMethod      = GetCheckedRadioButton(IDC_RADIO_PREFILTER, IDC_RADIO_POSTRENDER) - IDC_RADIO_PREFILTER;
	textColor       = GetCheckedRadioButton(IDC_RADIO_WHITE, IDC_RADIO_BLACK) - IDC_RADIO_WHITE;
	if (textMethod < 0)
		textMethod = 0;
	if (textMethod > 2)
		textMethod = 2;
	if (textColor < 0)
		textColor = 0;
	if (textColor > 7)
		textColor = 7;

	storeCurrentSelection();

	theApp.screenTextFont = m_screenFont;
	theApp.gameTextFont   = m_gameFont;
	theApp.screenTextSize = m_screenSize;
	theApp.gameTextSize   = m_gameSize;

	BOOL translated = FALSE;
	UINT leftMargin = GetDlgItemInt(IDC_TEXT_MARGIN_LEFT, &translated, FALSE);
	if (translated)
	{
		if (leftMargin > (UINT)TEXT_MARGIN_MAX)
			leftMargin = (UINT)TEXT_MARGIN_MAX;
		theApp.textMarginLeft = (int)leftMargin;
	}
	UINT bottomMargin = GetDlgItemInt(IDC_TEXT_MARGIN_BOTTOM, &translated, FALSE);
	if (translated)
	{
		if (bottomMargin > (UINT)TEXT_MARGIN_MAX)
			bottomMargin = (UINT)TEXT_MARGIN_MAX;
		theApp.textMarginBottom = (int)bottomMargin;
	}

	// Show the clamped size if the user typed one outside the range.
	updateSizeControl();
	updateMarginControls();
}

void TextOptions::OnBnClickedOk()
{
	applySettings();
	theApp.saveSettings();
	OnOK();
}

void TextOptions::OnBnClickedApply()
{
	applySettings();
	theApp.saveSettings();
	systemRefreshScreen();
}

void TextOptions::OnBnClickedRadioPrefilter()
{
	GetDlgItem(IDC_CHECK_TRANSPARENT)->EnableWindow(TRUE);
	changeMode();
}

void TextOptions::OnBnClickedRadioPostfilter()
{
	GetDlgItem(IDC_CHECK_TRANSPARENT)->EnableWindow(TRUE);
	changeMode();
}

void TextOptions::OnBnClickedRadioPostrender()
{
	GetDlgItem(IDC_CHECK_TRANSPARENT)->EnableWindow(FALSE);
	CheckDlgButton(IDC_CHECK_TRANSPARENT, FALSE);
	changeMode();
}

void TextOptions::OnSelchangeFont()
{
	// Game faces have a few fixed strikes; keep the current size as the
	// reference and pick the closest strike the new face provides.  On Screen
	// sizes are typed in, so its size does not depend on the face.
	storeCurrentSelection();
	if (m_currentMode != 2)
		rebuildSizeList();
}
