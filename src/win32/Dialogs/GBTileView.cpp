// GBTileView.cpp : implementation file
//

#include "../stdafx.h"
#include "../resource.h"
#include "GBTileView.h"
#include "FileDlg.h"
#include "../Reg.h"
#include "../WinResUtil.h"
#include "../WinMiscUtil.h"
#include "../VBA.h"

//#include "../../common/System.h"
#include "../../gb/gbGlobals.h"
#include "../../NLS.h"
#include "../../common/Util.h"

/////////////////////////////////////////////////////////////////////////////
// GBTileView dialog

GBTileView::GBTileView(CWnd*pParent /*=NULL*/)
	: ResizeDlg(GBTileView::IDD, pParent)
{
	//{{AFX_DATA_INIT(GBTileView)
	m_charBase = -1;
	m_bank     = -1;
	m_stretch  = FALSE;
	//}}AFX_DATA_INIT
	autoUpdate = false;

	memset(&bmpInfo, 0, sizeof(bmpInfo));

	bmpInfo.bmiHeader.biSize        = sizeof(bmpInfo.bmiHeader);
	bmpInfo.bmiHeader.biWidth       = 32*8;
	bmpInfo.bmiHeader.biHeight      = 32*8;
	bmpInfo.bmiHeader.biPlanes      = 1;
	bmpInfo.bmiHeader.biBitCount    = 24;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	data = (u8 *)calloc(1, 3 * 32*32 * 64);

	tileView.setData(data);
	tileView.setBmpInfo(&bmpInfo);

	charBase = 0;
	palette  = 0;
	bank     = 0;
	w        = h = 0;
}

GBTileView::~GBTileView()
{
	free(data);
	data = NULL;
}

void GBTileView::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GBTileView)
	DDX_Control(pDX, IDC_PALETTE_SLIDER, m_slider);
	DDX_Radio(pDX, IDC_CHARBASE_0, m_charBase);
	DDX_Radio(pDX, IDC_BANK_0, m_bank);
	DDX_Check(pDX, IDC_STRETCH, m_stretch);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_TILE_VIEW, tileView);
	DDX_Control(pDX, IDC_MAP_VIEW_ZOOM, zoom);
	DDX_Control(pDX, IDC_COLOR, color);
}

BEGIN_MESSAGE_MAP(GBTileView, CDialog)
//{{AFX_MSG_MAP(GBTileView)
ON_BN_CLICKED(IDC_SAVE, OnSave)
ON_BN_CLICKED(IDC_CLOSE, OnClose)
ON_BN_CLICKED(IDC_AUTO_UPDATE, OnAutoUpdate)
ON_BN_CLICKED(IDC_CHARBASE_0, OnCharbase0)
ON_BN_CLICKED(IDC_CHARBASE_1, OnCharbase1)
ON_BN_CLICKED(IDC_BANK_0, OnBank0)
ON_BN_CLICKED(IDC_BANK_1, OnBank1)
ON_BN_CLICKED(IDC_STRETCH, OnStretch)
ON_WM_HSCROLL()
//}}AFX_MSG_MAP
ON_MESSAGE(WM_MAPINFO, OnMapInfo)
ON_MESSAGE(WM_COLINFO, OnColInfo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GBTileView message handlers
void GBTileView::OnSave()
{
	CString name = winImageSaveDlg(this,
	                               "tiles",
	                               "",
	                               winResLoadString(IDS_SELECT_CAPTURE_NAME));

	if (name.IsEmpty())
		return;

	winWriteImageBGR(name, w, h, data, 3 * w);
}

void GBTileView::renderTile(int tile, int x, int y, u8 *charBase)
{
	u8 *bmp = &data[24*x + 8*16*24*y];

	for (int j = 0; j < 8; j++)
	{
		u8 mask   = 0x80;
		u8 tile_a = charBase[tile*16+j*2];
		u8 tile_b = charBase[tile*16+j*2+1];

		for (int i = 0; i < 8; i++)
		{
			u8 c = (tile_a & mask) ? 1 : 0;
			c += ((tile_b & mask) ? 2 : 0);

			if (gbCgbMode)
			{
				c = c + palette*4;
			}
			else
			{
				c = gbBgp[c];
			}

			u16 color = gbPalette[c];

			*bmp++ = ((color >> 10) & 0x1f) << 3;
			*bmp++ = ((color >> 5) & 0x1f) << 3;
			*bmp++ = (color & 0x1f) << 3;

			mask >>= 1;
		}
		bmp += 15*24; // advance line
	}
}

void GBTileView::render()
{
	int tiles = 0x0000;
	if (charBase)
		tiles = 0x0800;
	u8 *charBase = (gbVram != NULL) ?
	               (bank ? &gbVram[0x2000+tiles] : &gbVram[tiles]) :
	               &gbMemory[0x8000+tiles];

	int tile = 0;
	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			renderTile(tile, x, y, charBase);
			tile++;
		}
	}
	tileView.setSize(16*8, 16*8);
	w = 16*8;
	h = 16*8;
	SIZE s;
	s.cx = s.cy = 16*8;
	if (tileView.getStretch())
	{
		s.cx = s.cy = 1;
	}
	tileView.SetScrollSizes(MM_TEXT, s);
}

