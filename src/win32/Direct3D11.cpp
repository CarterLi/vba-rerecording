// Direct3D11.cpp: Direct3D 11 display backend.
//
// The emulator frame is uploaded into a dynamic BGRA/BGR565 texture and drawn
// as a single textured quad.  OSD text is rendered by TextOverlay into a
// second texture and blended on top, so it stays 1:1 with the window.
//
// Optionally the frame is scaled up by NVIDIA Image Scaling (NIS) instead of
// the sampler: the SDK compute shader runs between the frame upload and the
// quad draw, see the NIS section below.

#include "stdafx.h"

#pragma comment( lib, "d3d11" )
#pragma comment( lib, "dxgi" )
#pragma comment( lib, "d3dcompiler" )

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include "resource.h"
#include "MainWnd.h"
#include "Reg.h"
#include "VBA.h"
#include "TextOverlay.h"

// NVIDIA Image Scaling SDK, see src/win32/nis for these files.
#include "nis/NIS_Config.h"
#include "nis/NIS_Shader.h"

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

struct D3D11VERTEX
{
	float x, y;
	float u, v;
};

static const char *g_quadShader =
    "struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };\n"
    "VSOut VSMain(float4 pos : POSITION, float2 uv : TEXCOORD0)\n"
    "{\n"
    "    VSOut o;\n"
    "    o.pos = pos;\n"
    "    o.uv = uv;\n"
    "    return o;\n"
    "}\n"
    "Texture2D frameTex : register(t0);\n"
    "SamplerState frameSampler : register(s0);\n"
    "float4 PSMain(VSOut i) : SV_TARGET\n"
    "{\n"
    "    return frameTex.Sample(frameSampler, i.uv);\n"
    "}\n";

// --- NVIDIA Image Scaling (NIS) ------------------------------------------
//
// The block layout is baked into the compute shader and has to match the
// dispatch; 32x24 with 128 threads per group is what the SDK's NISOptimizer
// picks for upscaling on generic hardware.
#define NIS_BLOCK_WIDTH        32
#define NIS_BLOCK_HEIGHT       24
#define NIS_THREAD_GROUP_SIZE  128

#define NIS_STR_INNER(x) #x
#define NIS_STR(x)       NIS_STR_INNER(x)

// A single NIS pass scales by at most 2x, so larger windows need several
// passes.  Four of them reach 16x, well past any window size.
#define NIS_MAX_PASSES 4

// Serves NIS_Scaler.h to D3DCompile from the embedded SDK sources, so the
// shaders stay inside the executable instead of sitting next to it.
class NisIncludeHandler : public ID3DInclude
{
public:
	STDMETHOD(Open)(D3D_INCLUDE_TYPE type, LPCSTR name, LPCVOID parent, LPCVOID *data, UINT *bytes)
	{
		if (name == NULL || _stricmp(name, "NIS_Scaler.h") != 0)
			return E_FAIL;

		*data  = g_nisScalerShader;
		*bytes = (UINT)strlen(g_nisScalerShader);
		return S_OK;
	}

	STDMETHOD(Close)(LPCVOID data)
	{
		return S_OK;
	}
};

class Direct3D11Display : public IDisplay
{
public:
	Direct3D11Display();
	virtual ~Direct3D11Display();

	virtual bool initialize();
	virtual void cleanup();
	virtual void render();
	virtual void checkFullScreen();
	virtual void renderMenu();
	virtual void clear();
	virtual bool changeRenderSize(int w, int h);
	virtual void resize(int w, int h);
	virtual DISPLAY_TYPE getType() { return DIRECT_3D11; };
	virtual void setOption(const char *, int);
	virtual int selectFullScreenMode(GUID * *);

private:
	bool createDevice();
	bool createShaders();
	bool createSamplers();
	bool createBackBufferViews();
	void releaseFrameTexture();
	void releaseAll();
	bool initializeOffscreen(int w, int h);
	void updateFiltering(int filter);
	void calculateVertices();
	void drawQuad(ID3D11ShaderResourceView *srv, D3D11VERTEX *quad, bool blend);

	// NVIDIA Image Scaling
	bool initNis();
	void releaseNis();
	void releaseNisTargets();
	bool updateNisChain(int srcWidth, int srcHeight, int dstWidth, int dstHeight);
	ID3D11ShaderResourceView *scaleFrameWithNis();

