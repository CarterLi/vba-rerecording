// OpenGL3.cpp: OpenGL 3.3 display backend.
//
// The frame is uploaded into a texture and drawn as one textured quad through a
// shader pipeline, and the on screen display is blended on top from a texture
// the TextOverlay the Direct3D 11 backend uses fills, so the two renderers show
// the same thing.  A core profile context of at least 3.3 is asked for, which is
// not what the fixed function renderer in OpenGL.cpp uses; that one stays as it
// is and the two are offered next to each other as "OpenGL 3.3" and
// "OpenGL (Legacy)".
//
// The frame can be scaled up by a shader instead of by the sampler: the single
// pass xBRZ pixel filter and AMD FidelityFX Super Resolution 1.0, the two
// upscalers that do not need a compute shader.  Both are the Direct3D 11
// backend's, ported to GLSL in src/win32/upscalers, and both stack with the CPU
// pixel filter the same way they do there.
//
// When the system cannot hand out a 3.3 core context the renderer does not
// start and VBA::updateRenderMethod() falls back to the renderer below it, the
// fixed function OpenGL one, and from there on down the list.
//
// Vertical synchronisation follows theApp.vsync through wglSwapIntervalEXT,
// like the Direct3D 11 backend follows it through its Present interval, and
// uses adaptive synchronisation where the driver has it.

#include "stdafx.h"

#include <stddef.h>
#include <gl/GL.h>

#include "resource.h"
#include "MainWnd.h"
#include "Reg.h"
#include "TextOverlay.h"
#include "VBA.h"

#include "../gba/GBAGlobals.h"
#include "../gb/gbGlobals.h"
#include "../common/SystemGlobals.h"
#include "../common/Text.h"
#include "../version.h"

// fsrEasuCon() and fsrRcasCon(), the shader constants both backends hand to the
// unmodified AMD headers.
#include "upscalers/UpscalerFsr.h"

extern int systemSpeed;
extern int Init_2xSaI(u32);
extern void winlog(const char *, ...);

// ---------------------------------------------------------------------------
// The OpenGL entry points the renderer calls.
//
// opengl32.dll exports OpenGL 1.1 and nothing more, and the SDK's headers are
// from that time as well, so everything newer - the shaders, the buffer objects,
// the framebuffer objects, the texture parameters - is declared here and
// resolved through wglGetProcAddress() while a context is current.

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char      GLchar;

#define GL_ARRAY_BUFFER          0x8892
#define GL_STREAM_DRAW           0x88E0
#define GL_STATIC_DRAW           0x88E1
#define GL_TEXTURE0              0x84C0
#define GL_VERTEX_SHADER         0x8B31
#define GL_FRAGMENT_SHADER       0x8B30
#define GL_COMPILE_STATUS        0x8B81
#define GL_LINK_STATUS           0x8B82
#define GL_INFO_LOG_LENGTH       0x8B84
#define GL_FRAMEBUFFER           0x8D40
#define GL_COLOR_ATTACHMENT0     0x8CE0
#define GL_FRAMEBUFFER_COMPLETE  0x8CD5
#define GL_CLAMP_TO_EDGE         0x812F
#define GL_BGRA                  0x80E1
#define GL_MAJOR_VERSION         0x821B
#define GL_MINOR_VERSION         0x821C
#define GL_ACTIVE_UNIFORMS       0x8B86
#define GL_SAMPLER_2D            0x8B5E
#define GL_TEXTURE1              0x84C1
#define GL_TEXTURE2              0x84C2
#define GL_TEXTURE3              0x84C3

