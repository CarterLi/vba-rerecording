#include "stdafx.h"
#include "OpenGLConfig.h"

#if VBA_ENABLE_OPENGL

#include <gl/GL.h>

// The headers the Windows SDK ships stop at OpenGL 1.1; these two constants
// come from later revisions.  The overlay is blitted one to one onto a picture
// that may not be a power of two, so it needs the edge clamp, and the DIB GDI
// fills is BGRA.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

#include "resource.h"
#include "MainWnd.h"
#include "Reg.h"
#include "VBA.h"
#include "TextOverlay.h"

#include "../gba/GBAGlobals.h"
#include "../gb/gbGlobals.h"
#include "../common/SystemGlobals.h"
#include "../common/Text.h"
#include "../version.h"

extern int systemSpeed;
extern int Init_2xSaI(u32);

class OpenGLDisplay : public IDisplay
{
private:
	HDC    hDC;
	CWnd * dcWnd;
	HGLRC  hglrc;
	GLuint texture;
	int    width;
	int    height;
	float  size;
	u8 *   filterData;
	bool   failed;

	// The on screen display: GDI rasterises the messages into this DIB and the
	// renderer composites it over the picture, the same TextOverlay the D3D
	// backends use.
	TextOverlay overlay;
	GLuint      overlayTexture;
	int         overlayWidth;
	int         overlayHeight;

	bool initializeTexture(int w, int h);
	void updateFiltering(int);

	bool createOverlayTexture(int w, int h);
	void destroyOverlayTexture();
	void drawOverlay();
public:
	OpenGLDisplay();
	virtual ~OpenGLDisplay();

	virtual bool initialize();
	virtual void cleanup();
	virtual void render();
	virtual void clear();
	virtual bool changeRenderSize(int w, int h);
	virtual void resize(int w, int h);
	virtual DISPLAY_TYPE getType() { return OPENGL; };
	virtual void setOption(const char *, int);

	// The legacy renderer samples a texture with the two filters the fixed
	// function pipeline has, and has no mip chain to blend between: trilinear
	// is the one it cannot do.
	virtual bool filterAvailable(int filter) { return filter == 0 || filter == 1; };
};

OpenGLDisplay::OpenGLDisplay()
{
	hDC        = NULL;
	dcWnd      = NULL;
	hglrc      = NULL;
	texture    = 0;
	width      = 0;
	height     = 0;
	size       = 0.0f;
	filterData = (u8 *)malloc(4*16*256*256); // sufficient for 4x filters @ 32bit color depth
	failed     = false;

	overlayTexture = 0;
	overlayWidth   = 0;
	overlayHeight  = 0;
}

OpenGLDisplay::~OpenGLDisplay()
{
	cleanup();
}

void OpenGLDisplay::cleanup()
{
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}
	destroyOverlayTexture();
	if (hglrc != NULL)
	{
		// The context is let go of before it is deleted: deleting one that is
		// still current on the thread fails, and the context would then stay
		// alive with the window's device context still tied to it.
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hglrc);
		hglrc = NULL;
	}
	if (hDC != NULL)
	{
		if (dcWnd != NULL)
			::ReleaseDC(dcWnd->GetSafeHwnd(), hDC);
		dcWnd = NULL;
		hDC = NULL;
	}
	if (filterData)
	{
		free(filterData);
		filterData = NULL;
	}
	width  = 0;
	height = 0;
	size   = 0.0f;
}