	HWND                  hwnd;
	ID3D11Device         *pDevice;
	ID3D11DeviceContext  *pContext;
	IDXGISwapChain       *pSwapChain;
	ID3D11RenderTargetView *pRTV;
	ID3D11Texture2D      *pFrameTexture;
	ID3D11ShaderResourceView *pFrameSRV;
	ID3D11Texture2D      *pOverlayTexture;
	ID3D11ShaderResourceView *pOverlaySRV;
	ID3D11VertexShader   *pVS;
	ID3D11PixelShader    *pPS;
	ID3D11InputLayout    *pLayout;
	ID3D11SamplerState   *pSamplerPoint;
	ID3D11SamplerState   *pSamplerLinear;
	ID3D11BlendState     *pBlendAlpha;
	ID3D11Buffer         *pVertexBuffer;

	// NVIDIA Image Scaling: one pass target per 2x step between the frame and
	// the back buffer.
	struct NisTarget
	{
		ID3D11Texture2D           *texture;
		ID3D11ShaderResourceView  *srv;
		ID3D11UnorderedAccessView *uav;
		int                        width;
		int                        height;
	};

	D3D_FEATURE_LEVEL          featureLevel;
	bool                       nisAvailable;
	ID3D11ComputeShader       *pNisCS;
	ID3D11Buffer              *pNisCB;
	ID3D11Texture2D           *pNisCoefScale;
	ID3D11ShaderResourceView  *pNisCoefScaleSRV;
	ID3D11Texture2D           *pNisCoefUsm;
	ID3D11ShaderResourceView  *pNisCoefUsmSRV;
	NisTarget                  nisTargets[NIS_MAX_PASSES];
	int                        nisPassCount;
	int                        nisSrcWidth;
	int                        nisSrcHeight;
	int                        nisDstWidth;
	int                        nisDstHeight;

	DXGI_FORMAT frameFormat;
	int  width;
	int  height;
	int  backWidth;
	int  backHeight;
	int  filterMode;
	bool failed;
	TextOverlay overlay;
	D3D11VERTEX quad[4];
};

Direct3D11Display::Direct3D11Display()
{
	hwnd          = NULL;
	pDevice       = NULL;
	pContext      = NULL;
	pSwapChain    = NULL;
	pRTV          = NULL;
	pFrameTexture = NULL;
	pFrameSRV     = NULL;
	pOverlayTexture = NULL;
	pOverlaySRV   = NULL;
	pVS           = NULL;
	pPS           = NULL;
	pLayout       = NULL;
	pSamplerPoint = NULL;
	pSamplerLinear = NULL;
	pBlendAlpha   = NULL;
	pVertexBuffer = NULL;
	featureLevel  = D3D_FEATURE_LEVEL_10_0;
	nisAvailable  = false;
	pNisCS        = NULL;
	pNisCB        = NULL;
	pNisCoefScale = NULL;
	pNisCoefScaleSRV = NULL;
	pNisCoefUsm   = NULL;
	pNisCoefUsmSRV = NULL;
	ZeroMemory(nisTargets, sizeof(nisTargets));
	nisPassCount  = 0;
	nisSrcWidth   = 0;
	nisSrcHeight  = 0;
	nisDstWidth   = 0;
	nisDstHeight  = 0;
	frameFormat   = DXGI_FORMAT_B8G8R8A8_UNORM;
	width         = 0;
	height        = 0;
	backWidth     = 0;
	backHeight    = 0;
	filterMode    = 0;
	failed        = false;
}

Direct3D11Display::~Direct3D11Display()
{
	cleanup();
}

void Direct3D11Display::cleanup()
{
	overlay.destroy();
	releaseAll();

	if (pContext)
	{
		pContext->ClearState();
		pContext->Flush();
	}
}

void Direct3D11Display::releaseAll()
{
	releaseFrameTexture();
	releaseNis();

	if (pOverlaySRV)   { pOverlaySRV->Release();   pOverlaySRV = NULL; }
	if (pOverlayTexture) { pOverlayTexture->Release(); pOverlayTexture = NULL; }
	if (pRTV)          { pRTV->Release();          pRTV = NULL; }
	if (pVertexBuffer) { pVertexBuffer->Release(); pVertexBuffer = NULL; }
	if (pBlendAlpha)   { pBlendAlpha->Release();   pBlendAlpha = NULL; }
	if (pSamplerLinear) { pSamplerLinear->Release(); pSamplerLinear = NULL; }
	if (pSamplerPoint) { pSamplerPoint->Release(); pSamplerPoint = NULL; }
	if (pLayout)       { pLayout->Release();       pLayout = NULL; }
	if (pPS)           { pPS->Release();           pPS = NULL; }
	if (pVS)           { pVS->Release();           pVS = NULL; }
	if (pSwapChain)    { pSwapChain->Release();    pSwapChain = NULL; }
	if (pContext)      { pContext->Release();      pContext = NULL; }
	if (pDevice)       { pDevice->Release();       pDevice = NULL; }
}

