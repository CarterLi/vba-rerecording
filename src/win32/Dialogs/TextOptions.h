#ifndef VBA_WIN32_TEXT_OPTIONS_H
#define VBA_WIN32_TEXT_OPTIONS_H

#pragma once

#include <map>
#include <set>

// TextOptions dialog

class TextOptions : public CDialog
{
	DECLARE_DYNAMIC(TextOptions)

public:
	TextOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~TextOptions();

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Dialog Data
	enum { IDD = IDD_TEXTCONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedRadioPrefilter();
	afx_msg void OnBnClickedRadioPostfilter();
	afx_msg void OnBnClickedRadioPostrender();
	afx_msg void OnSelchangeFont();

private:
	void collectFonts();
	void rebuildFontList();
	void rebuildSizeList();
	void storeCurrentSelection();
	void applySettings();
	void changeMode();
	void updateFontTip();
	void updateSizeControl();
	void updateMarginControls();
	void populateTooltips();

	int currentMode();
	CString selectedFontName();
	int selectedSize();
	int selectedDisplaySize();

	CToolTipCtrl m_toolTip;

	// The font combo shows one kind of face at a time: TrueType fixed pitch
	// faces for On Screen and raster faces (plus the built-in font) for the
	// game modes.  Both sets are collected once, when the dialog opens.
	std::set<CString> m_screenFonts;
	// The fixed strikes each raster face provides, as rendered pixel height
	// (the label shown in the size combo) -> GDI character height (the value
	// CreateFont and the game renderer need).  Bitmap fonts only exist at a
	// few sizes, so the size list is rebuilt whenever the face changes.
	std::map<CString, std::map<int, int> > m_gameFontSizes;

	CString m_screenFont;
	CString m_gameFont;
	int     m_screenSize;
	int     m_gameSize;
	// The game size the user sees (the rendered pixel height).  m_gameSize is
	// the GDI character height that selects that strike; the two differ by the
	// bitmap's internal leading.
	int     m_gameDisplaySize;
	int     m_currentMode;
	bool    m_listReady;
};

#endif
