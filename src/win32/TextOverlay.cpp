#include "stdafx.h"

#include <vector>

#include "TextOverlay.h"
#include "VBA.h"

#include "../common/Text.h"

static COLORREF overlayTextColor(int index)
{
	switch (index)
	{
	case 0:  return RGB(255, 255, 255);
	case 1:  return RGB(255, 0, 0);
	case 2:  return RGB(255, 255, 0);
	case 3:  return RGB(0, 255, 0);
	case 4:  return RGB(0, 255, 255);
	case 5:  return RGB(0, 0, 255);
	case 6:  return RGB(255, 0, 255);
	case 7:  return RGB(0, 0, 0);
	default: return RGB(255, 255, 255);
	}
}

int screenTextFontHeight()
{
	int height = theApp.screenTextSize;
	if (height < SCREEN_TEXT_SIZE_MIN)
		height = SCREEN_TEXT_SIZE_MIN;
	if (height > SCREEN_TEXT_SIZE_MAX)
		height = SCREEN_TEXT_SIZE_MAX;
	return height;
}

int screenTextLineHeight()
{
	// Six pixels of leading keep the default 12 pixel font on the same
	// comfortable row the historical 20 pixel On Screen layout had, and give
	// larger fonts the room they need.
	return screenTextFontHeight() + 6;
}

HFONT createScreenTextFont()
{
	CString face = theApp.screenTextFont;
	if (face.IsEmpty())
		face = _T("Consolas");

	// The On Screen font list only offers scalable faces, so asking for
	// ClearType here gets the system antialiasing the user expects.  The
	// renderer copies the coverage out of a black mask instead of letting
	// GDI touch the transparent overlay, which keeps the edges soft.
	return CreateFont(-screenTextFontHeight(), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
	                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}

TextOverlay::TextOverlay()
{
	dc           = NULL;
	dib          = NULL;
	oldBitmap    = NULL;
	dibBits      = NULL;
	overlayWidth = 0;
	overlayHeight = 0;
	overlayPitch = 0;
	anyVisible   = false;
	contentChanged = false;
	haveSignature  = false;
	lastSignature  = 0;
}

TextOverlay::~TextOverlay()
{
	destroy();
}

void TextOverlay::destroy()
{
	if (dc != NULL)
	{
		if (oldBitmap != NULL)
			SelectObject(dc, oldBitmap);
		DeleteDC(dc);
		dc = NULL;
	}
	if (dib != NULL)
	{
		DeleteObject(dib);
		dib = NULL;
	}
	oldBitmap     = NULL;
	dibBits       = NULL;
	overlayWidth  = 0;
	overlayHeight = 0;
	overlayPitch  = 0;

	// A new backing store has nothing in common with the one the signature was
	// taken from, so the next update() renders whatever is pending.
	contentChanged = false;
	haveSignature  = false;
	lastSignature  = 0;
}

bool TextOverlay::create(int width, int height)
{
	if (width <= 0 || height <= 0)
		return false;

	if (dc != NULL && width == overlayWidth && height == overlayHeight)
	{
		// The backing store stays, but the caller is usually about to make a
		// new texture of it, so the next update() renders it again rather than
		// leaving a texture that has never been written to on the screen.
		contentChanged = false;
		haveSignature  = false;
		return true;
	}

	destroy();

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = width;
	bmi.bmiHeader.biHeight      = -height; // top-down
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	dc = CreateCompatibleDC(NULL);
	if (dc == NULL)
		return false;

	dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &dibBits, NULL, 0);
	if (dib == NULL || dibBits == NULL)
	{
		destroy();
		return false;
	}

	oldBitmap     = (HBITMAP)SelectObject(dc, dib);
	overlayWidth  = width;
	overlayHeight = height;
	overlayPitch  = width * 4;
	return true;
}