void Direct3D11Display::releaseFrameTexture()
{
	if (pFrameSRV)     { pFrameSRV->Release();     pFrameSRV = NULL; }
	if (pFrameTexture) { pFrameTexture->Release(); pFrameTexture = NULL; }
}

bool Direct3D11Display::createDevice()
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount                        = 1;
	sd.BufferDesc.Width                   = backWidth > 0 ? backWidth : 1;
	sd.BufferDesc.Height                  = backHeight > 0 ? backHeight : 1;
	sd.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator   = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow                       = hwnd;
	sd.SampleDesc.Count                   = 1;
	sd.SampleDesc.Quality                 = 0;
	sd.Windowed                           = TRUE;
	sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

	UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	D3D_FEATURE_LEVEL obtained;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
	                                           D3D_DRIVER_TYPE_HARDWARE,
	                                           NULL,
	                                           flags,
	                                           levels,
	                                           sizeof(levels) / sizeof(levels[0]),
	                                           D3D11_SDK_VERSION,
	                                           &sd,
	                                           &pSwapChain,
	                                           &pDevice,
	                                           &obtained,
	                                           &pContext);
	if (FAILED(hr))
	{
		winlog("D3D11CreateDeviceAndSwapChain failed %08x\n", hr);
		return false;
	}

	featureLevel = obtained;

	return createBackBufferViews();
}

bool Direct3D11Display::createBackBufferViews()
{
	ID3D11Texture2D *backBuffer = NULL;
	if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer)))
		return false;

	HRESULT hr = pDevice->CreateRenderTargetView(backBuffer, NULL, &pRTV);
	backBuffer->Release();
	return SUCCEEDED(hr);
}

bool Direct3D11Display::createShaders()
{
	ID3DBlob *vsBlob = NULL;
	ID3DBlob *psBlob = NULL;
	ID3DBlob *error = NULL;

	if (FAILED(D3DCompile(g_quadShader, strlen(g_quadShader), "quadVS", NULL, NULL,
	                      "VSMain", "vs_4_0", 0, 0, &vsBlob, &error)))
	{
		winlog("D3D11 vertex shader compilation failed: %s\n", error ? (const char *)error->GetBufferPointer() : "?");
		if (error) error->Release();
		return false;
	}
	if (FAILED(D3DCompile(g_quadShader, strlen(g_quadShader), "quadPS", NULL, NULL,
	                      "PSMain", "ps_4_0", 0, 0, &psBlob, &error)))
	{
		winlog("D3D11 pixel shader compilation failed: %s\n", error ? (const char *)error->GetBufferPointer() : "?");
		if (error) error->Release();
		vsBlob->Release();
		return false;
	}

	HRESULT hr = pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &pVS);
	if (SUCCEEDED(hr))
		hr = pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pPS);

	if (SUCCEEDED(hr))
	{
		D3D11_INPUT_ELEMENT_DESC elems[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = pDevice->CreateInputLayout(elems, 2, vsBlob->GetBufferPointer(),
		                                vsBlob->GetBufferSize(), &pLayout);
	}

	vsBlob->Release();
	psBlob->Release();
	return SUCCEEDED(hr);
}

bool Direct3D11Display::createSamplers()
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	if (FAILED(pDevice->CreateSamplerState(&sd, &pSamplerPoint)))
		return false;

	sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	if (FAILED(pDevice->CreateSamplerState(&sd, &pSamplerLinear)))
		return false;

	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable           = TRUE;
	bd.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (FAILED(pDevice->CreateBlendState(&bd, &pBlendAlpha)))
		return false;

	D3D11_BUFFER_DESC vb;
	ZeroMemory(&vb, sizeof(vb));
	vb.Usage          = D3D11_USAGE_DYNAMIC;
	vb.ByteWidth      = sizeof(D3D11VERTEX) * 4;
	vb.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(pDevice->CreateBuffer(&vb, NULL, &pVertexBuffer)))
		return false;

	return true;
}