bool OpenGLDisplay::initialize()
{
	CWnd *pWnd = theApp.winRenderWnd();


	CDC *dc  = pWnd->GetDC();
	hDC      = dc->GetSafeHdc();
	dcWnd    = pWnd;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), //  size of this pfd
		1, // version number
		PFD_DRAW_TO_WINDOW | // support window
		PFD_SUPPORT_OPENGL | // support OpenGL
		PFD_DOUBLEBUFFER, // double buffered
		PFD_TYPE_RGBA, // RGBA type
		16, // 16-bit color depth
		0, 0, 0, 0, 0, 0,  // color bits ignored
		0, // no alpha buffer
		0, // shift bit ignored
		0, // no accumulation buffer
		0, 0, 0, 0, // accum bits ignored
		32, // 32-bit z-buffer
		0, // no stencil buffer
		0, // no auxiliary buffer
		PFD_MAIN_PLANE, // main layer
		0, // reserved
		0, 0, 0            // layer masks ignored
	};
	int iPixelFormat;

	if (!(iPixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		winlog(_T("Failed ChoosePixelFormat\n"));
		return false;
	}

	// obtain detailed information about
	// the device context's first pixel format
	if (!(DescribePixelFormat(hDC, iPixelFormat,
	                          sizeof(PIXELFORMATDESCRIPTOR), &pfd)))
	{
		winlog(_T("Failed DescribePixelFormat\n"));
		return false;
	}

	if (!SetPixelFormat(hDC, iPixelFormat, &pfd))
	{
		winlog(_T("Failed SetPixelFormat\n"));
		return false;
	}

	if (!(hglrc = wglCreateContext(hDC)))
	{
		winlog(_T("Failed wglCreateContext\n"));
		return false;
	}

	if (!wglMakeCurrent(hDC, hglrc))
	{
		winlog(_T("Failed wglMakeCurrent\n"));
		return false;
	}
	pWnd->ReleaseDC(dc);

	// setup 2D gl environment
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glViewport(0, 0, theApp.surfaceSizeX, theApp.surfaceSizeY);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, (GLdouble)(theApp.surfaceSizeX), (GLdouble)(theApp.surfaceSizeY),
	        0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	systemRedShift      = 3;
	systemGreenShift    = 11;
	systemBlueShift     = 19;
	systemColorDepth    = 32;
	theApp.fsColorDepth = 32;

	Init_2xSaI(32);
	if (theApp.ddrawDebug)
	{
		winlog(_T("R shift: %d\n"), systemRedShift);
		winlog(_T("G shift: %d\n"), systemGreenShift);
		winlog(_T("B shift: %d\n"), systemBlueShift);
	}

	systemUpdateColorMaps();

	theApp.updateFilter();
	theApp.updateIFB();

	if (failed)
		return false;

	pWnd->DragAcceptFiles(TRUE);

	return TRUE;
}

void OpenGLDisplay::clear()
{
	if (hglrc == NULL || theApp.winRenderWnd() == NULL)
		return;

	CDC *dc = theApp.winRenderWnd()->GetDC();
	if (dc == NULL)
		return;

	HDC hdc = dc->GetSafeHdc();
	wglMakeCurrent(hdc, hglrc);

	// The window keeps showing the last swapped buffer until the next swap,
	// so clearing alone does not remove the picture of a closed ROM.
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SwapBuffers(hdc);

	theApp.winRenderWnd()->ReleaseDC(dc);
}

