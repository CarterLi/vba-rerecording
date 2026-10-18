#ifndef VBA_WIN32_DISASSEMBLE_H
#define VBA_WIN32_DISASSEMBLE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum DISPLAY_TYPE
{
	GDI         = 0,
	DIRECT_DRAW = 1,
	DIRECT_3D   = 2,
	OPENGL      = 3,
	DIRECT_3D11 = 4,
	OPENGL33    = 5
};

class IDisplay
{
public:
	IDisplay() {};
	virtual ~IDisplay() {};

	virtual bool initialize() = 0;
	virtual void cleanup()    = 0;
	virtual void render()     = 0;
	virtual void checkFullScreen() { };
	virtual void renderMenu() { };
	virtual void clear() = 0;
	virtual bool changeRenderSize(int w, int h) { return true; };
	virtual void resize(int w, int h) {};
	virtual void setOption(const char *option, int value) = 0;
	virtual DISPLAY_TYPE getType() = 0;
	virtual int selectFullScreenMode(GUID * *) = 0;

	// Whether this backend can run the upscaler whose UPSCALER_* value is
	// given, which is the question the upscaling menu greys its items out
	// with.  False here: the sampler is the only thing that scales the
	// frame unless a backend says otherwise.
	virtual bool upscalerAvailable(int upscaler) { return false; };
};

#endif // VBA_WIN32_DISASSEMBLE_H
