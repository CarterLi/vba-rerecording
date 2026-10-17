#include "stdafx.h"

#pragma comment( lib, "d3d9" )

#include <d3d9.h>

#include "resource.h"
#include "MainWnd.h"
#include "Reg.h"
#include "VBA.h"
#include "TextOverlay.h"

//#include "../common/System.h"
#include "../common/SystemGlobals.h"
#include "../common/Text.h"
#include "../version.h"

#ifdef MMX
extern "C" bool cpu_mmx;

extern bool detectMMX();
#endif

extern int Init_2xSaI(u32);
extern void directXMessage(const char *);
extern void winlog(const char *, ...);

typedef struct _D3DTLVERTEX
{
	float    sx; /* Screen coordinates */
	float    sy;
	float    sz;
	float    rhw; /* Reciprocal of homogeneous w */
	D3DCOLOR color; /* Vertex color */
	float    tu; /* Texture coordinates */
	float    tv;
	_D3DTLVERTEX() { }
	_D3DTLVERTEX(float _x, float _y, float _z, float _rhw,
	             D3DCOLOR _color,
	             float _tu, float _tv)
	{
		sx    = _x; sy = _y; sz = _z; rhw = _rhw;
		color = _color;
		tu    = _tu; tv = _tv;
	}
} D3DTLVERTEX, *LPD3DTLVERTEX;

#define D3DFVF_TLVERTEX D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1

typedef HRESULT (WINAPI *PFN_Direct3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex * *ppD3D);
typedef IDirect3D9 *(WINAPI *PFN_Direct3DCreate9)(UINT SDKVersion);

class Direct3DDisplay : public IDisplay
{
public:
	Direct3DDisplay();
	virtual ~Direct3DDisplay();

	virtual bool initialize();
	virtual void cleanup();
	virtual void render();
	virtual void checkFullScreen();
	virtual void renderMenu();
	virtual void clear();
	virtual bool changeRenderSize(int w, int h);
	virtual void resize(int w, int h);
	virtual DISPLAY_TYPE getType() { return DIRECT_3D; };
	virtual void setOption(const char *, int);
	virtual int selectFullScreenMode(GUID * *);

private:
	HINSTANCE             d3dDLL;
	LPDIRECT3D9           pD3D;
	LPDIRECT3D9EX         pD3DEx;
	LPDIRECT3DDEVICE9     pDevice;
	LPDIRECT3DDEVICE9EX   pDeviceEx;
	LPDIRECT3DTEXTURE9    pTexture;
	LPDIRECT3DTEXTURE9    pOverlayTexture;
	D3DSURFACE_DESC       dsdBackBuffer;
	D3DPRESENT_PARAMETERS dpp;
	D3DFORMAT             screenFormat;
	int  width;
	int  height;
	bool filterDisabled;
	bool       failed;
	unsigned int textureSize;
	D3DTLVERTEX verts[4];
	TextOverlay overlay;

	void restoreDeviceObjects();
	void invalidateDeviceObjects();
	void releaseTextures();
	bool initializeOffscreen(int w, int h);
	void updateFiltering(int);
	void calculateVertices();
	void present();
	void drawTextOverlay();
};

Direct3DDisplay::Direct3DDisplay()
{
	d3dDLL         = NULL;
	pD3D           = NULL;
	pD3DEx         = NULL;
	pDevice        = NULL;
	pDeviceEx      = NULL;
	pTexture       = NULL;
	pOverlayTexture = NULL;
	screenFormat   = D3DFMT_R5G6B5;
	width          = 0;
	height         = 0;
	filterDisabled = false;
	failed         = false;
	textureSize    = 0;
}

Direct3DDisplay::~Direct3DDisplay()
{
	cleanup();
}

void Direct3DDisplay::cleanup()
{
	overlay.destroy();

	if (pOverlayTexture)
	{
		pOverlayTexture->Release();
		pOverlayTexture = NULL;
	}

	if (pTexture)
	{
		pTexture->Release();
		pTexture = NULL;
	}

	// pDevice / pDeviceEx and pD3D / pD3DEx may point at the same object, so
	// only one Release() per object is issued here.
	if (pDeviceEx)
	{
		pDeviceEx->Release();
		pDeviceEx = NULL;
		pDevice   = NULL;
	}
	else if (pDevice)
	{
		pDevice->Release();
		pDevice = NULL;
	}

	if (pD3DEx)
	{
		pD3DEx->Release();
		pD3DEx = NULL;
		pD3D   = NULL;
	}
	else if (pD3D)
	{
		pD3D->Release();
		pD3D = NULL;
	}

	if (d3dDLL != NULL)
	{
		FreeLibrary(d3dDLL);
		d3dDLL = NULL;
	}
}