typedef void   (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void   (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void   (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void   (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef void   (APIENTRY *PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
typedef void   (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void   (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void   (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void   (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void   (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void   (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void   (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
typedef void   (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void   (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void   (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void   (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void   (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void   (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void   (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void   (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef GLint  (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void   (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void   (APIENTRY *PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void   (APIENTRY *PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void   (APIENTRY *PFNGLUNIFORM4UIVPROC)(GLint location, GLsizei count, const GLuint *value);
typedef void   (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void   (APIENTRY *PFNGLBINDATTRIBLOCATIONPROC)(GLuint program, GLuint index, const GLchar *name);
typedef void   (APIENTRY *PFNGLGETACTIVEUNIFORMPROC)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void   (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void   (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void   (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void   (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);

#define GL3_FUNCTIONS(X) \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
	X(PFNGLGENBUFFERSPROC, glGenBuffers) \
	X(PFNGLBINDBUFFERPROC, glBindBuffer) \
	X(PFNGLBUFFERDATAPROC, glBufferData) \
	X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
	X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers) \
	X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
	X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
	X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays) \
	X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
	X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
	X(PFNGLDELETESHADERPROC, glDeleteShader) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \
	X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLUNIFORM1IPROC, glUniform1i) \
	X(PFNGLUNIFORM2FPROC, glUniform2f) \
	X(PFNGLUNIFORM4FPROC, glUniform4f) \
	X(PFNGLUNIFORM4UIVPROC, glUniform4uiv) \
	X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
	X(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation) \
	X(PFNGLGETACTIVEUNIFORMPROC, glGetActiveUniform) \
	X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
	X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
	X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
	X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
	X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)

#define GL3_DECLARE(type, name) static type name = NULL;
GL3_FUNCTIONS(GL3_DECLARE)
#undef GL3_DECLARE

// The entry points come from wglGetProcAddress() and, for the few drivers that
// only export them from the DLL, from opengl32.dll itself.  The DLL is linked,
// so its module handle is always there.
static bool loadGL3Functions()
{
	HMODULE module = GetModuleHandle(_T("opengl32.dll"));
	bool complete = true;

#define GL3_RESOLVE(type, name) \
	name = (type)wglGetProcAddress(#name); \
	if (name == NULL) \
		name = (type)GetProcAddress(module, #name); \
	if (name == NULL) \
	{ \
		winlog("OpenGL 3.3: %s is not available\n", #name); \
		complete = false; \
	}

	GL3_FUNCTIONS(GL3_RESOLVE)

#undef GL3_RESOLVE

	return complete;
}

// ---------------------------------------------------------------------------
// The WGL entry points and constants of the extensions the context is made of.

#define WGL_DRAW_TO_WINDOW_ARB           0x2001
#define WGL_ACCELERATION_ARB             0x2003
#define WGL_SUPPORT_OPENGL_ARB           0x2010
#define WGL_DOUBLE_BUFFER_ARB            0x2011
#define WGL_PIXEL_TYPE_ARB               0x2013
#define WGL_COLOR_BITS_ARB               0x2014
#define WGL_FULL_ACCELERATION_ARB        0x2027
#define WGL_TYPE_RGBA_ARB                0x202B
#define WGL_CONTEXT_MAJOR_VERSION_ARB    0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB    0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB     0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

typedef BOOL  (WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int *);
typedef BOOL  (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
typedef const char *(WINAPI *PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);

static PFNWGLCHOOSEPIXELFORMATARBPROC     wglChoosePixelFormatARB   = NULL;
static PFNWGLCREATECONTEXTATTRIBSARBPROC  wglCreateContextAttribsARB = NULL;
static PFNWGLSWAPINTERVALEXTPROC          wglSwapIntervalEXT         = NULL;
static PFNWGLGETEXTENSIONSSTRINGEXTPROC   wglGetExtensionsStringEXT  = NULL;

// The version and profile the running renderer really got, for the render
// method menu entry, which reads "OpenGL 3.3 (4.6 core)".
static char g_openGL33Version[32] = "";

// The hidden window the modern entry points are looked up on: SetPixelFormat()
// can be called once per window and the window the emulator draws into is
// already the fixed function renderer's by the time this is asked for.
static const TCHAR *g_setupWindowClass = _T("VBAOpenGL33Setup");

static HWND createSetupWindow()
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_OWNDC;
	wc.lpfnWndProc   = DefWindowProc;
	wc.hInstance     = AfxGetInstanceHandle();
	wc.lpszClassName = g_setupWindowClass;

	if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		return NULL;

	return CreateWindowEx(0, g_setupWindowClass, g_setupWindowClass, WS_POPUP,
	                      0, 0, 1, 1, NULL, NULL, AfxGetInstanceHandle(), NULL);
}

// ---------------------------------------------------------------------------
// The shader sources.
//
// The upscalers are stored in the executable as RCDATA resources rather than in
// string literals, because the FidelityFX headers are far past the 64 kB a
// single C++ literal may hold; see src/win32/upscalers.  GLSL has no #include,
// so the loader splices the headers in itself, the way the Direct3D 11 compiler
// does it for the HLSL sources.

// The quad every pass of the renderer is drawn with.  The upscalers are driven
// by the varying it hands them, so they declare it under the same name.
static const char *g_quadVertexShader =
    "#version 330 core\n"
    "\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "\n"
    "out vec2 uv;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    uv = texCoord;\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

static const char *g_quadFragmentShader =
    "#version 330 core\n"
    "\n"
    "in vec2 uv;\n"
    "\n"
    "uniform sampler2D frameTex;\n"
    "\n"
    "out vec4 color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    color = texture(frameTex, uv);\n"
    "}\n";

static const char *upscalerShaderSource(LPCSTR name, DWORD *size)
{
	HRSRC resource = FindResource(NULL, name, RT_RCDATA);
	if (resource == NULL)
	{
		winlog("upscaler: %s is missing from the executable\n", name);
		return NULL;
	}

	HGLOBAL handle = LoadResource(NULL, resource);
	if (handle == NULL)
	{
		winlog("upscaler: cannot load %s\n", name);
		return NULL;
	}

	if (size)
		*size = SizeofResource(NULL, resource);
	return (const char *)LockResource(handle);
}

// The files the shaders include, as they are stored in the executable.  The
// FidelityFX ones are the resources the Direct3D 11 backend compiles from too,
// and the body is the GLSL counterpart of its FSR_Main.hlsl.
struct UpscalerInclude
{
	const char *file;     // the name as the shader includes it
	const char *resource; // the RCDATA resource holding it
};

static const UpscalerInclude g_upscalerIncludes[] =
{
	{ "ffx_a.h",       "FSR_SHADER_A" },
	{ "ffx_fsr1.h",    "FSR_SHADER_FSR1" },
	{ "FSR_Body.glsl", "FSR_GLSL_BODY" },
};

#define UPSCALER_INCLUDE_COUNT (sizeof(g_upscalerIncludes) / sizeof(g_upscalerIncludes[0]))

// Copies a shader source into `source`, replacing every #include directive with
// the text of the resource it names.  A newline is kept where a line ends, so
// that the line numbers of each file stay what the file itself says.
static bool spliceShaderSource(const char *text, DWORD size, CString &source, int depth, const char *name)
{
	const char *p = text;
	const char *end = text + size;

	while (p < end)
	{
		const char *lineEnd = p;
		while (lineEnd < end && *lineEnd != '\n' && *lineEnd != '\r')
			lineEnd++;

		const char *s = p;
		while (s < lineEnd && (*s == ' ' || *s == '\t'))
			s++;

		const char *open = NULL;
		if (lineEnd - s > 10 && !strncmp(s, "#include", 8))
		{
			const char *q = s + 8;
			while (q < lineEnd && (*q == ' ' || *q == '\t'))
				q++;
			if (q < lineEnd && *q == '"')
				open = q + 1;
		}

		if (open == NULL)
		{
			source.Append(p, (int)(lineEnd - p));
			source += "\n";
		}
		else
		{
			const char *close = open;
			while (close < lineEnd && *close != '"')
				close++;

			if (close == lineEnd)
			{
				winlog("%s: unterminated #include\n", name);
				return false;
			}

			CString includeName(open, (int)(close - open));

			const char *includeText = NULL;
			DWORD includeSize = 0;

			for (UINT i = 0; i < UPSCALER_INCLUDE_COUNT; i++)
				if (!_stricmp(includeName, g_upscalerIncludes[i].file))
				{
					includeText = upscalerShaderSource(g_upscalerIncludes[i].resource, &includeSize);
					break;
				}

			if (includeText == NULL)
			{
				winlog("%s: %s is not available\n", name, (const char *)includeName);
				return false;
			}

			if (depth > 4)
			{
				winlog("%s: #include nested too deeply\n", name);
				return false;
			}

			if (!spliceShaderSource(includeText, includeSize, source, depth + 1, name))
				return false;

			source += "\n";
		}

		p = lineEnd;
		while (p < end && (*p == '\n' || *p == '\r'))
			p++;
	}

	return true;
}

// Compiles one shader stage.  Returns 0 and logs what the driver said when it
// does not build.
static GLuint compileShader(GLenum stage, const char *source, const char *name)
{
	const char *stageName = stage == GL_VERTEX_SHADER ? "vertex" : "fragment";

	GLuint shader = glCreateShader(stage);
	if (shader == 0)
	{
		winlog("%s: cannot create the %s shader\n", name, stageName);
		return 0;
	}

	const GLchar *text = source;
	glShaderSource(shader, 1, &text, NULL);
	glCompileShader(shader);

	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		char log[4096];
		GLsizei length = 0;
		glGetShaderInfoLog(shader, sizeof(log) - 1, &length, log);
		log[length] = 0;
		winlog("%s: %s shader does not compile: %s\n", name, stageName, log);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint linkProgram(GLuint vertex, GLuint fragment, const char *name)
{
	GLuint program = glCreateProgram();
	if (program == 0)
	{
		winlog("%s: cannot create the program\n", name);
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return 0;
	}

	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		char log[4096];
		GLsizei length = 0;
		glGetProgramInfoLog(program, sizeof(log) - 1, &length, log);
		log[length] = 0;
		winlog("%s: does not link: %s\n", name, log);
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

// The quad program: the frame texture, an upscaler target and the overlay are
// all drawn with it.
static GLuint createQuadProgram()
{
	GLuint vertex = compileShader(GL_VERTEX_SHADER, g_quadVertexShader, "the quad");
	if (vertex == 0)
		return 0;

	GLuint fragment = compileShader(GL_FRAGMENT_SHADER, g_quadFragmentShader, "the quad");
	if (fragment == 0)
	{
		glDeleteShader(vertex);
		return 0;
	}

	return linkProgram(vertex, fragment, "the quad");
}

// One of the upscalers: the quad vertex shader and the fragment shader stored
// in the executable under `resource`.
static GLuint createUpscalerProgram(LPCSTR resource, const char *name)
{
	DWORD size = 0;
	const char *text = upscalerShaderSource(resource, &size);
	if (text == NULL)
		return 0;

	CString source;
	if (!spliceShaderSource(text, size, source, 0, name))
		return 0;

	GLuint vertex = compileShader(GL_VERTEX_SHADER, g_quadVertexShader, name);
	if (vertex == 0)
		return 0;

	GLuint fragment = compileShader(GL_FRAGMENT_SHADER, source, name);
	if (fragment == 0)
	{
		glDeleteShader(vertex);
		return 0;
	}

	return linkProgram(vertex, fragment, name);
}

// ---------------------------------------------------------------------------
// The render targets the upscalers draw into.

struct UpscaleTarget
{
	GLuint texture;
	GLuint framebuffer;
	int    width;
	int    height;
};

static void releaseUpscaleTarget(UpscaleTarget &target)
{
	if (target.framebuffer != 0)
	{
		glDeleteFramebuffers(1, &target.framebuffer);
		target.framebuffer = 0;
	}
	if (target.texture != 0)
	{
		glDeleteTextures(1, &target.texture);
		target.texture = 0;
	}
	target.width  = 0;
	target.height = 0;
}

// (Re)creates the target unless it already has the size wanted.  The texture is
// attached to a framebuffer, so a pass can draw into it.
static bool updateUpscaleTarget(UpscaleTarget &target, int width, int height, const char *name)
{
	if (target.texture != 0 && target.width == width && target.height == height)
		return true;

	releaseUpscaleTarget(target);

	if (width <= 0 || height <= 0)
		return false;

	glGenTextures(1, &target.texture);
	glBindTexture(GL_TEXTURE_2D, target.texture);

	// Drawn 1:1 when the target is the size of the window, and stretched by the
	// sampler when the upscalers were capped, so it follows the same filtering
	// the frame does; the FSR passes read it with texelFetch, where it does not
	// come into it.
	GLint filter = theApp.glFilter == 1 ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffers(1, &target.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		winlog("%s: cannot create the %dx%d target\n", name, width, height);
		releaseUpscaleTarget(target);
		return false;
	}

	target.width  = width;
	target.height = height;
	return true;
}

// ---------------------------------------------------------------------------
// The single pass upscalers: xBRZ and anything else like it.
//
// One pixel shader, one block of constants and one render target the size of the
// window.  The pass maps the whole frame texture onto the whole window, the same
// mapping the textured quad would use, so it stacks with the CPU pixel filter
// and with the sharpening.  Adding another one is a row of the table below, its
// source as a resource in vba.rc, a UPSCALER_* value and a menu item.

struct SinglePassUpscalerSource
{
	int         value;    // the UPSCALER_* value it implements
	const char *name;     // for the trace log and the screen message
	const char *resource; // the RCDATA resource holding its source
};

static const SinglePassUpscalerSource g_singlePassUpscalers[] =
{
	{ UPSCALER_XBRZ, "xBRZ", "XBRZ_GLSL_FREESCALE" },
};

#define SINGLE_PASS_UPSCALER_COUNT (sizeof(g_singlePassUpscalers) / sizeof(g_singlePassUpscalers[0]))

struct SinglePassUpscalerState
{
	GLuint program;
	GLint  sourceSizeLocation;
	GLint  outputSizeLocation;
	GLint  scaleLocation;
	GLint  sourceLocation;
	UpscaleTarget target;
};

static SinglePassUpscalerState g_singlePassUpscalerState[SINGLE_PASS_UPSCALER_COUNT];

// The row of the table a UPSCALER_* value is, or -1.
static int singlePassUpscalerIndex(int upscaler)
{
	for (int i = 0; i < (int)SINGLE_PASS_UPSCALER_COUNT; i++)
		if (g_singlePassUpscalers[i].value == upscaler)
			return i;

	return -1;
}

// The single pass shaders are built while the renderer starts up rather than on
// demand, so that the upscaling menu knows whether they are there before they
// are selected.  Failing is not fatal, the entry is then left out.
static void initSinglePassUpscalers()
{
	for (int i = 0; i < (int)SINGLE_PASS_UPSCALER_COUNT; i++)
	{
		const SinglePassUpscalerSource &source = g_singlePassUpscalers[i];
		SinglePassUpscalerState        &state  = g_singlePassUpscalerState[i];

		state.program = createUpscalerProgram(source.resource, source.name);
		if (state.program == 0)
			continue;

		state.sourceSizeLocation = glGetUniformLocation(state.program, "SourceSize");
		state.outputSizeLocation = glGetUniformLocation(state.program, "OutputSize");
		state.scaleLocation      = glGetUniformLocation(state.program, "Scale");
		state.sourceLocation     = glGetUniformLocation(state.program, "frameTex");
	}
}

static void releaseSinglePassUpscalers()
{
	for (int i = 0; i < (int)SINGLE_PASS_UPSCALER_COUNT; i++)
	{
		SinglePassUpscalerState &state = g_singlePassUpscalerState[i];

		if (state.program != 0)
		{
			glDeleteProgram(state.program);
			state.program = 0;
		}

		releaseUpscaleTarget(state.target);
	}
}

// ---------------------------------------------------------------------------
// The libretro chains.
//
// These are the shaders of libretro/glsl-shaders as they are upstream rather
// than ports: every one of those files carries both stages of a pass, selected
// by the VERTEX and FRAGMENT macros, so the renderer compiles the same source
// twice and takes the part it needs, the way RetroArch does.  It also plays
// the part of RetroArch's shader backend: it hands the stages the attributes,
// the uniforms and the textures of the passes before them that they expect, so
// that the sources stay readable and diffable against upstream.
//
// A chain is a row of the table below, its passes in the order the preset runs
// them.  A pass either renders at a fixed multiple of the frame, which is how
// the passes that only compute metrics work, or at the target size, which is
// what the last one does and what makes an upscaler factor agnostic.

// The most passes any of the chains has; ScaleFX has five.
#define LIBRETRO_MAX_PASSES 5

struct LibretroPass
{
	const char *resource; // the RCDATA resource holding the source of the pass
	int         scale;    // 0: the target size, n: n times the size of the frame
	bool        linear;   // the pass reads its input with linear filtering
};

struct LibretroShader
{
	int                value;     // the UPSCALER_* value it implements
	const char        *name;      // for the trace log and the screen message
	const LibretroPass *passes;
	int                passCount;
};

// ScaleFX prepares the metrics of an edge in four passes on the frame itself
// and interpolates the edge in the fifth, at whatever size the target has.
static const LibretroPass g_scalefxPasses[] =
{
	{ "SCALEFX_PASS0_GLSL", 1, false },
	{ "SCALEFX_PASS1_GLSL", 1, false },
	{ "SCALEFX_PASS2_GLSL", 1, false },
	{ "SCALEFX_PASS3_GLSL", 1, false },
	{ "SCALEFX_PASS4_GLSL", 0, false },
};

// xBR-lv3 in two passes: one collects the angle of the edge, one interpolates.
static const LibretroPass g_xbrLv3MultipassPasses[] =
{
	{ "XBR_LV3_PASS0_GLSL", 1, false },
	{ "XBR_LV3_PASS1_GLSL", 0, false },
};

static const LibretroPass g_xbrLv2Passes[] =
{
	{ "XBR_LV2_GLSL", 0, false },
};

static const LibretroPass g_xbrLv2NoBlendPasses[] =
{
	{ "XBR_LV2_NOBLEND_GLSL", 0, false },
};

static const LibretroPass g_xbrLv3Passes[] =
{
	{ "XBR_LV3_GLSL", 0, false },
};

// 2xBR in two passes: the corner analysis, then the blend at the target size.
static const LibretroPass g_xbr2xPasses[] =
{
	{ "XBR_2X_PASS0_GLSL", 1, false },
	{ "XBR_2X_PASS1_GLSL", 0, false },
};

// xBR-mlv4 keeps two frames of angle data on the frame itself before it
// doubles the picture and interpolates it.
static const LibretroPass g_xbrMlv4Passes[] =
{
	{ "XBR_MLV4_PASS1_GLSL", 1, false },
	{ "XBR_MLV4_PASS2_GLSL", 1, false },
	{ "XBR_MLV4_PASS3_GLSL", 2, false },
	{ "XBR_MLV4_PASS4_GLSL", 0, false },
};

// super-xBR: the edge data first, then the 2x picture blended with the frame,
// which is what its PassPrev2Texture is, and finally the jinc2 windowed filter
// that finishes the picture at the target size.
static const LibretroPass g_superXbrPasses[] =
{
	{ "SUPER_XBR_PASS0_GLSL", 1, false },
	{ "SUPER_XBR_PASS1_GLSL", 2, false },
	{ "CUSTOM_JINC2_SHARPER_GLSL", 0, false },
};

// The Pixel Transparency shader of the handheld folder, which needs nothing
// but the frame it is handed.
static const LibretroPass g_ptSkyWalker541Passes[] =
{
	{ "PT_SKYWALKER541_GLSL", 0, false },
};

static const LibretroShader g_libretroShaders[] =
{
	{ UPSCALER_XBR_LV2,           "xBR lv2",              g_xbrLv2Passes,             1 },
	{ UPSCALER_XBR_LV2_NOBLEND,   "xBR lv2 no-blend",     g_xbrLv2NoBlendPasses,      1 },
	{ UPSCALER_XBR_LV3,           "xBR lv3",              g_xbrLv3Passes,             1 },
	{ UPSCALER_XBR_LV3_MULTIPASS, "xBR lv3 multipass",    g_xbrLv3MultipassPasses,    2 },
	{ UPSCALER_XBR_2X,            "2xBR",                 g_xbr2xPasses,              2 },
	{ UPSCALER_XBR_MLV4,          "xBR-mlv4",             g_xbrMlv4Passes,            4 },
	{ UPSCALER_SUPER_XBR,         "super-xBR",            g_superXbrPasses,           3 },
	{ UPSCALER_SCALEFX,           "ScaleFX",              g_scalefxPasses,            5 },
	{ UPSCALER_PT_SKYWALKER,      "PT SkyWalker541",      g_ptSkyWalker541Passes,     1 },
};

#define LIBRETRO_SHADER_COUNT (sizeof(g_libretroShaders) / sizeof(g_libretroShaders[0]))

// The samplers a pass reads, resolved while it is built: a libretro stage
// names the texture of an earlier pass PassPrev<N>Texture, the frame it was
// given OrigTexture, and the pass before it Texture or decal.
#define LIBRETRO_MAX_SAMPLERS 4

struct LibretroSampler
{
	GLint location;
	int   back;     // passes back to read, or -1 for the frame
};

struct LibretroPassState
{
	GLuint program;
	GLint  mvpMatrix;
	GLint  outputSize;
	GLint  textureSize;
	GLint  inputSize;
	GLint  frameCount;
	GLint  frameDirection;
	int    samplerCount;
	LibretroSampler samplers[LIBRETRO_MAX_SAMPLERS];
	UpscaleTarget target;
};

struct LibretroState
{
	LibretroPassState passes[LIBRETRO_MAX_PASSES];
};

static LibretroState g_libretroState[LIBRETRO_SHADER_COUNT];

// The row of the table a UPSCALER_* value is, or -1.
static int libretroShaderIndex(int upscaler)
{
	for (int i = 0; i < (int)LIBRETRO_SHADER_COUNT; i++)
		if (g_libretroShaders[i].value == upscaler)
			return i;

	return -1;
}

// Which pass output a sampler of pass `pass` reads, counted back from it.
static int libretroSamplerBack(const char *name)
{
	if (!strncmp(name, "PassPrev", 8))
	{
		int back = atoi(name + 8);
		return back > 0 ? back : 1;
	}

	if (!strcmp(name, "OrigTexture"))
		return -1;

	return 1;
}

// One stage of a pass: the file as it is, with the version asked for at its
// head and the stage defined after that, which is how the upstream file picks
// the stage it compiles.  The version line of the file is replaced rather than
// kept: those files are written for a compatibility context, the renderer
// makes a core one, and every construct they use - texture(), textureOffset(),
// the in and out variables, mat4x3 and transpose - is in 3.30 as well.
static bool libretroStageSource(LPCSTR resource, const char *stage, CString &source)
{
	DWORD size = 0;
	const char *text = upscalerShaderSource(resource, &size);
	if (text == NULL)
		return false;

	const char *end = text + size;

	if (end - text >= 8 && !strncmp(text, "#version", 8))
	{
		while (text < end && *text != '\n' && *text != '\r')
			text++;
	}

	source = "#version 330 core\n#define ";
	source += stage;
	source += "\n";

	return spliceShaderSource(text, (DWORD)(end - text), source, 0, stage);
}

// A pass of a chain: both stages out of the one file, with the attributes of
// the libretro API at the places the quad of the renderer keeps them.
static GLuint createLibretroProgram(LPCSTR resource, const char *name)
{
	CString vertexSource;
	CString fragmentSource;

	if (!libretroStageSource(resource, "VERTEX", vertexSource) ||
	    !libretroStageSource(resource, "FRAGMENT", fragmentSource))
		return 0;

	GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource, name);
	if (vertex == 0)
		return 0;

	GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource, name);
	if (fragment == 0)
	{
		glDeleteShader(vertex);
		return 0;
	}

	GLuint program = glCreateProgram();
	if (program == 0)
	{
		winlog("%s: cannot create the program\n", name);
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return 0;
	}

	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	// What the libretro stages read, and where the vertex array of the
	// renderer has them.  A stage that does not declare one of them ignores
	// the binding, so all three are always made.
	glBindAttribLocation(program, 0, "VertexCoord");
	glBindAttribLocation(program, 1, "TexCoord");
	glBindAttribLocation(program, 2, "COLOR");

	glLinkProgram(program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		char log[4096];
		GLsizei length = 0;
		glGetProgramInfoLog(program, sizeof(log) - 1, &length, log);
		log[length] = 0;
		winlog("%s: does not link: %s\n", name, log);
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

static void releaseLibretroShader(int index);

// Builds every pass of a chain, or none of them: a chain whose second pass did
// not compile is of no use to the menu either.
static void initLibretroShader(int index)
{
	const LibretroShader &shader = g_libretroShaders[index];
	LibretroState        &state  = g_libretroState[index];

	for (int i = 0; i < shader.passCount; i++)
	{
		LibretroPassState &pass = state.passes[i];

		pass.program = createLibretroProgram(shader.passes[i].resource, shader.name);
		if (pass.program == 0)
		{
			winlog("OpenGL 3.3: the %s upscaler is not available\n", shader.name);
			releaseLibretroShader(index);
			return;
		}

		pass.mvpMatrix      = glGetUniformLocation(pass.program, "MVPMatrix");
		pass.outputSize     = glGetUniformLocation(pass.program, "OutputSize");
		pass.textureSize    = glGetUniformLocation(pass.program, "TextureSize");
		pass.inputSize      = glGetUniformLocation(pass.program, "InputSize");
		pass.frameCount     = glGetUniformLocation(pass.program, "FrameCount");
		pass.frameDirection = glGetUniformLocation(pass.program, "FrameDirection");

		pass.samplerCount = 0;

		GLint uniformCount = 0;
		glGetProgramiv(pass.program, GL_ACTIVE_UNIFORMS, &uniformCount);

		for (GLint u = 0; u < uniformCount; u++)
		{
			char   name[64];
			GLint  size = 0;
			GLenum type = 0;

			glGetActiveUniform(pass.program, u, sizeof(name), NULL, &size, &type, name);

			if (type != GL_SAMPLER_2D || pass.samplerCount >= LIBRETRO_MAX_SAMPLERS)
				continue;

			LibretroSampler &sampler = pass.samplers[pass.samplerCount++];
			sampler.location = glGetUniformLocation(pass.program, name);
			sampler.back     = libretroSamplerBack(name);
		}
	}
}

static void releaseLibretroShader(int index)
{
	LibretroState &state = g_libretroState[index];

	for (int i = 0; i < LIBRETRO_MAX_PASSES; i++)
	{
		LibretroPassState &pass = state.passes[i];

		if (pass.program != 0)
		{
			glDeleteProgram(pass.program);
			pass.program = 0;
		}

		releaseUpscaleTarget(pass.target);
	}
}

static void initLibretroShaders()
{
	for (int i = 0; i < (int)LIBRETRO_SHADER_COUNT; i++)
		initLibretroShader(i);
}

static void releaseLibretroShaders()
{
	for (int i = 0; i < (int)LIBRETRO_SHADER_COUNT; i++)
		releaseLibretroShader(i);
}

// ---------------------------------------------------------------------------
// AMD FidelityFX Super Resolution 1.0: two passes, EASU to reconstruct the
// frame at an arbitrary ratio and RCAS to sharpen it, around the unmodified AMD
// headers.  Where the driver cannot compile the GLSL 4.20 those headers are
// written in, both passes stay out and the menu leaves FSR out too - the same
// bargain Direct3D 11 makes below feature level 11.

struct FsrState
{
	bool   available;
	GLuint easuProgram;
	GLuint rcasProgram;
	GLint  easuCon[4];    // con0 .. con3
	GLint  easuSource;
	GLint  rcasCon;
	GLint  rcasSizeInfo;
	GLint  rcasSource;
	UpscaleTarget easuTarget;
	UpscaleTarget rcasTarget;
};

static FsrState g_fsr;

// Defined below; initFsr() gives up through it when a pass did not build.
static void releaseFsr();

static void initFsr()
{
	ZeroMemory(&g_fsr, sizeof(g_fsr));

	g_fsr.easuProgram = createUpscalerProgram("FSR_GLSL_EASU", "FSR");
	g_fsr.rcasProgram = createUpscalerProgram("FSR_GLSL_RCAS", "FSR");

	if (g_fsr.easuProgram == 0 || g_fsr.rcasProgram == 0)
	{
		releaseFsr();
		return;
	}

	static const char *conNames[4] = { "con0", "con1", "con2", "con3" };
	for (int i = 0; i < 4; i++)
		g_fsr.easuCon[i] = glGetUniformLocation(g_fsr.easuProgram, conNames[i]);

	g_fsr.easuSource   = glGetUniformLocation(g_fsr.easuProgram, "fsrInput");
	g_fsr.rcasCon      = glGetUniformLocation(g_fsr.rcasProgram, "conRcas");
	g_fsr.rcasSizeInfo = glGetUniformLocation(g_fsr.rcasProgram, "sizeInfo");
	g_fsr.rcasSource   = glGetUniformLocation(g_fsr.rcasProgram, "fsrInput");

	g_fsr.available = true;
}

static void releaseFsr()
{
	if (g_fsr.easuProgram != 0)
	{
		glDeleteProgram(g_fsr.easuProgram);
		g_fsr.easuProgram = 0;
	}
	if (g_fsr.rcasProgram != 0)
	{
		glDeleteProgram(g_fsr.rcasProgram);
		g_fsr.rcasProgram = 0;
	}

	releaseUpscaleTarget(g_fsr.easuTarget);
	releaseUpscaleTarget(g_fsr.rcasTarget);

	g_fsr.available = false;
}

// ---------------------------------------------------------------------------
// The renderer itself.

// The quad: two triangles as a strip, named after the texture row each of their
// corners takes.  The window is the whole viewport, so the positions are the
// corners of normalised device coordinates.
struct QuadVertex
{
	float x, y;
	float u, v;
};

class OpenGL33Display : public IDisplay
{
private:
	// How many frame textures are rotated through, see uploadFrame().
	enum { FRAME_TEXTURE_COUNT = 2 };

	HDC    hDC;
	HWND   hwnd;
	HGLRC  hglrc;
	bool   failed;

	int    viewWidth;
	int    viewHeight;

	// Two frame textures, used in turn: uploading into the one the last frame
	// was drawn from would make the driver wait for that draw to finish first.
	// With two of them the texture written here was last read two frames ago.
	GLuint frameTextures[FRAME_TEXTURE_COUNT];
	int    frameTextureIndex;
	GLuint frameTexture; // the one this frame is uploaded into and drawn from
	int    frameWidth;
	int    frameHeight;

	GLuint quadProgram;
	GLint  quadSourceLocation;
	GLuint quadBuffer;
	GLuint quadArray;

	// The vertex array the libretro passes are drawn with: the attributes of
	// the libretro API, at the places createLibretroProgram() binds them to.
	GLuint libretroBuffer;
	GLuint libretroArray;

	GLuint overlayTexture;
	bool   overlayFullUpload; // the whole overlay has been uploaded once
	TextOverlay overlay;

	int    swapInterval;
	bool   swapTear;

	u8 *filterData;

	bool createContext();
	bool createQuad();
	void createFrameTexture(int w, int h);
	void createOverlayTexture(int w, int h);
	void releaseTextures();
	void updateFiltering();
	void uploadFrame();
	void setQuadVertices(float u0, float v0, float u1, float v1, bool flip);
	void drawStrip(bool blend);
	void drawQuad(GLuint texture, float u0, float v0, float u1, float v1, bool flip, bool blend);
	void drawOverlay();
	GLuint scaleFrameWithSinglePass(int upscaler, bool *flipped);
	GLuint scaleFrameWithLibretro(int upscaler, bool *flipped);
	GLuint scaleFrameWithFsr();

public:
	OpenGL33Display();
	virtual ~OpenGL33Display();

	virtual bool initialize();
	virtual void cleanup();
	virtual void render();
	virtual void checkFullScreen();
	virtual void renderMenu();
	virtual void clear();
	virtual bool changeRenderSize(int w, int h);
	virtual void resize(int w, int h);
	virtual DISPLAY_TYPE getType() { return OPENGL33; };
	virtual void setOption(const char *, int);
	virtual int selectFullScreenMode(GUID * *);
	virtual bool upscalerAvailable(int upscaler);
};

OpenGL33Display::OpenGL33Display()
{
	hDC            = NULL;
	hwnd           = NULL;
	hglrc          = NULL;
	failed         = false;
	viewWidth      = 0;
	viewHeight     = 0;
	for (int i = 0; i < FRAME_TEXTURE_COUNT; i++)
		frameTextures[i] = 0;
	frameTextureIndex = 0;
	frameTexture   = 0;
	frameWidth     = 0;
	frameHeight    = 0;
	quadProgram    = 0;
	quadSourceLocation = -1;
	quadBuffer     = 0;
	quadArray      = 0;
	libretroBuffer = 0;
	libretroArray  = 0;
	overlayTexture = 0;
	overlayFullUpload = true;
	swapInterval   = -1;
	swapTear       = false;
	filterData     = (u8 *)malloc(4*16*256*256); // sufficient for 4x filters @ 32bit color depth
}

OpenGL33Display::~OpenGL33Display()
{
	cleanup();
}

bool OpenGL33Display::createContext()
{
	hwnd = theApp.m_pMainWnd->GetSafeHwnd();

	// The modern entry points can only be asked for with a context current, and
	// the pixel format of a window can only be set once, so a window of our own
	// is used to look them up before the emulator's is touched.
	HWND setupWindow = createSetupWindow();
	if (setupWindow == NULL)
	{
		winlog("OpenGL 3.3: cannot create the setup window\n");
		return false;
	}

	HDC   setupDC = GetDC(setupWindow);
	HGLRC setupContext = NULL;
	bool  ok = false;

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	int format = ChoosePixelFormat(setupDC, &pfd);
	if (format != 0 && SetPixelFormat(setupDC, format, &pfd))
	{
		setupContext = wglCreateContext(setupDC);
		if (setupContext != NULL && wglMakeCurrent(setupDC, setupContext))
		{
			wglChoosePixelFormatARB    = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			wglSwapIntervalEXT         = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
			wglGetExtensionsStringEXT  = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
			ok = true;

			wglMakeCurrent(NULL, NULL);
		}
	}

	if (setupContext != NULL)
		wglDeleteContext(setupContext);
	ReleaseDC(setupWindow, setupDC);
	DestroyWindow(setupWindow);

	if (!ok || wglChoosePixelFormatARB == NULL || wglCreateContextAttribsARB == NULL)
	{
		winlog("OpenGL 3.3: the driver has no WGL_ARB_pixel_format or WGL_ARB_create_context\n");
		return false;
	}

	hDC = GetDC(hwnd);
	if (hDC == NULL)
	{
		winlog("OpenGL 3.3: cannot get the device context\n");
		return false;
	}

	const int attributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
		WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,     32,
		0
	};
	const FLOAT floatAttributes[] = { 0.0f };

	int  pixelFormat = 0;
	UINT formats     = 0;

	if (!wglChoosePixelFormatARB(hDC, attributes, floatAttributes, 1, &pixelFormat, &formats) ||
	    formats == 0 || pixelFormat == 0)
	{
		winlog("OpenGL 3.3: the driver has no accelerated 32 bit pixel format\n");
		return false;
	}

	PIXELFORMATDESCRIPTOR chosen;
	ZeroMemory(&chosen, sizeof(chosen));
	if (DescribePixelFormat(hDC, pixelFormat, sizeof(chosen), &chosen) == 0 ||
	    !SetPixelFormat(hDC, pixelFormat, &chosen))
	{
		winlog("OpenGL 3.3: cannot set the pixel format\n");
		return false;
	}

	// A 3.3 core context is the minimum the renderer needs, and the driver is
	// asked for the newest one it has above that, because a context cannot
	// compile a newer language version than itself and FSR's headers are
	// written in 4.20.  Every version from the top down is tried, the way the
	// Direct3D 11 backend walks its feature levels.
	static const int versions[][2] =
	{
		{ 4, 6 }, { 4, 5 }, { 4, 4 }, { 4, 3 }, { 4, 2 }, { 4, 1 }, { 4, 0 }, { 3, 3 }
	};

	for (int i = 0; i < (int)(sizeof(versions) / sizeof(versions[0])) && hglrc == NULL; i++)
	{
		const int contextAttributes[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, versions[i][0],
			WGL_CONTEXT_MINOR_VERSION_ARB, versions[i][1],
			WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		hglrc = wglCreateContextAttribsARB(hDC, NULL, contextAttributes);
	}

	if (hglrc == NULL)
	{
		winlog("OpenGL 3.3: the driver has no 3.3 core profile context\n");
		return false;
	}

	if (!wglMakeCurrent(hDC, hglrc))
	{
		winlog("OpenGL 3.3: cannot make the context current\n");
		return false;
	}

	if (!loadGL3Functions())
	{
		winlog("OpenGL 3.3: the driver is missing entry points the renderer needs\n");
		return false;
	}

	GLint major = 0, minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	sprintf(g_openGL33Version, "%d.%d core", (int)major, (int)minor);

	winlog("OpenGL 3.3: %s, %s, %s\n",
	       (const char *)glGetString(GL_VERSION),
	       (const char *)glGetString(GL_RENDERER),
	       (const char *)glGetString(GL_VENDOR));

	// Adaptive vertical synchronisation: where the driver has it, a frame that
	// misses the blanking interval is shown straight away instead of waiting
	// for the next one, so a frame rate that does not divide the refresh rate
	// tears once in a while instead of stuttering.
	swapTear = false;
	if (wglGetExtensionsStringEXT != NULL)
	{
		const char *extensions = wglGetExtensionsStringEXT();
		swapTear = extensions != NULL && strstr(extensions, "WGL_EXT_swap_control_tear") != NULL;
	}

	winlog("OpenGL 3.3: swap control%s\n", swapTear ? " with adaptive vsync" : "");

	// One texture unit is enough; the shaders all sample from unit 0.
	glActiveTexture(GL_TEXTURE0);

	return true;
}

bool OpenGL33Display::createQuad()
{
	quadProgram = createQuadProgram();
	if (quadProgram == 0)
		return false;

	quadSourceLocation = glGetUniformLocation(quadProgram, "frameTex");

	glGenVertexArrays(1, &quadArray);
	glGenBuffers(1, &quadBuffer);

	glBindVertexArray(quadArray);
	glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QuadVertex), NULL, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const GLvoid *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const GLvoid *)(sizeof(GLfloat) * 2));
	glBindVertexArray(0);

	// The quad the libretro passes are drawn with.  Its vertices carry the
	// attributes of the libretro API: the position of the corner, the texture
	// coordinate of it and a colour the stages are handed but do not use.  The
	// frame's first row is at the bottom of the viewport here, which puts it in
	// the first row of the target, so a pass leaves the picture the way it
	// found it.
	static const float libretroVertices[4][12] =
	{
		{ -1.0f, -1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f },
		{ -1.0f,  1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f },
		{  1.0f, -1.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f },
		{  1.0f,  1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f },
	};

	glGenVertexArrays(1, &libretroArray);
	glGenBuffers(1, &libretroBuffer);

	glBindVertexArray(libretroArray);
	glBindBuffer(GL_ARRAY_BUFFER, libretroBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(libretroVertices), libretroVertices, GL_STATIC_DRAW);

	const GLsizei libretroStride = sizeof(libretroVertices[0]);
	for (int attribute = 0; attribute < 3; attribute++)
	{
		glEnableVertexAttribArray(attribute);
		glVertexAttribPointer(attribute, 4, GL_FLOAT, GL_FALSE, libretroStride,
		                      (const GLvoid *)(sizeof(GLfloat) * 4 * attribute));
	}
	glBindVertexArray(0);

	// The upscalers, so that the menu knows which of them are there before they
	// are selected.
	initSinglePassUpscalers();
	initLibretroShaders();
	initFsr();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	return true;
}

bool OpenGL33Display::initialize()
{
	CWnd *pWnd = theApp.m_pMainWnd;

	theApp.mode320Available = false;
	theApp.mode640Available = false;
	theApp.mode800Available = false;

	viewWidth  = theApp.surfaceSizeX;
	viewHeight = theApp.surfaceSizeY;

	if (!createContext())
		return false;

	if (!createQuad())
	{
		systemMessage(0, "Failed to initialise the OpenGL 3.3 shaders");
		return false;
	}

	glViewport(0, 0, viewWidth, viewHeight);

	systemRedShift      = 3;
	systemGreenShift    = 11;
	systemBlueShift     = 19;
	systemColorDepth    = 32;
	theApp.fsColorDepth = 32;

	Init_2xSaI(32);
	if (theApp.ddrawDebug)
	{
		winlog("R shift: %d\n", systemRedShift);
		winlog("G shift: %d\n", systemGreenShift);
		winlog("B shift: %d\n", systemBlueShift);
	}

	systemUpdateColorMaps();

	theApp.updateFilter();
	theApp.updateIFB();

	// The on screen display is sized with the window, and resize() is not
	// called after a render method change - the window is recreated while no
	// display is there to hear about it - so it is made here as well.
	overlay.create(viewWidth, viewHeight);
	createOverlayTexture(viewWidth, viewHeight);

	if (failed)
		return false;

	pWnd->DragAcceptFiles(TRUE);

	return true;
}

void OpenGL33Display::cleanup()
{
	if (hglrc != NULL)
	{
		wglMakeCurrent(hDC, hglrc);

		releaseSinglePassUpscalers();
		releaseLibretroShaders();
		releaseFsr();

		if (quadProgram != 0)
		{
			glDeleteProgram(quadProgram);
			quadProgram = 0;
		}
		if (quadArray != 0)
		{
			glDeleteVertexArrays(1, &quadArray);
			quadArray = 0;
		}
		if (quadBuffer != 0)
		{
			glDeleteBuffers(1, &quadBuffer);
			quadBuffer = 0;
		}
		if (libretroArray != 0)
		{
			glDeleteVertexArrays(1, &libretroArray);
			libretroArray = 0;
		}
		if (libretroBuffer != 0)
		{
			glDeleteBuffers(1, &libretroBuffer);
			libretroBuffer = 0;
		}

		releaseTextures();

		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hglrc);
		hglrc = NULL;
	}

	if (hDC != NULL)
	{
		ReleaseDC(hwnd, hDC);
		hDC = NULL;
	}

	if (filterData)
	{
		free(filterData);
		filterData = NULL;
	}

	overlay.destroy();

	hwnd       = NULL;
	swapInterval = -1;
	viewWidth  = 0;
	viewHeight = 0;
}

void OpenGL33Display::releaseTextures()
{
	for (int i = 0; i < FRAME_TEXTURE_COUNT; i++)
	{
		if (frameTextures[i] != 0)
		{
			glDeleteTextures(1, &frameTextures[i]);
			frameTextures[i] = 0;
		}
	}
	frameTexture      = 0;
	frameTextureIndex = 0;

	if (overlayTexture != 0)
	{
		glDeleteTextures(1, &overlayTexture);
		overlayTexture = 0;
	}
	overlayFullUpload = true;

	frameWidth  = 0;
	frameHeight = 0;
}

void OpenGL33Display::createFrameTexture(int w, int h)
{
	for (int i = 0; i < FRAME_TEXTURE_COUNT; i++)
	{
		if (frameTextures[i] != 0)
		{
			glDeleteTextures(1, &frameTextures[i]);
			frameTextures[i] = 0;
		}
	}
	frameTexture      = 0;
	frameTextureIndex = 0;
	frameWidth        = 0;
	frameHeight       = 0;

	if (w <= 0 || h <= 0)
	{
		failed = true;
		return;
	}

	glGenTextures(FRAME_TEXTURE_COUNT, frameTextures);

	for (int i = 0; i < FRAME_TEXTURE_COUNT; i++)
	{
		if (frameTextures[i] == 0)
		{
			failed = true;
			return;
		}

		glBindTexture(GL_TEXTURE_2D, frameTextures[i]);

		// A frame is not a power of two and does not need to be: the texture
		// is drawn 1:1 or sampled at texel centres by the upscalers.  Clamping
		// is what the reconstruction wants at the edges.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		updateFiltering();

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	frameTexture      = frameTextures[0];
	frameTextureIndex = 0;
	frameWidth        = w;
	frameHeight       = h;
}

void OpenGL33Display::createOverlayTexture(int w, int h)
{
	if (overlayTexture != 0)
	{
		glDeleteTextures(1, &overlayTexture);
		overlayTexture = 0;
	}

	// A new texture holds nothing yet, so the first upload after this one has
	// to cover all of it rather than the band the messages are drawn in.
	overlayFullUpload = true;

	if (w <= 0 || h <= 0)
		return;

	glGenTextures(1, &overlayTexture);
	if (overlayTexture == 0)
		return;

	glBindTexture(GL_TEXTURE_2D, overlayTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	winlog("OpenGL 3.3: on screen display %dx%d\n", w, h);
}

void OpenGL33Display::updateFiltering()
{
	switch (theApp.glFilter)
	{
	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	}
}

// The window is the whole viewport.  The vertices follow the Direct3D 11
// backend's: the top of the window is the texture's first row, which is the top
// of the frame, and `flip` selects the other way around for the results of the
// upscalers that are driven by the varying.
void OpenGL33Display::setQuadVertices(float u0, float v0, float u1, float v1, bool flip)
{
	QuadVertex vertices[4];

	float top    = flip ? v1 : v0;
	float bottom = flip ? v0 : v1;

	vertices[0].x = -1.0f; vertices[0].y = -1.0f; vertices[0].u = u0; vertices[0].v = bottom;
	vertices[1].x = -1.0f; vertices[1].y =  1.0f; vertices[1].u = u0; vertices[1].v = top;
	vertices[2].x =  1.0f; vertices[2].y = -1.0f; vertices[2].u = u1; vertices[2].v = bottom;
	vertices[3].x =  1.0f; vertices[3].y =  1.0f; vertices[3].u = u1; vertices[3].v = top;

	glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

// Draws the quad as it was set up, with the program that is current and the
// texture unit 0 holds, which is how the upscalers run their own passes.
void OpenGL33Display::drawStrip(bool blend)
{
	glBindVertexArray(quadArray);

	if (blend)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

// The quad program, for the frame, an upscaler target and the overlay.
void OpenGL33Display::drawQuad(GLuint texture, float u0, float v0, float u1, float v1,
                               bool flip, bool blend)
{
	setQuadVertices(u0, v0, u1, v1, flip);

	glUseProgram(quadProgram);
	if (quadSourceLocation >= 0)
		glUniform1i(quadSourceLocation, 0);
	glBindTexture(GL_TEXTURE_2D, texture);

	drawStrip(blend);
}

// Prepares the frame the emulator just finished and uploads it: the CPU pixel
// filter first, then the on screen text, then the pixels, the same order the
// Direct3D 11 backend uses.  The first row of a frame buffer is the border row
// the filters read above the frame, so the frame itself starts one row further
// in.
void OpenGL33Display::uploadFrame()
{
	// Take the other frame texture, so the upload below does not have to wait
	// for the draw of the previous frame to be done reading the one it lands
	// in.  Everything after this - the upscalers and the quad - samples the
	// texture this frame is in.
	frameTextureIndex = (frameTextureIndex + 1) % FRAME_TEXTURE_COUNT;
	frameTexture      = frameTextures[frameTextureIndex];

	u8 *data = pix;
	int dataPitch = theApp.filterWidth * 4 + 4;

	if (textMethod != 0) // do not draw Lua HUD to a video dump
	{
		systemClonePixBuffer(osd);
		data = osd;
		systemRenderLua((u8 *)data, dataPitch);
	}

	u8 *image = data + dataPitch;
	int imagePitch = dataPitch;

	int rectWidth  = theApp.rect.right - theApp.rect.left;
	int rectHeight = theApp.rect.bottom - theApp.rect.top;

	if (theApp.filterFunction)
	{
		theApp.filterFunction(data + dataPitch,
		                      dataPitch,
		                      (u8 *)theApp.delta,
		                      filterData,
		                      rectWidth * 4,
		                      theApp.filterWidth,
		                      theApp.filterHeight);

		image      = filterData;
		imagePitch = rectWidth * 4;
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
			drawTextTransp(image,
			               imagePitch,
			               theApp.rect.left + 10,
			               theApp.rect.bottom - 10,
			               buffer);
		else
			drawText(image,
			         imagePitch,
			         theApp.rect.left + 10,
			         theApp.rect.bottom - 10,
			         buffer);
	}

	if (textMethod == 1)
		DrawTextMessages(image, imagePitch, theApp.rect.left, theApp.rect.bottom);

	glBindTexture(GL_TEXTURE_2D, frameTexture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, imagePitch / 4);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
	                theApp.rect.left, theApp.rect.top, rectWidth, rectHeight,
	                GL_RGBA, GL_UNSIGNED_BYTE, image);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

// The single pass upscalers map the frame texture onto the whole window the same
// way the textured quad would, so the result covers the window and the frame
// stacks with whatever the CPU filter made of it.
//
// Their shader is driven by the varying the quad vertex shader hands it, and its
// taps are laid out in the frame the way the Direct3D 11 shader lays them out,
// so the frame's first row ends up at the top of the viewport - which in a
// framebuffer is the last row of the target texture.  The pass therefore stores
// the result bottom first and the caller draws it flipped.
GLuint OpenGL33Display::scaleFrameWithSinglePass(int upscaler, bool *flipped)
{
	int index = singlePassUpscalerIndex(upscaler);
	if (index < 0)
		return 0;

	SinglePassUpscalerState &state = g_singlePassUpscalerState[index];

	if (state.program == 0 || frameTexture == 0)
		return 0;

	if (theApp.rect.right != frameWidth || theApp.rect.bottom != frameHeight)
		return 0;

	// The window, unless the user capped the ratio the upscaler may reach; the
	// quad then stretches what it made over the rest of the window.
	int targetWidth, targetHeight;
	upscalerTargetSize(frameWidth, frameHeight, viewWidth, viewHeight,
	                   theApp.upscalerMaxScale, &targetWidth, &targetHeight);

	if (!updateUpscaleTarget(state.target, targetWidth, targetHeight, g_singlePassUpscalers[index].name))
		return 0;

	glBindFramebuffer(GL_FRAMEBUFFER, state.target.framebuffer);
	glViewport(0, 0, state.target.width, state.target.height);

	glUseProgram(state.program);
	glUniform4f(state.sourceSizeLocation, (float)frameWidth, (float)frameHeight,
	            1.0f / (float)frameWidth, 1.0f / (float)frameHeight);
	glUniform4f(state.outputSizeLocation, (float)targetWidth, (float)targetHeight,
	            1.0f / (float)targetWidth, 1.0f / (float)targetHeight);
	glUniform4f(state.scaleLocation, (float)targetWidth / (float)frameWidth,
	            (float)targetHeight / (float)frameHeight, 0.0f, 0.0f);
	if (state.sourceLocation >= 0)
		glUniform1i(state.sourceLocation, 0);

	// The taps are taken at texel centres, so the sampler has to be the point
	// one; the frame's own filtering is put back below.
	glBindTexture(GL_TEXTURE_2D, frameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	setQuadVertices((float)theApp.rect.left / (float)frameWidth, (float)theApp.rect.top / (float)frameHeight,
	                (float)theApp.rect.right / (float)frameWidth, (float)theApp.rect.bottom / (float)frameHeight,
	                false);
	drawStrip(false);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, frameTexture);
	updateFiltering();

	*flipped = true;
	return state.target.texture;
}

// A chain runs its passes one after the other and hands back what the last one
// made.  Every pass maps the whole of its input onto the whole of its target,
// so it leaves the picture the way it found it, and the result covers the
// window the way the frame would - the caller draws it like the frame.
GLuint OpenGL33Display::scaleFrameWithLibretro(int upscaler, bool *flipped)
{
	int index = libretroShaderIndex(upscaler);
	if (index < 0)
		return 0;

	const LibretroShader &shader = g_libretroShaders[index];
	LibretroState        &state  = g_libretroState[index];

	if (shader.passCount <= 0 || state.passes[0].program == 0 || frameTexture == 0)
		return 0;

	if (theApp.rect.right != frameWidth || theApp.rect.bottom != frameHeight)
		return 0;

	// The window, unless the user capped the ratio the upscaler may reach; the
	// quad then stretches what it made over the rest of the window.
	int targetWidth, targetHeight;
	upscalerTargetSize(frameWidth, frameHeight, viewWidth, viewHeight,
	                   theApp.upscalerMaxScale, &targetWidth, &targetHeight);

	GLuint outputs[LIBRETRO_MAX_PASSES];
	int    outputWidths[LIBRETRO_MAX_PASSES];
	int    outputHeights[LIBRETRO_MAX_PASSES];

	for (int i = 0; i < shader.passCount; i++)
	{
		// A fixed scale is a multiple of the frame rather than of the pass
		// before it, which is what the presets ask for: the metrics passes of
		// ScaleFX and xBR-mlv4 work on the frame itself.
		int width  = shader.passes[i].scale > 0 ? frameWidth  * shader.passes[i].scale : targetWidth;
		int height = shader.passes[i].scale > 0 ? frameHeight * shader.passes[i].scale : targetHeight;

		if (!updateUpscaleTarget(state.passes[i].target, width, height, shader.name))
			return 0;

		outputs[i]       = state.passes[i].target.texture;
		outputWidths[i]  = state.passes[i].target.width;
		outputHeights[i] = state.passes[i].target.height;
	}

	static const GLfloat identity[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	int    inputWidth  = frameWidth;
	int    inputHeight = frameHeight;

	for (int i = 0; i < shader.passCount; i++)
	{
		LibretroPassState &pass = state.passes[i];

		glBindFramebuffer(GL_FRAMEBUFFER, pass.target.framebuffer);
		glViewport(0, 0, pass.target.width, pass.target.height);

		glUseProgram(pass.program);

		// What the libretro API hands a stage.  The quad is already in the
		// clip space the matrix would produce, so it is the identity; the two
		// sizes are those of this pass and of what it reads.
		if (pass.mvpMatrix >= 0)
			glUniformMatrix4fv(pass.mvpMatrix, 1, GL_FALSE, identity);

		if (pass.outputSize >= 0)
			glUniform2f(pass.outputSize, (float)pass.target.width, (float)pass.target.height);

		if (pass.textureSize >= 0)
			glUniform2f(pass.textureSize, (float)inputWidth, (float)inputHeight);

		if (pass.inputSize >= 0)
			glUniform2f(pass.inputSize, (float)inputWidth, (float)inputHeight);

		if (pass.frameCount >= 0)
			glUniform1i(pass.frameCount, 0);

		if (pass.frameDirection >= 0)
			glUniform1i(pass.frameDirection, 1);

		// The textures the pass reads.  A libretro stage counts the passes
		// back from its own: PassPrev2Texture is the one before the one it
		// reads as Texture, and anything before the first pass is the frame.
		GLint filter = shader.passes[i].linear ? GL_LINEAR : GL_NEAREST;

		for (int s = 0; s < pass.samplerCount; s++)
		{
			const LibretroSampler &sampler = pass.samplers[s];
			int back = sampler.back;

			GLuint texture = frameTexture;
			if (back > 0 && i - back >= 0)
				texture = outputs[i - back];

			glActiveTexture(GL_TEXTURE0 + s);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

			if (sampler.location >= 0)
				glUniform1i(sampler.location, s);
		}

		glBindVertexArray(libretroArray);
		glDisable(GL_BLEND);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		inputWidth  = pass.target.width;
		inputHeight = pass.target.height;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// The frame keeps the filtering the renderer gave it, which the passes
	// above set to what their taps need.
	glBindTexture(GL_TEXTURE_2D, frameTexture);
	updateFiltering();

	// The last target holds the frame's first row first, like the frame does.
	*flipped = false;
	return state.passes[shader.passCount - 1].target.texture;
}

// FSR reconstructs and sharpens the frame in two passes of its own.
//
// Its shaders are driven by the fragment position instead of by the varying:
// EASU rebuilds output pixel row 0 from input row 0, wherever that lands in the
// framebuffer, so the target comes out with the frame's first row first, like
// the frame texture itself, and the caller draws it the same way it draws the
// frame.
GLuint OpenGL33Display::scaleFrameWithFsr()
{
	if (!g_fsr.available || frameTexture == 0)
		return 0;

	if (theApp.rect.right != frameWidth || theApp.rect.bottom != frameHeight)
		return 0;

	// The window, unless the user capped the ratio the upscaler may reach; the
	// quad then stretches what it made over the rest of the window.
	int targetWidth, targetHeight;
	upscalerTargetSize(frameWidth, frameHeight, viewWidth, viewHeight,
	                   theApp.upscalerMaxScale, &targetWidth, &targetHeight);

	if (!updateUpscaleTarget(g_fsr.easuTarget, targetWidth, targetHeight, "FSR") ||
	    !updateUpscaleTarget(g_fsr.rcasTarget, targetWidth, targetHeight, "FSR"))
		return 0;

	// The constants the AMD headers want, in the block the Direct3D 11 backend
	// fills with the same function.  The shaders read the EASU and RCAS values
	// as the bits of the floats they are, and the size as integers.
	float constants[FSR_CONSTANT_FLOATS];
	fsrWriteConstants(constants, (float)frameWidth, (float)frameHeight,
	                  (float)targetWidth, (float)targetHeight, theApp.upscalerSharpness);

	const GLuint *bits = (const GLuint *)constants;

	// EASU: reconstruct the frame into the first target.
	glBindFramebuffer(GL_FRAMEBUFFER, g_fsr.easuTarget.framebuffer);
	glViewport(0, 0, g_fsr.easuTarget.width, g_fsr.easuTarget.height);

	glUseProgram(g_fsr.easuProgram);
	for (int i = 0; i < 4; i++)
		if (g_fsr.easuCon[i] >= 0)
			glUniform4uiv(g_fsr.easuCon[i], 1, bits + i * 4);
	if (g_fsr.easuSource >= 0)
		glUniform1i(g_fsr.easuSource, 0);

	// The frame is only ever read through textureGather(), which takes its taps
	// itself, so its filtering does not come into it.
	glBindTexture(GL_TEXTURE_2D, frameTexture);

	setQuadVertices((float)theApp.rect.left / (float)frameWidth, (float)theApp.rect.top / (float)frameHeight,
	                (float)theApp.rect.right / (float)frameWidth, (float)theApp.rect.bottom / (float)frameHeight,
	                false);
	drawStrip(false);

	// RCAS: sharpen what EASU made.
	glBindFramebuffer(GL_FRAMEBUFFER, g_fsr.rcasTarget.framebuffer);
	glViewport(0, 0, g_fsr.rcasTarget.width, g_fsr.rcasTarget.height);

	glUseProgram(g_fsr.rcasProgram);
	if (g_fsr.rcasCon >= 0)
		glUniform4uiv(g_fsr.rcasCon, 1, bits + 16);
	if (g_fsr.rcasSizeInfo >= 0)
		glUniform4uiv(g_fsr.rcasSizeInfo, 1, bits + 20);
	if (g_fsr.rcasSource >= 0)
		glUniform1i(g_fsr.rcasSource, 0);

	glBindTexture(GL_TEXTURE_2D, g_fsr.easuTarget.texture);
	setQuadVertices(0.0f, 0.0f, 1.0f, 1.0f, false);
	drawStrip(false);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return g_fsr.rcasTarget.texture;
}

void OpenGL33Display::drawOverlay()
{
	if (overlayTexture == 0 || overlay.empty())
	{
		// The overlay is made when the renderer starts and whenever the window
		// changes size, which covers every path the emulator takes; this is the
		// last resort for one that recreated the window without telling us.
		if (viewWidth <= 0 || viewHeight <= 0)
			return;

		overlay.create(viewWidth, viewHeight);
		createOverlayTexture(viewWidth, viewHeight);

		if (overlayTexture == 0 || overlay.empty())
			return;
	}

	// Redraws the pending message slots and says whether there is anything to
	// show at all.
	if (!overlay.update())
		return;

	// The overlay is the size of the window - 8 MB a frame at 1080p - and it
	// only changes when a message does, so it is uploaded again only then.  The
	// band the slots are drawn in is all that can have changed; the rest of the
	// texture was left transparent by the first upload after it was created.
	if (overlay.changed())
	{
		int top    = overlayFullUpload ? 0 : overlay.usedTop();
		int height = overlayFullUpload ? overlay.height() : overlay.usedHeight();

		glBindTexture(GL_TEXTURE_2D, overlayTexture);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, overlay.pitch() / 4);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, top, overlay.width(), height,
		                GL_BGRA, GL_UNSIGNED_BYTE,
		                overlay.pixels() + (size_t)top * (size_t)overlay.pitch());
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		overlayFullUpload = false;
	}

	drawQuad(overlayTexture, 0.0f, 0.0f, 1.0f, 1.0f, false, true);
}

void OpenGL33Display::render()
{
	if (hglrc == NULL)
		return;

	if (frameTexture == 0 || frameWidth == 0 || frameHeight == 0)
		return;

	uploadFrame();

	GLuint source = frameTexture;
	bool   flip   = false;

	// The window's top is the frame's first row, and the frame occupies the
	// part of the texture the emulator last drew into.
	float u0 = (float)theApp.rect.left / (float)frameWidth;
	float v0 = (float)theApp.rect.top / (float)frameHeight;
	float u1 = (float)theApp.rect.right / (float)frameWidth;
	float v1 = (float)theApp.rect.bottom / (float)frameHeight;

	if (theApp.upscaler == UPSCALER_FSR)
	{
		GLuint scaled = scaleFrameWithFsr();
		if (scaled != 0)
			source = scaled;
	}
	else if (theApp.upscaler != UPSCALER_OFF)
	{
		GLuint scaled = scaleFrameWithSinglePass(theApp.upscaler, &flip);
		if (scaled == 0)
			scaled = scaleFrameWithLibretro(theApp.upscaler, &flip);
		if (scaled != 0)
			source = scaled;
	}

	glViewport(0, 0, viewWidth, viewHeight);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (source == frameTexture)
		drawQuad(source, u0, v0, u1, v1, flip, false);
	else
		drawQuad(source, 0.0f, 0.0f, 1.0f, 1.0f, flip, false);

	if (textMethod == 2)
		drawOverlay();

	if (wglSwapIntervalEXT != NULL)
	{
		// Only wait for the vertical blank when the user asked for it and the
		// emulator is not running ahead of the frame rate, the same condition
		// the Direct3D 11 backend presents with.  Where the driver has adaptive
		// synchronisation it is asked for: a frame that is late is then shown
		// at once instead of being held back a whole interval, which turns the
		// stutter of a 59.73 Hz emulator on a 60 Hz display into a tear.
		bool fastForward = speedup || theApp.frameSearchSkipping;
		int interval = 0;
		if (theApp.vsync && !fastForward)
			interval = swapTear ? -1 : 1;

		if (interval != swapInterval)
		{
			wglSwapIntervalEXT(interval);
			swapInterval = interval;
		}
	}

	SwapBuffers(hDC);
}

void OpenGL33Display::clear()
{
        if (hglrc == NULL || hDC == NULL)
                return;

        // The window keeps showing the last swapped buffer until the next
        // swap, so clearing alone does not remove the picture of a closed
        // ROM: the swap is what changes what is on screen.
        wglMakeCurrent(hDC, hglrc);

        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SwapBuffers(hDC);
}

void OpenGL33Display::renderMenu()
{
	checkFullScreen();
	if (theApp.m_pMainWnd)
		theApp.m_pMainWnd->DrawMenuBar();
}

void OpenGL33Display::checkFullScreen()
{
}

bool OpenGL33Display::changeRenderSize(int w, int h)
{
	if (frameWidth != w || frameHeight != h)
	{
		createFrameTexture(w, h);
		if (frameTexture == 0)
			return false;
	}
	return true;
}

void OpenGL33Display::resize(int w, int h)
{
	viewWidth  = w;
	viewHeight = h;

	if (hglrc == NULL)
		return;

	glViewport(0, 0, w, h);

	overlay.destroy();
	overlay.create(w, h);
	createOverlayTexture(w, h);

	// The targets of the upscalers follow the window.
	for (int i = 0; i < (int)SINGLE_PASS_UPSCALER_COUNT; i++)
		releaseUpscaleTarget(g_singlePassUpscalerState[i].target);

	for (int i = 0; i < (int)LIBRETRO_SHADER_COUNT; i++)
		for (int pass = 0; pass < LIBRETRO_MAX_PASSES; pass++)
			releaseUpscaleTarget(g_libretroState[i].passes[pass].target);

	releaseUpscaleTarget(g_fsr.easuTarget);
	releaseUpscaleTarget(g_fsr.rcasTarget);
}

void OpenGL33Display::setOption(const char *option, int value)
{
	if (!strcmp(option, "glFilter"))
	{
		// Every frame texture carries its own sampler state.
		for (int i = 0; i < FRAME_TEXTURE_COUNT; i++)
		{
			if (frameTextures[i] == 0)
				continue;

			glBindTexture(GL_TEXTURE_2D, frameTextures[i]);
			updateFiltering();
		}

		// The upscaler targets too: when the scaling factor is capped they are
		// stretched over the window by the sampler instead of drawn 1:1.
		for (int i = 0; i < (int)SINGLE_PASS_UPSCALER_COUNT; i++)
		{
			if (g_singlePassUpscalerState[i].target.texture == 0)
				continue;

			glBindTexture(GL_TEXTURE_2D, g_singlePassUpscalerState[i].target.texture);
			updateFiltering();
		}

		// The last target of a chain is the one the quad draws.  The targets
		// before it are read by the passes, which set their own filtering.
		for (int i = 0; i < (int)LIBRETRO_SHADER_COUNT; i++)
		{
			int last = g_libretroShaders[i].passCount - 1;
			if (last < 0 || g_libretroState[i].passes[last].target.texture == 0)
				continue;

			glBindTexture(GL_TEXTURE_2D, g_libretroState[i].passes[last].target.texture);
			updateFiltering();
		}

		if (g_fsr.rcasTarget.texture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, g_fsr.rcasTarget.texture);
			updateFiltering();
		}
	}

	// The upscaler and its sharpening are read from theApp on every frame, and
	// glType picks between the quads and triangles of the fixed function
	// renderer, which a quad strip covers both of.
}

int OpenGL33Display::selectFullScreenMode(GUID * *)
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

// Which of the upscalers the renderer can run: the single pass shaders were
// built while it started up and FSR only where its headers compiled, so the
// menu leaves out what is not there.  NVIDIA Image Scaling is a compute shader
// and stays with Direct3D 11.
bool OpenGL33Display::upscalerAvailable(int upscaler)
{
	switch (upscaler)
	{
	case UPSCALER_OFF:
		return true;
	case UPSCALER_FSR:
		return g_fsr.available;
	default:
		int index = singlePassUpscalerIndex(upscaler);
		if (index >= 0)
			return g_singlePassUpscalerState[index].program != 0;

		index = libretroShaderIndex(upscaler);
		return index >= 0 && g_libretroState[index].passes[0].program != 0;
	}
}

IDisplay *newOpenGL33Display()
{
	return new OpenGL33Display();
}

const char *openGL33Version()
{
	return g_openGL33Version;
}
