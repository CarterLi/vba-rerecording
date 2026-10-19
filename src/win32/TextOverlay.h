// TextOverlay.h: GDI based OSD text renderer shared by the D3D9 and D3D11 displays.
//
// The overlay is rendered once per frame into a 32bpp top-down DIB and then
// uploaded as a BGRA texture by the backend.  Pixels that keep the transparent
// key colour end up with alpha = 0, everything else becomes opaque, which lets
// the backends draw the overlay as a normal alpha blended quad.

#ifndef VBA_WIN32_TEXTOVERLAY_H
#define VBA_WIN32_TEXTOVERLAY_H

#if _MSC_VER > 1000
#pragma once
#endif

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
};

#endif // VBA_WIN32_TEXTOVERLAY_H