bool Direct3DDisplay::initialize()
{
	CWnd *pWnd = theApp.m_pMainWnd;

	d3dDLL = LoadLibrary("D3D9.DLL");
	if (d3dDLL == NULL)
	{
		directXMessage("D3D9.DLL");
		return FALSE;
	}

	// Prefer Direct3D 9Ex (Vista and later); fall back to plain 9.0c.
	PFN_Direct3DCreate9Ex create9Ex =
	    (PFN_Direct3DCreate9Ex)GetProcAddress(d3dDLL, "Direct3DCreate9Ex");
	if (create9Ex != NULL)
		create9Ex(D3D_SDK_VERSION, &pD3DEx);

	if (pD3DEx != NULL)
	{
		pD3D = pD3DEx;
		winlog("Direct3D 9Ex device selected\n");
	}
	else
	{
		PFN_Direct3DCreate9 create9 =
		    (PFN_Direct3DCreate9)GetProcAddress(d3dDLL, "Direct3DCreate9");
		if (create9 == NULL)
		{
			winlog("Error creating Direct3D object\n");
			return FALSE;
		}
		pD3D = create9(D3D_SDK_VERSION);
		if (pD3D == NULL)
		{
			winlog("Error creating Direct3D object\n");
			return FALSE;
		}
	}

	theApp.mode320Available = false;
	theApp.mode640Available = false;
	theApp.mode800Available = false;

	D3DDISPLAYMODE mode;
	pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);

	switch (mode.Format)
	{
	case D3DFMT_R8G8B8:
		systemColorDepth = 24;
		systemRedShift   = 19;
		systemGreenShift = 11;
		systemBlueShift  = 3;
		break;
	case D3DFMT_X8R8G8B8:
		systemColorDepth = 32;
		systemRedShift   = 19;
		systemGreenShift = 11;
		systemBlueShift  = 3;
		Init_2xSaI(32);
		break;
	case D3DFMT_R5G6B5:
		systemColorDepth = 16;
		systemRedShift   = 11;
		systemGreenShift = 6;
		systemBlueShift  = 0;
		Init_2xSaI(565);
		break;
	case D3DFMT_X1R5G5B5:
		systemColorDepth = 16;
		systemRedShift   = 10;
		systemGreenShift = 5;
		systemBlueShift  = 0;
		Init_2xSaI(555);
		break;
	default:
		systemMessage(0, "Unsupport D3D format %d", mode.Format);
		return false;
	}
	theApp.fsColorDepth = systemColorDepth;

#ifdef MMX
	if (!theApp.disableMMX)
		cpu_mmx = theApp.detectMMX();
	else
		cpu_mmx = 0;
#endif

	screenFormat = mode.Format;

	// check for available fullscreen modes
	ZeroMemory(&dpp, sizeof(dpp));
	dpp.Windowed         = TRUE;
	dpp.BackBufferFormat = mode.Format;
	dpp.BackBufferCount  = 1;
	dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
	dpp.BackBufferWidth  = theApp.surfaceSizeX;
	dpp.BackBufferHeight = theApp.surfaceSizeY;

	HRESULT hret;
	if (pD3DEx != NULL)
	{
		hret = pD3DEx->CreateDeviceEx(D3DADAPTER_DEFAULT,
		                              D3DDEVTYPE_HAL,
		                              pWnd->GetSafeHwnd(),
		                              D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
		                              &dpp,
		                              NULL,
		                              &pDeviceEx);
		if (SUCCEEDED(hret))
			pDevice = pDeviceEx;
	}
	else
	{
		hret = pD3D->CreateDevice(D3DADAPTER_DEFAULT,
		                          D3DDEVTYPE_HAL,
		                          pWnd->GetSafeHwnd(),
		                          D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
		                          &dpp,
		                          &pDevice);
	}
	if (!SUCCEEDED(hret) || pDevice == NULL)
	{
		winlog("Error creating Direct3DDevice %08x\n", hret);
		return false;
	}

	restoreDeviceObjects();

	systemUpdateColorMaps();

	theApp.updateFilter();
	theApp.updateIFB();

	if (failed)
		return false;

	pWnd->DragAcceptFiles(TRUE);

	return TRUE;
}

void Direct3DDisplay::releaseTextures()
{
	if (pOverlayTexture)
	{
		pOverlayTexture->Release();
		pOverlayTexture = NULL;
	}
	if (pTexture)
	{
		pTexture->Release();
		pTexture = NULL;
	}
}