void TextOverlay::drawSlot(int slot)
{
	const int lineHeight = screenTextLineHeight();

	RECT r;
	r.left   = 10 + theApp.textMarginLeft;
	r.top    = overlayHeight - theApp.textMarginBottom - lineHeight * (slot + 1);
	r.right  = overlayWidth - 10 + theApp.textMarginLeft;
	r.bottom = r.top + lineHeight;

	const int w = r.right - r.left;
	const int h = r.bottom - r.top;
	if (w <= 0 || h <= 0)
		return;

	// GDI rasterises the glyphs into a mask of their own, one pixel larger on
	// each side than the text row so the outline is not clipped.  Rendering
	// straight into the overlay would blend the glyph against the transparent
	// overlay colour and lose the antialiased edge.
	const int mw = w + 2;
	const int mh = h + 2;

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = mw;
	bmi.bmiHeader.biHeight      = -mh; // top-down
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void *bits = NULL;
	HBITMAP bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
	if (bitmap == NULL || bits == NULL)
	{
		if (bitmap != NULL)
			DeleteObject(bitmap);
		return;
	}

	HDC maskDC = CreateCompatibleDC(dc);
	if (maskDC == NULL)
	{
		DeleteObject(bitmap);
		return;
	}

	HGDIOBJ oldBitmap = SelectObject(maskDC, bitmap);
	HGDIOBJ oldFont   = SelectObject(maskDC, GetCurrentObject(dc, OBJ_FONT));

	PatBlt(maskDC, 0, 0, mw, mh, BLACKNESS);
	SetBkMode(maskDC, OPAQUE);
	SetBkColor(maskDC, RGB(0, 0, 0));
	SetTextColor(maskDC, RGB(255, 255, 255));
	SetTextAlign(maskDC, TA_LEFT | TA_TOP);

	RECT maskRect;
	maskRect.left   = 1;
	maskRect.top    = 1;
	maskRect.right  = w + 1;
	maskRect.bottom = h + 1;
	DrawTextW(maskDC, theApp.screenMessageBuffer[slot], -1, &maskRect,
	          DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

	SelectObject(maskDC, oldFont);
	SelectObject(maskDC, oldBitmap);
	DeleteDC(maskDC);

	// Every channel of a white ClearType rendering carries a share of the
	// subpixel coverage; their average is the coverage of that pixel.
	std::vector<unsigned char> coverage((size_t)mw * mh);
	const unsigned char *src = (const unsigned char *)bits;
	for (int y = 0; y < mh; y++)
	{
		const unsigned char *row = src + (size_t)y * mw * 4;
		for (int x = 0; x < mw; x++, row += 4)
			coverage[(size_t)y * mw + x] = (unsigned char)((row[0] + row[1] + row[2]) / 3);
	}
	DeleteObject(bitmap);

	const COLORREF color  = overlayTextColor(textColor);
	const COLORREF border = textColor != 7 ? RGB(0, 0, 0) : RGB(255, 255, 255);
	const int tr = GetRValue(color),  tg = GetGValue(color),  tb = GetBValue(color);
	const int br = GetRValue(border), bg = GetGValue(border), bb = GetBValue(border);

	const int originX = r.left - 1;
	const int originY = r.top - 1;

	for (int y = 0; y < mh; y++)
	{
		const int py = originY + y;
		if (py < 0 || py >= overlayHeight)
			continue;

		unsigned char *dstRow = (unsigned char *)dibBits + (size_t)py * overlayPitch;
		for (int x = 0; x < mw; x++)
		{
			const int px = originX + x;
			if (px < 0 || px >= overlayWidth)
				continue;

			const int glyph = coverage[(size_t)y * mw + x];

			int outline = 0;
			if (outlinedText)
			{
				for (int dy = -1; dy <= 1; dy++)
				{
					const int ny = y + dy;
					if (ny < 0 || ny >= mh)
						continue;
					for (int dx = -1; dx <= 1; dx++)
					{
						if (dx == 0 && dy == 0)
							continue;
						const int nx = x + dx;
						if (nx < 0 || nx >= mw)
							continue;
						const int value = coverage[(size_t)ny * mw + nx];
						if (value > outline)
							outline = value;
					}
				}
			}

			if (glyph == 0 && outline == 0)
				continue;

			// The outline sits behind the glyph: a partially covered glyph
			// pixel shows the outline through the rest of the pixel.
			const int outlineAlpha = outline * (255 - glyph) / 255;
			const int alpha = glyph + outlineAlpha;
			if (alpha <= 0)
				continue;

			unsigned char *p = dstRow + px * 4;
			p[0] = (unsigned char)((tb * glyph + bb * outlineAlpha) / alpha);
			p[1] = (unsigned char)((tg * glyph + bg * outlineAlpha) / alpha);
			p[2] = (unsigned char)((tr * glyph + br * outlineAlpha) / alpha);
			p[3] = (unsigned char)alpha;
		}
	}
}

// Folds one visible slot into the signature the overlay is redrawn on.  The
// slot number goes in because two slots with the same text are drawn at
// different rows, and a separator keeps "ab" + "c" apart from "a" + "bc".
static unsigned int hashSlot(unsigned int hash, int slot, const CString &text)
{
	hash = (hash ^ (unsigned int)slot) * 16777619u;

	for (int i = 0; i < text.GetLength(); i++)
		hash = (hash ^ (unsigned int)(unsigned char)text[i]) * 16777619u;

	return (hash ^ 0x7fu) * 16777619u;
}

int TextOverlay::usedTop() const
{
	// One row per slot, counted up from the bottom, and the outline reaches a
	// few pixels above its row at larger scales.  The bottom margin moves the
	// whole block, so it moves the first painted row too.
	int top = overlayHeight - theApp.textMarginBottom
		- screenTextLineHeight() * SCREEN_MESSAGE_SLOTS - 1;

	return top > 0 ? top : 0;
}

int TextOverlay::usedHeight() const
{
	return overlayHeight - usedTop();
}

bool TextOverlay::update()
{
	if (dc == NULL || dibBits == NULL)
		return false;

	// The status bar owns the messages while it is showing.  Dropping the
	// signature makes the first update after it is switched off rebuild the
	// DIB instead of reusing whatever it held before.
	if (theApp.statusBarActive())
	{
		anyVisible     = false;
		contentChanged = false;
		haveSignature  = false;
		return false;
	}

	// Which slots are still visible, and what they say, is all the overlay is
	// made of, so a signature of that says whether the DIB already holds the
	// right picture.  While it does, nothing is rendered: the DIB is the size
	// of the window and update() walks it twice, which is a few million pixels
	// a frame for a picture that does not change between messages.
	bool visible[SCREEN_MESSAGE_SLOTS];
	unsigned int signature = 2166136261u; // FNV-1a
	int  visibleCount = 0;

	for (int slot = 0; slot < SCREEN_MESSAGE_SLOTS; slot++)
	{
		visible[slot] = false;

		if (!theApp.screenMessage[slot])
			continue;

		if ((theApp.screenMessageDuration[slot] < 0 ||
		     (int)(GetTickCount() - theApp.screenMessageTime[slot]) < theApp.screenMessageDuration[slot]) &&
		    (!theApp.disableStatusMessage || slot == 1 || slot == 2))
		{
			visible[slot] = true;
			visibleCount++;
			signature = hashSlot(signature, slot, theApp.screenMessageBuffer[slot]);
		}
		else
		{
			theApp.screenMessage[slot] = false;
		}
	}

	// Both colours are picked when the text is drawn, so they are part of what
	// the signature has to cover, and so is the font the options selected.
	signature = (signature ^ (unsigned int)textColor) * 16777619u;
	signature = (signature ^ (unsigned int)outlinedText) * 16777619u;
	signature = (signature ^ (unsigned int)theApp.screenTextSize) * 16777619u;
	signature = (signature ^ (unsigned int)theApp.textMarginLeft) * 16777619u;
	signature = (signature ^ (unsigned int)theApp.textMarginBottom) * 16777619u;
	for (int i = 0; i < theApp.screenTextFont.GetLength(); i++)
		signature = (signature ^ (unsigned int)(unsigned short)theApp.screenTextFont[i]) * 16777619u;

	anyVisible     = visibleCount > 0;
	contentChanged = false;

	if (!anyVisible)
	{
		// Nothing to show.  The caller stops drawing the overlay, so the DIB
		// may keep what it has until the next message fills it again.
		lastSignature = signature;
		haveSignature = true;
		return false;
	}

	if (haveSignature && signature == lastSignature)
		return true;

	// Clear to transparent; drawSlot() writes the text pixels and their alpha.
	for (int y = 0; y < overlayHeight; y++)
		memset((unsigned char *)dibBits + (size_t)y * overlayPitch, 0, (size_t)overlayPitch);

	HFONT font = createScreenTextFont();
	HGDIOBJ oldFont = NULL;
	if (font != NULL)
		oldFont = SelectObject(dc, font);

	for (int slot = 0; slot < SCREEN_MESSAGE_SLOTS; slot++)
	{
		if (visible[slot])
			drawSlot(slot);
	}

	if (font != NULL)
	{
		SelectObject(dc, oldFont);
		DeleteObject(font);
	}

	lastSignature  = signature;
	haveSignature  = true;
	contentChanged = true;
	return true;
}