bool Direct3D11Display::initialize()
{
	CWnd *pWnd = theApp.m_pMainWnd;

	hwnd      = pWnd->GetSafeHwnd();
	backWidth  = theApp.surfaceSizeX;
	backHeight = theApp.surfaceSizeY;

	theApp.mode320Available = false;
	theApp.mode640Available = false;
	theApp.mode800Available = false;

	// Pick the emulator pixel format from the desktop mode, like the D3D9 backend.
	HDC dc = GetDC(hwnd);
	int bits = GetDeviceCaps(dc, BITSPIXEL);
	ReleaseDC(hwnd, dc);

	if (bits <= 16)
	{
		systemColorDepth = 16;
		systemRedShift   = 11;
		systemGreenShift = 6;
		systemBlueShift  = 0;
		frameFormat      = DXGI_FORMAT_B5G6R5_UNORM;
		Init_2xSaI(565);
	}
	else
	{
		systemColorDepth = 32;
		// Same byte-aligned layout the D3D9 backend uses: a 5-bit component is
		// shifted into the top bits of its byte, so 0x1F becomes 0xF8 instead of
		// 0x1F.  Using 16/8/0 here renders the whole frame almost black.
		systemRedShift   = 19;
		systemGreenShift = 11;
		systemBlueShift  = 3;
		frameFormat      = DXGI_FORMAT_B8G8R8A8_UNORM;
		Init_2xSaI(32);
	}
	theApp.fsColorDepth = systemColorDepth;

#ifdef MMX
	if (!theApp.disableMMX)
		cpu_mmx = theApp.detectMMX();
	else
		cpu_mmx = 0;
#endif

	if (!createDevice())
	{
		systemMessage(0, "Failed to create Direct3D 11 device");
		return false;
	}
	if (!createShaders() || !createSamplers())
	{
		systemMessage(0, "Failed to initialise Direct3D 11 shaders");
		return false;
	}

	if (theApp.nisSharpness >= 0)
		initNis();

	filterMode = regQueryDwordValue("d3dFilter", 0);
	if (filterMode < 0 || filterMode > 3)
		filterMode = 0;

	systemUpdateColorMaps();

	theApp.updateFilter();
	theApp.updateIFB();

	if (failed)
		return false;

	pWnd->DragAcceptFiles(TRUE);
	return true;
}

bool Direct3D11Display::initializeOffscreen(int w, int h)
{
	releaseFrameTexture();

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width              = w;
	td.Height             = h;
	td.MipLevels          = 1;
	td.ArraySize          = 1;
	td.Format             = frameFormat;
	td.SampleDesc.Count   = 1;
	td.Usage              = D3D11_USAGE_DYNAMIC;
	td.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = pDevice->CreateTexture2D(&td, NULL, &pFrameTexture);
	if (FAILED(hr))
	{
		winlog("D3D11 CreateTexture2D failed %08x\n", hr);
		return false;
	}

	hr = pDevice->CreateShaderResourceView(pFrameTexture, NULL, &pFrameSRV);
	if (FAILED(hr))
	{
		releaseFrameTexture();
		return false;
	}

	if (pOverlayTexture == NULL && backWidth > 0 && backHeight > 0)
	{
		D3D11_TEXTURE2D_DESC od;
		ZeroMemory(&od, sizeof(od));
		od.Width            = backWidth;
		od.Height           = backHeight;
		od.MipLevels        = 1;
		od.ArraySize        = 1;
		od.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
		od.SampleDesc.Count = 1;
		od.Usage            = D3D11_USAGE_DYNAMIC;
		od.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
		od.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;

		if (SUCCEEDED(pDevice->CreateTexture2D(&od, NULL, &pOverlayTexture)))
			pDevice->CreateShaderResourceView(pOverlayTexture, NULL, &pOverlaySRV);
		overlay.create(backWidth, backHeight);
	}

	width  = w;
	height = h;
	return true;
}

void Direct3D11Display::updateFiltering(int filter)
{
	filterMode = filter;
}

// --- NVIDIA Image Scaling -------------------------------------------------

