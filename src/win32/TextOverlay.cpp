#include "stdafx.h"

#include "TextOverlay.h"
#include "VBA.h"

#include "../common/Text.h"

// The DIB is filled with this colour and every pixel that still carries it
// after rendering becomes fully transparent.
#define OVERLAY_KEY_R 1
#define OVERLAY_KEY_G 2
#define OVERLAY_KEY_B 3

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
	RECT r;
	r.left   = 10;
	r.top    = overlayHeight - 20 * (slot + 1);
	r.right  = overlayWidth - 10;
	r.bottom = r.top + 20;

	SetBkMode(dc, TRANSPARENT);
	SetTextAlign(dc, TA_LEFT | TA_TOP);

	if (outlinedText)
	{
		const static int xd[8] = { -1, 0, 1, 1, 1, 0, -1, -1 };
		const static int yd[8] = { -1, -1, -1, 0, 1, 1, 1, 0 };

		SetTextColor(dc, textColor != 7 ? RGB(0, 0, 0) : RGB(255, 255, 255));
		for (int i = 0; i < 8; i++)
		{
			RECT r2 = r;
			r2.left   += xd[i];
			r2.right  += xd[i];
			r2.top    += yd[i];
			r2.bottom += yd[i];
			DrawText(dc, theApp.screenMessageBuffer[slot], -1, &r2,
			         DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		}
	}

	SetTextColor(dc, overlayTextColor(textColor));
	DrawText(dc, theApp.screenMessageBuffer[slot], -1, &r,
	         DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
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
	// One 20 pixel row per slot, counted up from the bottom, and the outline
	// reaches one pixel above its row.
	int top = overlayHeight - 20 * SCREEN_MESSAGE_SLOTS - 1;

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
	// the signature has to cover.
	signature = (signature ^ (unsigned int)textColor) * 16777619u;
	signature = (signature ^ (unsigned int)outlinedText) * 16777619u;

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

	// Fill with the transparent key colour.
	unsigned char *row = (unsigned char *)dibBits;
	for (int y = 0; y < overlayHeight; y++)
	{
		unsigned char *p = row;
		for (int x = 0; x < overlayWidth; x++, p += 4)
		{
			p[0] = OVERLAY_KEY_B;
			p[1] = OVERLAY_KEY_G;
			p[2] = OVERLAY_KEY_R;
			p[3] = 0;
		}
		row += overlayPitch;
	}

	HFONT font = CreateFont(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
	                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	                        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
	                        _T("Arial"));
	HFONT oldFont = (HFONT)SelectObject(dc, font);

	for (int slot = 0; slot < SCREEN_MESSAGE_SLOTS; slot++)
	{
		if (visible[slot])
			drawSlot(slot);
	}

	SelectObject(dc, oldFont);
	DeleteObject(font);

	// Turn the key colour into transparency.
	row = (unsigned char *)dibBits;
	for (int y = 0; y < overlayHeight; y++)
	{
		unsigned char *p = row;
		for (int x = 0; x < overlayWidth; x++, p += 4)
		{
			if (p[0] == OVERLAY_KEY_B && p[1] == OVERLAY_KEY_G && p[2] == OVERLAY_KEY_R)
			{
				p[0] = p[1] = p[2] = 0;
				p[3] = 0;
			}
			else
			{
				p[3] = 255;
			}
		}
		row += overlayPitch;
	}

	lastSignature  = signature;
	haveSignature  = true;
	contentChanged = true;
	return true;
}
