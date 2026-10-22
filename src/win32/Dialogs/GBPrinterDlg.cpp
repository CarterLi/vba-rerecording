// GBPrinter.cpp : implementation file
//

#include "../stdafx.h"
#include "../resource.h"
#include "GBPrinterDlg.h"
#include "FileDlg.h"
#include "../Reg.h"
#include "../WinResUtil.h"
#include "../WinMiscUtil.h"
#include "../VBA.h"

//#include "../../common/System.h"
#include "../../NLS.h"
#include "../../common/Util.h"

/////////////////////////////////////////////////////////////////////////////
// GBPrinter dialog

GBPrinterDlg::GBPrinterDlg(CWnd*pParent /*=NULL*/)
	: CDialog(GBPrinterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(GBPrinterDlg)
	m_scale = -1;
	//}}AFX_DATA_INIT
	bitmap = (BITMAPINFO *)bitmapHeader;

	bitmap->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
	bitmap->bmiHeader.biWidth           = 160;
	bitmap->bmiHeader.biHeight          = -144;
	bitmap->bmiHeader.biPlanes          = 1;
	bitmap->bmiHeader.biBitCount        = 8;
	bitmap->bmiHeader.biCompression     = BI_RGB;
	bitmap->bmiHeader.biSizeImage       = 160*144;
	bitmap->bmiHeader.biXPelsPerMeter   = 0;
	bitmap->bmiHeader.biYPelsPerMeter   = 0;
	bitmap->bmiHeader.biClrUsed         = 4;
	bitmap->bmiHeader.biClrImportant    = 4;
	bitmap->bmiColors[0].rgbBlue        =
	    bitmap->bmiColors[0].rgbGreen   =
	        bitmap->bmiColors[0].rgbRed =
	            255;
	bitmap->bmiColors[0].rgbReserved    = 0;
	bitmap->bmiColors[1].rgbBlue        =
	    bitmap->bmiColors[1].rgbGreen   =
	        bitmap->bmiColors[1].rgbRed =
	            168;
	bitmap->bmiColors[1].rgbReserved    = 0;
	bitmap->bmiColors[2].rgbBlue        =
	    bitmap->bmiColors[2].rgbGreen   =
	        bitmap->bmiColors[2].rgbRed =
	            96;
	bitmap->bmiColors[2].rgbReserved    = 0;
	bitmap->bmiColors[3].rgbBlue        =
	    bitmap->bmiColors[3].rgbGreen   =
	        bitmap->bmiColors[3].rgbRed =
	            0;
	bitmap->bmiColors[3].rgbReserved = 0;
}

void GBPrinterDlg::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GBPrinterDlg)
	DDX_Radio(pDX, IDC_1X, m_scale);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(GBPrinterDlg, CDialog)
//{{AFX_MSG_MAP(GBPrinterDlg)
ON_BN_CLICKED(ID_SAVE, OnSave)
ON_BN_CLICKED(ID_PRINT, OnPrint)
ON_BN_CLICKED(ID_OK, OnOk)
ON_BN_CLICKED(IDC_1X, On1x)
ON_BN_CLICKED(IDC_2X, On2x)
ON_BN_CLICKED(IDC_3X, On3x)
ON_BN_CLICKED(IDC_4X, On4x)
ON_WM_PAINT()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GBPrinter message handlers
void GBPrinterDlg::OnSave()
{
	CString name = winImageSaveDlg(this,
	                               "printer",
	                               "",
	                               winResLoadString(IDS_SELECT_CAPTURE_NAME));

	if (name.IsEmpty())
		return;

	// bitmapData is the printed page, 160x144 of 4-bit palette indices; expand
	// it to the BGR the writer wants.
	u8         bgr[160 * 144 * 3];
	const int  stride = 160 * 3;

	for (int y = 0; y < 144; ++y)
	{
		const u8 *source = bitmapData + y * 160;
		u8       *dest   = bgr + y * stride;

		for (int x = 0; x < 160; ++x)
		{
			const RGBQUAD &color = bitmap->bmiColors[source[x]];
			dest[x * 3 + 0] = color.rgbBlue;
			dest[x * 3 + 1] = color.rgbGreen;
			dest[x * 3 + 2] = color.rgbRed;
		}
	}

	winWriteImageBGR(name, 160, 144, bgr, stride);
}