bool Direct3D11Display::initNis()
{
	if (nisAvailable)
		return true;

	if (pDevice == NULL)
		return false;

	if (featureLevel < D3D_FEATURE_LEVEL_11_0)
	{
		winlog("NIS: needs Direct3D feature level 11.0\n");
		return false;
	}

	D3D_SHADER_MACRO defines[] =
	{
		{ "NIS_SCALER",            "1" },
		{ "NIS_HDR_MODE",          "0" },
		{ "NIS_BLOCK_WIDTH",       NIS_STR(NIS_BLOCK_WIDTH) },
		{ "NIS_BLOCK_HEIGHT",      NIS_STR(NIS_BLOCK_HEIGHT) },
		{ "NIS_THREAD_GROUP_SIZE", NIS_STR(NIS_THREAD_GROUP_SIZE) },
		{ NULL, NULL }
	};

	NisIncludeHandler includeHandler;
	ID3DBlob *blob  = NULL;
	ID3DBlob *error = NULL;

	HRESULT hr = D3DCompile(g_nisMainShader, strlen(g_nisMainShader), "NIS_Main.hlsl",
	                        defines, &includeHandler, "main", "cs_5_0", 0, 0,
	                        &blob, &error);
	if (FAILED(hr))
	{
		winlog("NIS: shader compilation failed: %s\n",
		       error ? (const char *)error->GetBufferPointer() : "?");
		if (error)
			error->Release();
		return false;
	}
	if (error)
		error->Release();

	hr = pDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &pNisCS);
	blob->Release();
	if (FAILED(hr))
	{
		winlog("NIS: CreateComputeShader failed %08x\n", hr);
		return false;
	}

	// The filter bank is 64 phases of 8 taps; the SDK exposes it as two 2x64
	// RGBA32F textures (kFilterSize / 4 columns).
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width            = (UINT)(kFilterSize / 4);
	td.Height           = (UINT)kPhaseCount;
	td.MipLevels        = 1;
	td.ArraySize        = 1;
	td.Format           = DXGI_FORMAT_R32G32B32A32_FLOAT;
	td.SampleDesc.Count = 1;
	td.Usage            = D3D11_USAGE_IMMUTABLE;
	td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.SysMemPitch = (UINT)kFilterSize * sizeof(float);

	data.pSysMem = coef_scale;
	if (FAILED(pDevice->CreateTexture2D(&td, &data, &pNisCoefScale)))
	{
		winlog("NIS: cannot create the scaler coefficient texture\n");
		return false;
	}

	data.pSysMem = coef_usm;
	if (FAILED(pDevice->CreateTexture2D(&td, &data, &pNisCoefUsm)))
	{
		winlog("NIS: cannot create the sharpening coefficient texture\n");
		return false;
	}

	if (FAILED(pDevice->CreateShaderResourceView(pNisCoefScale, NULL, &pNisCoefScaleSRV)) ||
	    FAILED(pDevice->CreateShaderResourceView(pNisCoefUsm, NULL, &pNisCoefUsmSRV)))
	{
		winlog("NIS: cannot create the coefficient views\n");
		return false;
	}

	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.ByteWidth      = sizeof(NISConfig);
	cbd.Usage          = D3D11_USAGE_DYNAMIC;
	cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(pDevice->CreateBuffer(&cbd, NULL, &pNisCB)))
	{
		winlog("NIS: cannot create the config buffer\n");
		return false;
	}

	nisAvailable = true;
	return true;
}

void Direct3D11Display::releaseNisTargets()
{
	for (int i = 0; i < NIS_MAX_PASSES; i++)
	{
		if (nisTargets[i].uav)     { nisTargets[i].uav->Release();     nisTargets[i].uav = NULL; }
		if (nisTargets[i].srv)     { nisTargets[i].srv->Release();     nisTargets[i].srv = NULL; }
		if (nisTargets[i].texture) { nisTargets[i].texture->Release(); nisTargets[i].texture = NULL; }
		nisTargets[i].width  = 0;
		nisTargets[i].height = 0;
	}
	nisPassCount = 0;
}

void Direct3D11Display::releaseNis()
{
	releaseNisTargets();

	if (pNisCoefScaleSRV) { pNisCoefScaleSRV->Release(); pNisCoefScaleSRV = NULL; }
	if (pNisCoefUsmSRV)   { pNisCoefUsmSRV->Release();   pNisCoefUsmSRV   = NULL; }
	if (pNisCoefScale)    { pNisCoefScale->Release();    pNisCoefScale    = NULL; }
	if (pNisCoefUsm)      { pNisCoefUsm->Release();      pNisCoefUsm      = NULL; }
	if (pNisCB)           { pNisCB->Release();           pNisCB           = NULL; }
	if (pNisCS)           { pNisCS->Release();           pNisCS           = NULL; }
	nisAvailable = false;
}

bool Direct3D11Display::updateNisChain(int srcWidth, int srcHeight, int dstWidth, int dstHeight)
{
	if (nisPassCount > 0 && nisSrcWidth == srcWidth && nisSrcHeight == srcHeight &&
	    nisDstWidth == dstWidth && nisDstHeight == dstHeight)
		return true;

	releaseNisTargets();

	nisSrcWidth  = srcWidth;
	nisSrcHeight = srcHeight;
	nisDstWidth  = dstWidth;
	nisDstHeight = dstHeight;

	if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
		return false;

	// A pass may not shrink the image and may not scale by more than 2x, so a
	// window bigger than that is reached in steps of at most 2x.
	int w    = srcWidth;
	int h    = srcHeight;
	int pass = 0;

	while ((w < dstWidth || h < dstHeight) && pass < NIS_MAX_PASSES)
	{
		if (w < dstWidth)
			w = (w * 2 < dstWidth) ? w * 2 : dstWidth;
		if (h < dstHeight)
			h = (h * 2 < dstHeight) ? h * 2 : dstHeight;

		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(td));
		td.Width            = w;
		td.Height           = h;
		td.MipLevels        = 1;
		td.ArraySize        = 1;
		td.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage            = D3D11_USAGE_DEFAULT;
		td.BindFlags        = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		if (FAILED(pDevice->CreateTexture2D(&td, NULL, &nisTargets[pass].texture)) ||
		    FAILED(pDevice->CreateShaderResourceView(nisTargets[pass].texture, NULL, &nisTargets[pass].srv)) ||
		    FAILED(pDevice->CreateUnorderedAccessView(nisTargets[pass].texture, NULL, &nisTargets[pass].uav)))
		{
			winlog("NIS: cannot create the %dx%d pass target\n", w, h);
			releaseNisTargets();
			return false;
		}

		nisTargets[pass].width  = w;
		nisTargets[pass].height = h;
		pass++;
	}

	if (w != dstWidth || h != dstHeight)
	{
		// The back buffer is not reachable within NIS_MAX_PASSES, or it is
		// smaller than the frame, which NIS cannot scale down to.
		releaseNisTargets();
		return false;
	}

	nisPassCount = pass;
	return pass > 0;
}