bool Direct3DDisplay::initializeOffscreen(int w, int h)
{
	int size = 256;
	if (w > 512 || h > 512)
		size = 1024;
	else if (w > 256 || h > 256)
		size = 512;
	textureSize = size;

	D3DCAPS9 caps;
	ZeroMemory(&caps, sizeof(caps));
	pDevice->GetDeviceCaps(&caps);

	int maxSize = (int)caps.MaxTextureWidth;
	if ((int)caps.MaxTextureHeight < maxSize)
		maxSize = (int)caps.MaxTextureHeight;
	if (maxSize > 0 && size > maxSize)
		size = maxSize;
	textureSize = size;

	if (size < w || size < h)
	{
		if (theApp.filterFunction)
		{
			filterDisabled        = true;
			theApp.filterFunction = NULL;
			systemMessage(0, "3D card cannot support needed texture size for filter function. Disabling it");
		}
		return false;
	}

	// Direct3D 9Ex does not support the managed pool, so a dynamic default
	// pool texture is used and fully rewritten every frame.
	if (SUCCEEDED(pDevice->CreateTexture(size,
	                                     size,
	                                     1,
	                                     D3DUSAGE_DYNAMIC,
	                                     screenFormat,
	                                     D3DPOOL_DEFAULT,
	                                     &pTexture,
	                                     NULL)))
	{
		width  = w;
		height = h;
		return true;
	}
	return false;
}

void Direct3DDisplay::updateFiltering(int filter)
{
	switch (filter)
	{
	default:
	case 0:
		// point filtering
		pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		break;
	case 1:
		// bilinear
		pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		break;
	}
}

void Direct3DDisplay::restoreDeviceObjects()
{
	// Store render target surface desc
	LPDIRECT3DSURFACE9 pBackBuffer;
	HRESULT hr;
	if (SUCCEEDED(hr = pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
	{
		pBackBuffer->GetDesc(&dsdBackBuffer);
		pBackBuffer->Release();
	}
	else
		systemMessage(0, "Failed GetBackBuffer %08x", hr);

	// Set up the texture
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	int filter = regQueryDwordValue("d3dFilter", 0);
	if (filter < 0 || filter > 3)
		filter = 0;
	updateFiltering(filter);

	// Set miscellaneous render states
	pDevice->SetRenderState(D3DRS_DITHERENABLE,   TRUE);
	pDevice->SetRenderState(D3DRS_ZENABLE,        FALSE);

	// turn off lighting
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	// Text overlay texture, sized 1:1 with the back buffer.
	if (pOverlayTexture == NULL && dpp.BackBufferWidth > 0 && dpp.BackBufferHeight > 0)
	{
		pDevice->CreateTexture(dpp.BackBufferWidth,
		                       dpp.BackBufferHeight,
		                       1,
		                       D3DUSAGE_DYNAMIC,
		                       D3DFMT_A8R8G8B8,
		                       D3DPOOL_DEFAULT,
		                       &pOverlayTexture,
		                       NULL);
		overlay.create(dpp.BackBufferWidth, dpp.BackBufferHeight);
	}
}

void Direct3DDisplay::clear()
{}

void Direct3DDisplay::renderMenu()
{
	checkFullScreen();
	if (theApp.m_pMainWnd)
		theApp.m_pMainWnd->DrawMenuBar();
}

void Direct3DDisplay::checkFullScreen()
{
	//  if(tripleBuffering)
	//    pDirect3D->FlipToGDISurface();
}

void Direct3DDisplay::calculateVertices()
{
	// color
	D3DCOLOR col = 0xFFFFFFFF;

	// calculate rhw
	FLOAT z	  = 1.0f;
	FLOAT rhw = 1.0f / (z * 990.0f + 10.0f);

	// -0.5f is necessary in order to match texture alignment to display pixels
	FLOAT left     = -0.5f;
	FLOAT right    = dpp.BackBufferWidth - 0.5f;
	FLOAT top      = -0.5f;
	FLOAT bottom   = dpp.BackBufferHeight - 0.5f;

	FLOAT textureX = (FLOAT)theApp.rect.right / (FLOAT)textureSize;
	FLOAT textureY = (FLOAT)theApp.rect.bottom / (FLOAT)textureSize;

//#define D3D_DRAW_SINGLE_RECTANGLE
#ifdef D3D_DRAW_SINGLE_RECTANGLE
	// set up a rectangle
	verts[0] = D3DTLVERTEX(left,  top,    z, rhw, col, 0.0f,     0.0f);
	verts[1] = D3DTLVERTEX(right, top,    z, rhw, col, textureX, 0.0f);
	verts[2] = D3DTLVERTEX(right, bottom, z, rhw, col, textureX, textureY);
	verts[3] = D3DTLVERTEX(left,  bottom, z, rhw, col, 0.0f,     textureY);
#else
	// set up triangles
	verts[0] = D3DTLVERTEX(left,  bottom, z, rhw, col, 0.0f,     textureY);
	verts[1] = D3DTLVERTEX(left,  top,    z, rhw, col, 0.0f,     0.0f);
	verts[2] = D3DTLVERTEX(right, bottom, z, rhw, col, textureX, textureY);
	verts[3] = D3DTLVERTEX(right, top,    z, rhw, col, textureX, 0.0f);
#endif
}

void Direct3DDisplay::drawTextOverlay()
{
	if (textMethod != 2)
		return;
	if (pOverlayTexture == NULL || overlay.empty())
		return;

	if (!overlay.update())
		return;

	D3DLOCKED_RECT locked;
	if (FAILED(pOverlayTexture->LockRect(0, &locked, NULL, D3DLOCK_DISCARD)))
		return;

	const unsigned char *src = overlay.pixels();
	int srcPitch = overlay.pitch();
	int copyW    = overlay.width() * 4;
	if (copyW > (int)locked.Pitch)
		copyW = (int)locked.Pitch;
	for (int y = 0; y < overlay.height(); y++)
		memcpy((unsigned char *)locked.pBits + y * locked.Pitch, src + y * srcPitch, copyW);

	pOverlayTexture->UnlockRect(0);

	pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	pDevice->SetTexture(0, pOverlayTexture);

	D3DTLVERTEX quad[4];
	D3DCOLOR col = 0xFFFFFFFF;
	FLOAT z   = 1.0f;
	FLOAT rhw = 1.0f / (z * 990.0f + 10.0f);
	quad[0] = D3DTLVERTEX(-0.5f,                              dpp.BackBufferHeight - 0.5f, z, rhw, col, 0.0f, 1.0f);
	quad[1] = D3DTLVERTEX(-0.5f,                              -0.5f,                       z, rhw, col, 0.0f, 0.0f);
	quad[2] = D3DTLVERTEX(dpp.BackBufferWidth - 0.5f,         dpp.BackBufferHeight - 0.5f, z, rhw, col, 1.0f, 1.0f);
	quad[3] = D3DTLVERTEX(dpp.BackBufferWidth - 0.5f,         -0.5f,                       z, rhw, col, 1.0f, 0.0f);

	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3DTLVERTEX));

	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	int filter = regQueryDwordValue("d3dFilter", 0);
	if (filter < 0 || filter > 3)
		filter = 0;
	updateFiltering(filter);
}