void OpenGLDisplay::render()
{
	void (*filterFunction)(u8 *, u32, u8 *, u8 *, u32, int, int) = theApp.filterFunction;
	int  filterWidth = theApp.filterWidth, filterHeight = theApp.filterHeight;

	int rectWidth = theApp.rect.right - theApp.rect.left, dataHeight = theApp.rect.bottom - theApp.rect.top;
	int filterPitch = rectWidth * 4;

	u8 *data = pix;
	int dataPitch = filterWidth * 4 + 4;

	if (textMethod != 0) // do not draw Lua HUD to a video dump
	{
		systemClonePixBuffer(osd);
		data = osd;
		systemRenderLua((u8 *)data, dataPitch);
	}

	if (filterFunction)
	{
		filterFunction(data + dataPitch,
		               dataPitch,
		               (u8 *)theApp.delta,
		               (u8 *)filterData,
		               filterPitch,
		               filterWidth,
		               filterHeight);

		data = filterData;
		dataPitch = filterPitch;
	}

	if (theApp.showSpeed && isFullscreenOption(theApp.videoOption))
	{
		char buffer[30];
		if (theApp.showSpeed == 1)
			sprintf(buffer, "%3d%%", systemSpeed);
		else
			sprintf(buffer, "%3d%%(%d, %d fps)", systemSpeed,
			        systemFrameSkip,
			        theApp.showRenderedFrames);

		if (theApp.showSpeedTransparent)
			drawTextTransp(data,
				dataPitch,
				theApp.rect.left + 10,
				theApp.rect.bottom - 10,
				buffer);
		else
			drawText(data,
				dataPitch,
				theApp.rect.left + 10,
				theApp.rect.bottom - 10,
				buffer);
	}

	if (textMethod == 1)
	{
		DrawTextMessages((u8 *)data, dataPitch, theApp.rect.left, theApp.rect.bottom);
	}

	// Texturemap complete texture to surface so we have free scaling
	// and antialiasing
	if (filterFunction)
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, rectWidth);
	}
	else
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, theApp.sizeX + 1);
	}

	// The picture is drawn from whichever texture is current, so it is named
	// here rather than left to be whatever the last call happened to bind.
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexSubImage2D(GL_TEXTURE_2D, 0,
					theApp.rect.left, theApp.rect.top, rectWidth, dataHeight,
	                GL_RGBA, GL_UNSIGNED_BYTE, data);

	if (theApp.glType == 0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(theApp.rect.left / size, theApp.rect.top / size); glVertex3i(0, 0, 0);
		glTexCoord2f(theApp.rect.right / size, theApp.rect.top / size); glVertex3i(theApp.surfaceSizeX, 0, 0);
		glTexCoord2f(theApp.rect.left / size, theApp.rect.bottom / size); glVertex3i(0, theApp.surfaceSizeY, 0);
		glTexCoord2f(theApp.rect.right / size, theApp.rect.bottom / size); glVertex3i(theApp.surfaceSizeX, theApp.surfaceSizeY, 0);
		glEnd();
	}
	else
	{
		glBegin(GL_QUADS);
		glTexCoord2f(theApp.rect.left / size, theApp.rect.top / size); glVertex3i(0, 0, 0);
		glTexCoord2f(theApp.rect.right / size, theApp.rect.top / size); glVertex3i(theApp.surfaceSizeX, 0, 0);
		glTexCoord2f(theApp.rect.right / size, theApp.rect.bottom / size); glVertex3i(theApp.surfaceSizeX, theApp.surfaceSizeY, 0);
		glTexCoord2f(theApp.rect.left / size, theApp.rect.bottom / size); glVertex3i(0, theApp.surfaceSizeY, 0);
		glEnd();
	}

	// The messages are rasterised by GDI into a DIB and drawn as a texture,
	// which is the way every other renderer carries them: D3D9, D3D11 and
	// WebGPU all fill a TextOverlay and composite it over the picture.  GDI
	// straight onto this window's device context cannot work here, because the
	// picture is presented through a flip swap chain and what the desktop
	// composes is that swap chain - not the surface GDI would be writing to.
	// The glyphs are still GDI's; only the last step belongs to the renderer.
	if (textMethod == 2)
		drawOverlay();

	CDC *dc = theApp.winRenderWnd()->GetDC();
	if (dc == NULL)
		return;

	SwapBuffers(dc->GetSafeHdc());

	theApp.winRenderWnd()->ReleaseDC(dc);
}