void GBTileView::update()
{
	paint();
}

BOOL GBTileView::OnInitDialog()
{
	CDialog::OnInitDialog();

	DIALOG_SIZER_START(sz)
	DIALOG_SIZER_ENTRY(IDC_TILE_VIEW, DS_SizeX | DS_SizeY)
	DIALOG_SIZER_ENTRY(IDC_COLOR, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_R, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_G, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_B, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_REFRESH, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_CLOSE, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_SAVE, DS_MoveY)
	DIALOG_SIZER_END()
	SetData(sz,
	        TRUE,
	        HKEY_CURRENT_USER,
	        "Software\\Emulators\\VisualBoyAdvance\\Viewer\\GBTileView",
	        NULL);

	m_charBase = charBase;
	m_bank     = bank;

	m_slider.SetRange(0, 7);
	m_slider.SetPageSize(2);
	m_slider.SetTicFreq(1);
	paint();

	m_stretch = regQueryDwordValue("tileViewStretch", 0);
	if (m_stretch)
		tileView.setStretch(true);
	UpdateData(FALSE);

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

void GBTileView::OnClose()
{
	theApp.winRemoveUpdateListener(this);

	DestroyWindow();
}

void GBTileView::OnAutoUpdate()
{
	autoUpdate = !autoUpdate;
	if (autoUpdate)
	{
		theApp.winAddUpdateListener(this);
	}
	else
	{
		theApp.winRemoveUpdateListener(this);
	}
}

void GBTileView::paint()
{
	if (gbRom != NULL)
	{
		render();
		tileView.refresh();
	}
}

void GBTileView::OnCharbase0()
{
	charBase = 0;
	paint();
}

void GBTileView::OnCharbase1()
{
	charBase = 1;
	paint();
}

void GBTileView::OnBank0()
{
	bank = 0;
	paint();
}

void GBTileView::OnBank1()
{
	bank = 1;
	paint();
}

void GBTileView::OnStretch()
{
	tileView.setStretch(!tileView.getStretch());
	paint();
	regSetDwordValue("tileViewStretch", tileView.getStretch());
}

LRESULT GBTileView::OnMapInfo(WPARAM wParam, LPARAM lParam)
{
	u8 *colors = (u8 *)lParam;
	zoom.setColors(colors);

	int x = (wParam & 0xFFFF)/8;
	int y = ((wParam >> 16) & 0xFFFF)/8;

	int tiles = 0x0000;
	if (charBase)
		tiles = 0x0800;
	u32 address = 0x8000 + tiles;
	int tile    = 16 * y + x;

	address += 16 * tile;

	CString buffer;
	buffer.Format("%d", tile);
	GetDlgItem(IDC_TILE_NUMBER)->SetWindowText(buffer);

	buffer.Format("%04x", address);
	GetDlgItem(IDC_ADDRESS)->SetWindowText(buffer);

	return TRUE;
}

LRESULT GBTileView::OnColInfo(WPARAM wParam, LPARAM)
{
	u16 c = (u16)wParam;

	color.setColor(c);

	int r = (c & 0x1f);
	int g = (c & 0x3e0) >> 5;
	int b = (c & 0x7c00) >> 10;

	CString buffer;
	buffer.Format("R: %d", r);
	GetDlgItem(IDC_R)->SetWindowText(buffer);

	buffer.Format("G: %d", g);
	GetDlgItem(IDC_G)->SetWindowText(buffer);

	buffer.Format("B: %d", b);
	GetDlgItem(IDC_B)->SetWindowText(buffer);

	return TRUE;
}

void GBTileView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*pScrollBar)
{
	switch (nSBCode)
	{
	case TB_THUMBPOSITION:
		palette = nPos;
		break;
	default:
		palette = m_slider.GetPos();
		break;
	}
	paint();
}

void GBTileView::PostNcDestroy()
{
	delete this;
}