void Direct3DDisplay::render()
{
	if (!pDevice)
		return;

	// Test the cooperative level to see if it's okay to render
	if (FAILED(pDevice->TestCooperativeLevel()))
	{
		return;
	}
	pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, 0x000000ff, 1.0f, 0L);

	u8 *data = pix;
	int dataPitch = theApp.filterWidth * (systemColorDepth / 8) + (systemColorDepth == 24 ? 0 : 4);

	if (textMethod != 0) // do not draw Lua HUD to a video dump
	{
		systemClonePixBuffer(osd);
		data = osd;
		systemRenderLua((u8 *)data, dataPitch);
	}

	if (SUCCEEDED(pDevice->BeginScene()))
	{
		D3DLOCKED_RECT locked;
		if (pTexture && SUCCEEDED(pTexture->LockRect(0, &locked, NULL, D3DLOCK_DISCARD)))
		{
			if (theApp.filterFunction)
			{
					theApp.filterFunction(data + dataPitch,
					                      dataPitch,
					                      (u8 *)theApp.delta,
					                      (u8 *)locked.pBits,
					                      locked.Pitch,
					                      theApp.filterWidth,
					                      theApp.filterHeight);
			}
			else
			{
				int copyX, copyY;
				systemGetLCDResolution(copyX, copyY);

				// MMX doesn't seem to be faster to copy the data
				__asm {
					mov eax, copyX;
					mov ebx, copyY;

					mov esi, data;
					mov edi, locked.pBits;
					mov edx, locked.Pitch;
					cmp systemColorDepth, 16;
					jnz gbaOtherColor;
					sub edx, eax;
					sub edx, eax;
					lea esi, [esi+2*eax+4];
					shr eax, 1;
gbaLoop16bit:
					mov ecx, eax;
					repz movsd;
					inc  esi;
					inc  esi;
					inc  esi;
					inc  esi;
					add  edi, edx;
					dec  ebx;
					jnz  gbaLoop16bit;
					jmp  gbaLoopEnd;
gbaOtherColor:
					cmp systemColorDepth, 32;
					jnz gbaOtherColor2;

					sub edx, eax;
					sub edx, eax;
					sub edx, eax;
					sub edx, eax;
					lea esi, [esi+4*eax+4];
gbaLoop32bit:
					mov ecx, eax;
					repz movsd;
					add  esi, 4;
					add  edi, edx;
					dec  ebx;
					jnz  gbaLoop32bit;
					jmp  gbaLoopEnd;
gbaOtherColor2:
					lea eax, [eax+2*eax];
					sub edx, eax;
gbaLoop24bit:
					mov ecx, eax;
					shr  ecx, 2;
					repz movsd;
					add  edi, edx;
					dec  ebx;
					jnz  gbaLoop24bit;
gbaLoopEnd:
				}
			}

			if (isFullscreenOption(theApp.videoOption) && theApp.showSpeed)
			{
				char buffer[30];
				if (theApp.showSpeed == 1)
					sprintf(buffer, "%3d%%", systemSpeed);
				else
					sprintf(buffer, "%3d%%(%d, %d fps)", systemSpeed,
					        systemFrameSkip,
					        theApp.showRenderedFrames);
				if (theApp.showSpeedTransparent)
					drawTextTransp((u8 *)locked.pBits,
					               locked.Pitch,
					               theApp.rect.left+10,
					               theApp.rect.bottom-10,
					               buffer);
				else
					drawText((u8 *)locked.pBits,
					         locked.Pitch,
					         theApp.rect.left+10,
					         theApp.rect.bottom-10,
					         buffer);
			}

			if (textMethod == 1)
			{
				DrawTextMessages((u8 *)locked.pBits, locked.Pitch, theApp.rect.left, theApp.rect.bottom);
			}

			pTexture->UnlockRect(0);

			// set the texture
			pDevice->SetTexture(0, pTexture);

			// configure shader for vertex type
			pDevice->SetVertexShader(NULL);
			pDevice->SetFVF(D3DFVF_TLVERTEX);

#ifdef D3D_DRAW_SINGLE_RECTANGLE
			// draw the rectangle
				pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(D3DTLVERTEX));
			//#undef D3D_DRAW_RECT
#else
			// draw the triangles
			pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(D3DTLVERTEX));