// The overlay is a texture the size of the picture area, and it is written
// again only when a message appears, expires or says something else.
bool OpenGLDisplay::createOverlayTexture(int w, int h)
{
	destroyOverlayTexture();

	if (!overlay.create(w, h))
	{
		winlog(_T("OpenGL: the on screen display cannot be allocated\n"));
		return false;
	}

	glGenTextures(1, &overlayTexture);
	glBindTexture(GL_TEXTURE_2D, overlayTexture);

	// GL_TEXTURE_MIN_FILTER defaults to the trilinear mipmap filter, and a
	// texture that has no mip chain is incomplete, which draws as white.  The
	// overlay is blitted one to one, so point sampling keeps the glyphs as
	// sharp as GDI rasterised them.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA_EXT,
	             GL_UNSIGNED_BYTE, NULL);

	overlayWidth  = w;
	overlayHeight = h;

	return true;
}

void OpenGLDisplay::destroyOverlayTexture()
{
	if (overlayTexture != 0)
	{
		glDeleteTextures(1, &overlayTexture);
		overlayTexture = 0;
	}

	overlay.destroy();

	overlayWidth  = 0;
	overlayHeight = 0;
}

void OpenGLDisplay::drawOverlay()
{
	// The size comes from the same two numbers the picture vertices are placed
	// with, so the overlay can never end up a different shape than the surface
	// and be rebuilt on every frame.
	const int w = theApp.surfaceSizeX;
	const int h = theApp.surfaceSizeY;

	if (w <= 0 || h <= 0)
		return;

	if (overlayTexture == 0 || overlayWidth != w || overlayHeight != h)
	{
		if (!createOverlayTexture(w, h))
			return;
	}

	// False while the status bar owns the messages, and once the last one has
	// expired.
	if (!overlay.update())
		return;

	if (overlay.changed())
	{
		// A top-down DIB into a projection with y growing downwards: row 0 of
		// the texture is row 0 of the picture, with no flip on either side.
		glBindTexture(GL_TEXTURE_2D, overlayTexture);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, overlay.pitch() / 4);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
		                overlay.width(), overlay.height(),
		                GL_BGRA_EXT, GL_UNSIGNED_BYTE, overlay.pixels());
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, overlayTexture);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3i(0, 0, 0);
	glTexCoord2f(1.0f, 0.0f); glVertex3i(w, 0, 0);
	glTexCoord2f(1.0f, 1.0f); glVertex3i(w, h, 0);
	glTexCoord2f(0.0f, 1.0f); glVertex3i(0, h, 0);
	glEnd();

	glDisable(GL_BLEND);
}

void OpenGLDisplay::resize(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, (GLdouble)(w), (GLdouble)(h), 0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void OpenGLDisplay::updateFiltering(int value)
{
	switch (value)
	{
	case 0:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	}
}

bool OpenGLDisplay::initializeTexture(int w, int h)
{
	int mySize = 256;
	size = 256.0f;
	if (w > 511 || h > 511)
	{
		size   = 1024.0f;
		mySize = 1024;
	}
	else if (w > 255 || h > 255)
	{
		size   = 512.0f;
		mySize = 512;
	}
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int filter = regQueryDwordValue(_T("glFilter"), 0);
	if (filter < 0 || filter > 1)
		filter = 0;
	updateFiltering(filter);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mySize, mySize, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, NULL);
	width  = w;
	height = h;

	return true;
}

bool OpenGLDisplay::changeRenderSize(int w, int h)
{
	if (width != w || height != h)
	{
		if (texture != 0)
		{
			glDeleteTextures(1, &texture);
			texture = 0;
		}
		if (!initializeTexture(w, h))
		{
			failed = true;
			return false;
		}
	}
	return true;
}

void OpenGLDisplay::setOption(const char *option, int value)
{
	if (!strcmp(option, "glFilter"))
		updateFiltering(value);
}

IDisplay *newOpenGLDisplay()
{
	return new OpenGLDisplay();
}

#else // ndef VBA_ENABLE_OPENGL

#include "Display.h"

// The renderer is not built, so there is nothing to create.  The menu does
// not offer it either, see the switch in OpenGLConfig.h.
IDisplay *newOpenGLDisplay()
{
	return NULL;
}

#endif // !VBA_ENABLE_OPENGL

