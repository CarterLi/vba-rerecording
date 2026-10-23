// TextOverlay.h: GDI based OSD text renderer shared by the D3D9 and D3D11 displays.
//
// The overlay is rendered once per frame into a 32bpp top-down DIB and then
// uploaded as a BGRA texture by the backend.  Text is rasterised by GDI into a
// coverage mask and composited with straight alpha, so the antialiased edges
// of a TrueType face stay soft instead of turning opaque.

#ifndef VBA_WIN32_TEXTOVERLAY_H
#define VBA_WIN32_TEXTOVERLAY_H

#pragma once

// Font used by the On Screen display.  It is always a scalable fixed pitch
// face; the In Game / On Game modes have their own raster font list.
HFONT createScreenTextFont();
int   screenTextFontHeight();
int   screenTextLineHeight();

class TextOverlay
{
public:
	TextOverlay();
	~TextOverlay();

	// (Re)allocates the backing store.  Returns false when the DIB cannot be created.
	bool create(int width, int height);
	void destroy();

	// Redraws the pending screen messages.  Returns false when nothing is visible,
	// in which case the caller can skip uploading the overlay entirely.
	bool update();

	// True when the last update() changed the pixels, which is the case only
	// when a message appeared, expired or changed what it says.  The overlay is
	// the size of the window, so a backend only uploads it again when this is
	// set.
	bool changed() const { return contentChanged; }

	// The rows update() paints on, from usedTop() down to the bottom edge: the
	// message slots sit there, one 20 pixel row each, and the outline reaches a
	// pixel above its row.  Everything above them stays the transparent key
	// colour, so a backend only has to upload this band again.
	int usedTop() const;
	int usedHeight() const;

	bool  empty() const { return dib == NULL; }
	int   width() const { return overlayWidth; }
	int   height() const { return overlayHeight; }
	int   pitch() const { return overlayPitch; }
	const unsigned char *pixels() const { return (const unsigned char *)dibBits; }

private:
	void drawSlot(int slot);

	HDC     dc;
	HBITMAP dib;
	HBITMAP oldBitmap;
	void   *dibBits;
	int     overlayWidth;
	int     overlayHeight;
	int     overlayPitch;
	bool    anyVisible;
	bool    contentChanged;
	bool    haveSignature;
	unsigned int lastSignature;
};

#endif // VBA_WIN32_TEXTOVERLAY_H