ID3D11ShaderResourceView *Direct3D11Display::scaleFrameWithNis()
{
	if (!nisAvailable || pNisCS == NULL || pNisCB == NULL || pFrameSRV == NULL)
		return NULL;

	if (theApp.nisSharpness < 0)
		return NULL;

	// The chain maps the whole frame texture onto the whole back buffer, the
	// same mapping the textured quad would use.
	if (theApp.rect.right != width || theApp.rect.bottom != height)
		return NULL;

	if (!updateNisChain(width, height, backWidth, backHeight))
		return NULL;

	const float sharpness = theApp.nisSharpness / 100.0f;

	ID3D11ShaderResourceView *source = pFrameSRV;
	int srcWidth  = width;
	int srcHeight = height;

	for (int i = 0; i < nisPassCount; i++)
	{
		NisTarget& target = nisTargets[i];

		NISConfig config;
		ZeroMemory(&config, sizeof(config));
		if (!NVScalerUpdateConfig(config, sharpness,
		                          0, 0, srcWidth, srcHeight, srcWidth, srcHeight,
		                          0, 0, target.width, target.height,
		                          target.width, target.height, NISHDRMode::None))
		{
			return NULL;
		}

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (FAILED(pContext->Map(pNisCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return NULL;
		memcpy(mapped.pData, &config, sizeof(config));
		pContext->Unmap(pNisCB, 0);

		pContext->CSSetShader(pNisCS, NULL, 0);
		pContext->CSSetConstantBuffers(0, 1, &pNisCB);
		pContext->CSSetSamplers(0, 1, &pSamplerLinear);
		pContext->CSSetShaderResources(0, 1, &source);
		pContext->CSSetShaderResources(1, 1, &pNisCoefScaleSRV);
		pContext->CSSetShaderResources(2, 1, &pNisCoefUsmSRV);
		pContext->CSSetUnorderedAccessViews(0, 1, &target.uav, NULL);

		pContext->Dispatch((target.width + NIS_BLOCK_WIDTH - 1) / NIS_BLOCK_WIDTH,
		                   (target.height + NIS_BLOCK_HEIGHT - 1) / NIS_BLOCK_HEIGHT, 1);

		// Unbind so the next pass, and the quad draw, can read the result.
		ID3D11UnorderedAccessView *nullUav    = NULL;
		ID3D11ShaderResourceView  *nullSrv[3] = { NULL, NULL, NULL };
		pContext->CSSetUnorderedAccessViews(0, 1, &nullUav, NULL);
		pContext->CSSetShaderResources(0, 3, nullSrv);

		source    = target.srv;
		srcWidth  = target.width;
		srcHeight = target.height;
	}

	return source;
}

void Direct3D11Display::clear()
{}

void Direct3D11Display::renderMenu()
{
	checkFullScreen();
	if (theApp.m_pMainWnd)
		theApp.m_pMainWnd->DrawMenuBar();
}

void Direct3D11Display::checkFullScreen()
{
}

void Direct3D11Display::calculateVertices()
{
	float left   = 0.0f;
	float top    = 0.0f;
	float right  = (float)backWidth;
	float bottom = (float)backHeight;

	// Convert to normalised device coordinates.
	float l = (left   / (float)backWidth)  * 2.0f - 1.0f;
	float r = (right  / (float)backWidth)  * 2.0f - 1.0f;
	float t = 1.0f - (top    / (float)backHeight) * 2.0f;
	float b = 1.0f - (bottom / (float)backHeight) * 2.0f;

	float u = (float)theApp.rect.right / (float)width;
	float v = (float)theApp.rect.bottom / (float)height;

	quad[0].x = l; quad[0].y = b; quad[0].u = 0.0f; quad[0].v = v;
	quad[1].x = l; quad[1].y = t; quad[1].u = 0.0f; quad[1].v = 0.0f;
	quad[2].x = r; quad[2].y = b; quad[2].u = u;    quad[2].v = v;
	quad[3].x = r; quad[3].y = t; quad[3].u = u;    quad[3].v = 0.0f;
}

void Direct3D11Display::drawQuad(ID3D11ShaderResourceView *srv, D3D11VERTEX *vertices, bool blend)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(pContext->Map(pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return;
	memcpy(mapped.pData, vertices, sizeof(D3D11VERTEX) * 4);
	pContext->Unmap(pVertexBuffer, 0);

	UINT stride = sizeof(D3D11VERTEX);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	pContext->IASetInputLayout(pLayout);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	pContext->VSSetShader(pVS, NULL, 0);
	pContext->PSSetShader(pPS, NULL, 0);

	ID3D11SamplerState *sampler = (filterMode == 0) ? pSamplerPoint : pSamplerLinear;
	pContext->PSSetSamplers(0, 1, &sampler);
	pContext->PSSetShaderResources(0, 1, &srv);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pContext->OMSetBlendState(blend ? pBlendAlpha : NULL, blendFactor, 0xFFFFFFFF);

	pContext->Draw(4, 0);
}

void Direct3D11Display::render()
{
	if (pDevice == NULL || pContext == NULL || pSwapChain == NULL)
		return;

	if (pDevice->GetDeviceRemovedReason() != S_OK)
		return;

	if (pFrameTexture == NULL && (theApp.rect.right > 0))
	{
		if (!initializeOffscreen(theApp.rect.right, theApp.rect.bottom))
			return;
		calculateVertices();
	}

	u8 *data = pix;
	int dataPitch = theApp.filterWidth * (systemColorDepth / 8) + (systemColorDepth == 24 ? 0 : 4);

	if (textMethod != 0) // do not draw Lua HUD to a video dump
	{
		systemClonePixBuffer(osd);
		data = osd;
		systemRenderLua((u8 *)data, dataPitch);
	}

	if (pFrameTexture != NULL)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(pContext->Map(pFrameTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			if (theApp.filterFunction)
			{
				theApp.filterFunction(data + dataPitch,
				                      dataPitch,
				                      (u8 *)theApp.delta,
				                      (u8 *)mapped.pData,
				                      mapped.RowPitch,
				                      theApp.filterWidth,
				                      theApp.filterHeight);
			}
			else
			{
				int copyX, copyY;
				systemGetLCDResolution(copyX, copyY);

				int bytesPerPixel = systemColorDepth / 8;
				int srcPitch      = dataPitch;
				const u8 *srcRow  = data + srcPitch;
				u8 *dstRow        = (u8 *)mapped.pData;
				int rowBytes      = copyX * bytesPerPixel;
				if (rowBytes > (int)mapped.RowPitch)
					rowBytes = (int)mapped.RowPitch;

				for (int y = 0; y < copyY; y++)
				{
					memcpy(dstRow, srcRow, rowBytes);
					srcRow += srcPitch;
					dstRow += mapped.RowPitch;
				}
			}

			if (isFullscreenOption(theApp.videoOption) && theApp.showSpeed)
			{
				char buffer[30];
				if (theApp.showSpeed == 1)
					sprintf(buffer, "%3d%%", systemSpeed);
				else
					sprintf(buffer, "%3d%%(%d, %d fps)", systemSpeed,
					        systemFrameSkip, theApp.showRenderedFrames);
				if (theApp.showSpeedTransparent)
					drawTextTransp((u8 *)mapped.pData, mapped.RowPitch,
					               theApp.rect.left + 10, theApp.rect.bottom - 10, buffer);
				else
					drawText((u8 *)mapped.pData, mapped.RowPitch,
					         theApp.rect.left + 10, theApp.rect.bottom - 10, buffer);
			}

			if (textMethod == 1)
			{
				DrawTextMessages((u8 *)mapped.pData, mapped.RowPitch,
				                 theApp.rect.left, theApp.rect.bottom);
			}

			pContext->Unmap(pFrameTexture, 0);
		}
	}

	// Optionally let the NIS compute shader scale the frame up to the back
	// buffer instead of the sampler.
	ID3D11ShaderResourceView *frameView = pFrameSRV;
	bool nisScaled = false;

	if (pFrameSRV != NULL && theApp.nisSharpness >= 0)
	{
		ID3D11ShaderResourceView *scaled = scaleFrameWithNis();
		if (scaled != NULL)
		{
			frameView = scaled;
			nisScaled = true;
		}
	}

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	pContext->OMSetRenderTargets(1, &pRTV, NULL);
	pContext->ClearRenderTargetView(pRTV, clearColor);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width    = (float)backWidth;
	vp.Height   = (float)backHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	pContext->RSSetViewports(1, &vp);

	if (frameView != NULL)
	{
		if (nisScaled)
		{
			// The NIS output covers exactly the back buffer, so it is drawn 1:1.
			D3D11VERTEX fullQuad[4];
			fullQuad[0].x = -1.0f; fullQuad[0].y = -1.0f; fullQuad[0].u = 0.0f; fullQuad[0].v = 1.0f;
			fullQuad[1].x = -1.0f; fullQuad[1].y =  1.0f; fullQuad[1].u = 0.0f; fullQuad[1].v = 0.0f;
			fullQuad[2].x =  1.0f; fullQuad[2].y = -1.0f; fullQuad[2].u = 1.0f; fullQuad[2].v = 1.0f;
			fullQuad[3].x =  1.0f; fullQuad[3].y =  1.0f; fullQuad[3].u = 1.0f; fullQuad[3].v = 0.0f;

			int savedFilter = filterMode;
			filterMode = 0; // 1:1, no sampling needed
			drawQuad(frameView, fullQuad, false);
			filterMode = savedFilter;
		}
		else
		{
			drawQuad(frameView, quad, false);
		}
	}

	if (textMethod == 2 && pOverlayTexture != NULL && !overlay.empty())
	{
		if (overlay.update())
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(pContext->Map(pOverlayTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				const unsigned char *src = overlay.pixels();
				int srcPitch = overlay.pitch();
				int copyBytes = overlay.width() * 4;
				if (copyBytes > (int)mapped.RowPitch)
					copyBytes = (int)mapped.RowPitch;
				for (int y = 0; y < overlay.height(); y++)
					memcpy((unsigned char *)mapped.pData + y * mapped.RowPitch,
					       src + y * srcPitch, copyBytes);
				pContext->Unmap(pOverlayTexture, 0);
			}

			D3D11VERTEX overlayQuad[4];
			overlayQuad[0].x = -1.0f; overlayQuad[0].y =  1.0f; overlayQuad[0].u = 0.0f; overlayQuad[0].v = 1.0f;
			overlayQuad[1].x = -1.0f; overlayQuad[1].y = -1.0f; overlayQuad[1].u = 0.0f; overlayQuad[1].v = 0.0f;
			overlayQuad[2].x =  1.0f; overlayQuad[2].y =  1.0f; overlayQuad[2].u = 1.0f; overlayQuad[2].v = 1.0f;
			overlayQuad[3].x =  1.0f; overlayQuad[3].y = -1.0f; overlayQuad[3].u = 1.0f; overlayQuad[3].v = 0.0f;

			int savedFilter = filterMode;
			filterMode = 0; // point sampling for a 1:1 overlay
			drawQuad(pOverlaySRV, overlayQuad, true);
			filterMode = savedFilter;
		}
	}

	// Nothing must stay bound as a shader resource: the next frame writes the
	// frame texture again and may bind an NIS target as a UAV.
	ID3D11ShaderResourceView *nullView = NULL;
	pContext->PSSetShaderResources(0, 1, &nullView);

	pSwapChain->Present(1, 0);
}

void Direct3D11Display::resize(int w, int h)
{
	if (pSwapChain == NULL || w <= 0 || h <= 0)
		return;

	backWidth  = w;
	backHeight = h;

	if (pRTV)
	{
		pRTV->Release();
		pRTV = NULL;
	}
	pContext->OMSetRenderTargets(0, NULL, NULL);

	HRESULT hr = pSwapChain->ResizeBuffers(1, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
	if (SUCCEEDED(hr))
		createBackBufferViews();
	else
		systemMessage(0, "Failed swap chain resize %08x", hr);

	// Recreate the overlay at the new back buffer size.
	if (pOverlaySRV)     { pOverlaySRV->Release();     pOverlaySRV = NULL; }
	if (pOverlayTexture) { pOverlayTexture->Release(); pOverlayTexture = NULL; }
	overlay.destroy();

	calculateVertices();
}

bool Direct3D11Display::changeRenderSize(int w, int h)
{
	if (w != width || h != height)
	{
		if (!initializeOffscreen(w, h))
		{
			failed = true;
			return false;
		}
	}
	calculateVertices();
	return true;
}

void Direct3D11Display::setOption(const char *option, int value)
{
	if (!strcmp(option, "d3dFilter"))
	{
		updateFiltering(value);
	}
	else if (!strcmp(option, "nis"))
	{
		// The shaders and the coefficient textures are only built on demand.
		if (value >= 0 && !nisAvailable && !initNis())
			systemScreenMessage("NVIDIA Image Scaling is not available on this device");
	}
}

int Direct3D11Display::selectFullScreenMode(GUID * *)
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

IDisplay *newDirect3D11Display()
{
	return new Direct3D11Display();
}