#endif
		}

		drawTextOverlay();

		pDevice->EndScene();
	}

	present();
}

void Direct3DDisplay::present()
{
	if (pDeviceEx != NULL)
		pDeviceEx->PresentEx(NULL, NULL, NULL, NULL, 0);
	else
		pDevice->Present(NULL, NULL, NULL, NULL);
}

void Direct3DDisplay::invalidateDeviceObjects()
{
	overlay.destroy();
}

void Direct3DDisplay::resize(int w, int h)
{
	if (pDevice && w > 0 && h > 0)
	{
		dpp.BackBufferWidth  = w;
		dpp.BackBufferHeight = h;
		HRESULT hr;
		invalidateDeviceObjects();
		releaseTextures();
		if (pDeviceEx != NULL)
			hr = pDeviceEx->ResetEx(&dpp, NULL);
		else
			hr = pDevice->Reset(&dpp);
		if (SUCCEEDED(hr))
		{
			restoreDeviceObjects();
			if (width > 0 && height > 0)
				initializeOffscreen(width, height);
		}
		else
			systemMessage(0, "Failed device reset %08x", hr);
	}
	calculateVertices();
}

bool Direct3DDisplay::changeRenderSize(int w, int h)
{
	if (w != width || h != height)
	{
		if (pTexture)
		{
			pTexture->Release();
			pTexture = NULL;
		}
		if (!initializeOffscreen(w, h))
		{
			failed = true;
			return false;
		}
	}
	calculateVertices();

	return true;
}

void Direct3DDisplay::setOption(const char *option, int value)
{
	if (!strcmp(option, "d3dFilter"))
		updateFiltering(value);
}

int Direct3DDisplay::selectFullScreenMode(GUID * *)
{
	HWND wnd = GetDesktopWindow();
	RECT r;
	GetWindowRect(wnd, &r);
	int w  = (r.right - r.left) & 4095;
	int h  = (r.bottom - r.top) & 4095;
	HDC dc = GetDC(wnd);
	int c  = GetDeviceCaps(dc, BITSPIXEL);
	ReleaseDC(wnd, dc);

	return (c << 24) | (w << 12) | h;
}

IDisplay *newDirect3DDisplay()
{
	return new Direct3DDisplay();
}