void GBPrinterDlg::OnPrint()
{
	CPrintDialog dlg(FALSE);

	dlg.m_pd.nFromPage = 1;
	dlg.m_pd.nToPage   = 1;
	dlg.m_pd.nMinPage  = 1;
	dlg.m_pd.nMaxPage  = 1;
	dlg.m_pd.nCopies   = 1;

	if (dlg.DoModal() == IDOK)
	{
		DOCINFO di;
		float   fLogPelsX1 = 0;
		float   fLogPelsX2 = 0;
		float   fLogPelsY1 = 0;
		float   fLogPelsY2 = 0;
		float   fScaleX    = 0, fScaleY = 0;
		CDC *   winDC      = NULL;
		memset(&di, 0, sizeof(di));
		di.cbSize = sizeof(DOCINFO);
		CString docName = winResLoadString(IDS_POCKET_PRINTER);
		di.lpszDocName = docName;
		CDC dc;
		dc.Attach(dlg.GetPrinterDC());
		int nError = dc.StartDoc(&di);

		if (nError == SP_ERROR)
		{
			systemMessage(IDS_ERROR_ON_STARTDOC, "Error on StartDoc");
			goto error;
		}
		nError = dc.StartPage();
		if (nError <= 0)
		{
			systemMessage(IDS_ERROR_ON_STARTPAGE, "Error on StartPage");
			goto error;
		}

		winDC      = GetDC();
		fLogPelsX1 = (float)winDC->GetDeviceCaps(LOGPIXELSX);
		fLogPelsY1 = (float)winDC->GetDeviceCaps(LOGPIXELSY);
		ReleaseDC(winDC);

		fLogPelsX2 = (float)dc.GetDeviceCaps(LOGPIXELSX);
		fLogPelsY2 = (float)dc.GetDeviceCaps(LOGPIXELSY);

		if (fLogPelsX1 > fLogPelsX2)
			fScaleX = fLogPelsX1 / fLogPelsX2;
		else
			fScaleX = fLogPelsX2 / fLogPelsX1;

		if (fLogPelsY1 > fLogPelsY2)
			fScaleY = fLogPelsY1 / fLogPelsY2;
		else
			fScaleY = fLogPelsY2 / fLogPelsY1;

		fScaleX *= (scale+1);
		fScaleY *= (scale+1);

		if (StretchDIBits(dc,
		                  0,
		                  0,
		                  (int)((float)160*fScaleX),
		                  (int)((float)144*fScaleY),
		                  0,
		                  0,
		                  160,
		                  144,
		                  bitmapData,
		                  bitmap,
		                  DIB_RGB_COLORS,
		                  SRCCOPY) == GDI_ERROR)
		{
			systemMessage(IDS_ERROR_PRINTING_ON_STRETCH,
			              "Error printing on StretchDIBits");
		}

		nError = dc.EndPage();

		if (nError <= 0)
		{
			systemMessage(IDS_ERROR_ON_ENDPAGE, "Error on EndPage");
			goto error;
		}

		nError = dc.EndDoc();

		if (nError <= 0)
			systemMessage(IDS_ERROR_ON_ENDDOC, "Error on EndDoc");
error:
		dc.DeleteDC();
	}
}

void GBPrinterDlg::processData(u8 *data)
{
	for (int y = 0; y < 0x12; y++)
	{
		for (int x = 0; x < 0x14; x++)
		{
			for (int k = 0; k < 8; k++)
			{
				int a = *data++;
				int b = *data++;
				for (int j = 0; j < 8; j++)
				{
					int mask = 1 << (7-j);
					int c    = 0;
					if (a & mask)
						c++;
					if (b & mask)
						c += 2;
					bitmapData[x*8+j + 160*(y*8+k)] = c;
				}
			}
		}
	}
}

BOOL GBPrinterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	scale = regQueryDwordValue(_T("printerScale"), 0);
	if (scale < 0 || scale > 3)
		scale = 0;
	m_scale = scale;
	UpdateData(FALSE);

	CenterWindow();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

void GBPrinterDlg::OnOk()
{
	EndDialog(TRUE);
}

void GBPrinterDlg::On1x()
{
	regSetDwordValue(_T("printerScale"), 0);
	scale = 0;
}

void GBPrinterDlg::On2x()
{
	regSetDwordValue(_T("printerScale"), 1);
	scale = 1;
}

void GBPrinterDlg::On3x()
{
	regSetDwordValue(_T("printerScale"), 2);
	scale = 2;
}

void GBPrinterDlg::On4x()
{
	regSetDwordValue(_T("printerScale"), 3);
	scale = 3;
}

void GBPrinterDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	RECT  rect;
	CWnd *h = GetDlgItem(IDC_GB_PRINTER);
	h->GetWindowRect(&rect);
	POINT p;
	p.x = rect.left;
	p.y = rect.top;
	ScreenToClient((POINT *)&p);
	rect.left = p.x+1;
	rect.top  = p.y+1;
	p.x       = rect.right;
	p.y       = rect.bottom;
	ScreenToClient((POINT *)&p);
	rect.right  = p.x-1;
	rect.bottom = p.y-1;

	StretchDIBits(dc,
	              rect.left,
	              rect.top,
	              rect.right - rect.left,
	              rect.bottom - rect.top,
	              0,
	              0,
	              160,
	              144,
	              bitmapData,
	              bitmap,
	              DIB_RGB_COLORS,
	              SRCCOPY);
}

void systemGbPrint(u8 *data,
                   int pages,
                   int feed,
                   int palette,
                   int contrast)
{
	theApp.winCheckFullscreen();
	GBPrinterDlg printer;
	printer.processData(data);
	printer.DoModal();
}

