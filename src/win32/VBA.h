#if !defined(AFX_VBA_H__57514A10_49F9_4B83_A928_0D8A4A7306A3__INCLUDED_)
#define AFX_VBA_H__57514A10_49F9_4B83_A928_0D8A4A7306A3__INCLUDED_

#pragma once

/*
   #ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
   #endif
 */

#include <afxtempl.h>

#include "AcceleratorManager.h"
#include "AVIWrite.h"
#include "Display.h"
#include "DDrawConfig.h"
#include "GdiConfig.h"
#include "OpenGLConfig.h"
#include "WgpuConfig.h"
#include "../common/System.h"
#include "../common/CoreOptions.h"
#include "../filters/lcdfilter.h"

/////////////////////////////////////////////////////////////////////////////
// VBA:
// See VBA.cpp for the implementation of this class
//

enum
{
	VIDEO_1X, VIDEO_2X, VIDEO_3X, VIDEO_4X,
	VIDEO_320x240, VIDEO_640x480, VIDEO_800x600, VIDEO_OTHER,
	// Appended so the values stored in the registry by older builds keep
	// meaning the same thing.
	VIDEO_5X, VIDEO_6X, VIDEO_7X, VIDEO_8X,
	VIDEO_9X, VIDEO_10X, VIDEO_11X, VIDEO_12X
};

// Sound backend selection (theApp.soundApi).  XAudio2 is optional: when
// XAudio2_9.dll or the audio device is unavailable, systemSoundInit() falls
// back to DirectSound.
enum
{
	SOUND_API_DIRECTSOUND,
	SOUND_API_XAUDIO2
};

// Frame upscaling (theApp.upscaler): the frame is either drawn by the sampler
// or rebuilt by one of the upscalers that ship with the renderers.  The two SDK
// upscalers then apply theApp.upscalerSharpness percent of sharpening, the
// pixel filter shaders do not.  The other backends ignore all of it.
//
// The values are in the order of the upscaling menu, which MainWndOptions.cpp
// works out from the menu ids (see resource.h), so a new upscaler is a value
// here, an id, a menu item and a row of the table its renderer keeps: the
// single pass one in Direct3D11.cpp, or one of the two chains below it and in
// OpenGL3.cpp for the libretro shaders, which the two backends do not run the
// same way round.
enum
{
	UPSCALER_OFF,   // sampler filtering only
	UPSCALER_NIS,   // NVIDIA Image Scaling
	UPSCALER_FSR,   // AMD FidelityFX Super Resolution 1.0 (EASU + RCAS)
	// xBRZ as a pixel filter shader of the same backend.  Unlike the two SDKs
	// above it does not reconstruct the frame but filters it, at whatever size
	// the emulation thread produced it, the way a CPU pixel filter would.
	UPSCALER_XBRZ,
	// The libretro shaders of src/win32/upscalers/libretro, which filter the
	// frame rather than reconstruct it, in the order of the folders they come
	// from: the xbr folder, then scalefx, then the handheld one.  Lanczos, the
	// resampler of the windowed folder, is last because the values above it
	// are what the settings file of an older build holds.
	UPSCALER_XBR_LV2,
	UPSCALER_XBR_LV2_NOBLEND,
	UPSCALER_XBR_LV3,
	UPSCALER_XBR_LV3_MULTIPASS,
	UPSCALER_XBR_2X,
	UPSCALER_XBR_MLV4,
	UPSCALER_SUPER_XBR,
	UPSCALER_SCALEFX,
	UPSCALER_PT_SKYWALKER,
	// Windowed Lanczos, three taps per axis: the smooth enlargement of the
	// menu, where the shaders above it rebuild edges of pixel art.
	UPSCALER_LANCZOS3,
	// The last of them, so that the settings file is checked against the end
	// of the list rather than against a name that the next addition leaves
	// behind.
	UPSCALER_LAST = UPSCALER_LANCZOS3
};

// True for the fixed-size / custom full screen modes (as opposed to the
// integer window scale factors).
inline bool isFullscreenOption(int option)
{
	return option >= VIDEO_320x240 && option <= VIDEO_OTHER;
}

// The size an upscaler renders at.  That is the whole window, unless the user
// capped the scaling factor: the upscalers filter and reconstruct at the ratio
// they are given, so past a few times the frame size the extra resolution only
// makes them smear, and the sampler can stretch what they made the rest of the
// way.  A cap of 0 means no cap.
inline void upscalerTargetSize(int frameWidth, int frameHeight,
                               int viewWidth, int viewHeight, int maxScale,
                               int *width, int *height)
{
	*width  = viewWidth;
	*height = viewHeight;

	if (maxScale > 0)
	{
		if (*width > frameWidth * maxScale)
			*width = frameWidth * maxScale;
		if (*height > frameHeight * maxScale)
			*height = frameHeight * maxScale;
	}
}

#define REWIND_SIZE 400000
#define MAX_REWIND_SLOTS 256
#define SCREEN_MESSAGE_SLOTS 8

/////////////////////////////////////////////////////////////////////////////
// forward decl
class IUpdateListener;
class Input;
class ISound;
class AVIWrite;
class WavWriter;

class VBA : public CWinApp
{
public:
	CMenu	  m_menu;
	HMENU	  menu;
	HMENU	  popup;
	bool	  mode320Available;
	bool	  mode640Available;
	bool	  mode800Available;
	int		  windowPositionX;
	int		  windowPositionY;
	void	  (*filterFunction)(u8 *, u32, u8 *, u8 *, u32, int, int);
	void	  (*ifbFunction)(u8 *, u32, int, int);
	int		  ifbType;
	int		  filterType;
	int		  filterWidth;
	int		  filterHeight;
	int		  lcdFilterGba;      // LCD color filter variant for GBA (-1 = off)
	int		  lcdFilterGbc;      // LCD color filter variant for GB/GBC (-1 = off)
	int		  lcdFilterColorMode;
	int		  fsWidth;
	int		  fsHeight;
	int		  fsColorDepth;
	bool	  fsForceChange;
	bool	  AsscWithSaveState;
	int		  sizeX;
	int		  sizeY;
	int		  surfaceSizeX;
	int		  surfaceSizeY;
	double	  scale;
	int		  videoOption;
	bool	  fullScreenStretch;
	bool	  disableStatusMessage;
	int		  showSpeed;
	BOOL	  showSpeedTransparent;
	int		  showRenderedFrames;
	bool	  screenMessage [SCREEN_MESSAGE_SLOTS];
	CString	  screenMessageBuffer [SCREEN_MESSAGE_SLOTS];
	DWORD	  screenMessageTime [SCREEN_MESSAGE_SLOTS];
	int		  screenMessageDuration [SCREEN_MESSAGE_SLOTS];
	CString	  screenMessageColorBuffer [SCREEN_MESSAGE_SLOTS];
	u8 *	  delta[257 * 244 * 4];
	bool	  menuToggle;
	IDisplay *display;
	bool	  soundInitialized;
	// ---- emulator options owned by the core (common/CoreOptions.h) --------
	// Bound by reference, so every `theApp.<name>` elsewhere in the frontend
	// keeps compiling and behaving exactly as before, while the core reads and
	// writes the same storage without knowing that MFC exists.
	bool &	  useBiosFile		  = g_core.useBiosFile;
	bool &	  skipBiosIntro		  = g_core.skipBiosIntro;
	CString	  biosFileName;
	bool	  allowLeftRight;
	bool	  autofireAccountForLag;
	bool	  nextframeAccountForLag;
	bool	  active;
	bool	  iconic;
	bool	  paused;
	CString	  recentFiles[10];
	bool	  recentFreeze;
	bool	  autoSaveLoadCheatList;
	bool	  pauseDuringCheatSearch;
	bool	  modelessCheatDialogIsOpen;
//	FILE *    winout;
//	bool      removeIntros;
	bool  autoIPS;
	int &	  winGbBorderOn		  = g_core.winGbBorderOn;
	bool &  hideMovieBorder		  = g_core.hideMovieBorder;
	int &	  winFlashSize		  = g_core.winFlashSize;
	bool  winRtcEnable;
	int &	  winSaveType		  = g_core.winSaveType;
	char *rewindMemory;
	int	  rewindPos;
	int	  rewindTopPos;
	int	  rewindCounter;
	int	  rewindCount;
	bool  rewindSaveNeeded;
	int	  rewindTimer;
	int	  rewindSlots;
	bool  tripleBuffering;
	bool  autoHideMenu;
	bool  speedupToggle;
	int	  throttle;
	u32	  throttleLastTime;
///  u32 autoFrameSkipLastTime;
///  bool autoFrameSkip;
	bool		 accuratePitchThrottle;
	bool		 vsync;
	bool		 changingVideoSize;
	GUID		 videoDriverGUID;
	GUID *		 pVideoDriverGUID;
	DISPLAY_TYPE renderMethod;
	bool		 ddrawEmulationOnly;
	bool		 ddrawUsingEmulationOnly;
	bool		 ddrawDebug;
	bool		 ddrawUseVideoMemory;
	int			 d3dFilter;
	int			 upscaler;          // UPSCALER_OFF / UPSCALER_NIS / UPSCALER_FSR /
	                                // UPSCALER_XBRZ, see the enum above
	int			 upscalerSharpness; // upscaler sharpening in percent, 0..100
	int			 upscalerMaxScale;  // largest ratio the upscalers render at, 0 for
	                                // no limit, see upscalerTargetSize()
	int			 glFilter;
	int			 glType;
#if VBA_ENABLE_WGPU
	int			 wgpuFilter;
	int			 wgpuBackend;       // VBA_WGPU_BACKEND_*, see WgpuConfig.h
#endif
	bool		 muteWhenInactive;
	bool		 muteFrameAdvance;
	bool		 pauseWhenInactive;
	bool		 enableBackgroundInput;
	bool		 alwaysOnTop;
	bool		 useModernFont;		// system UI font instead of the bitmap one
	bool		 useOldSync;
	bool		 winGbPrinterEnabled;
	int			 threadPriority;
	int			 languageOption;
	CString		 languageName;
	HINSTANCE	 languageModule;
	int			 renderedFrames;
	Input *		 input;
	int			 joypadDefault;
	int &		 autoFire			= g_core.autoFire;
	int &		 autoFire2			= g_core.autoFire2;
	int &		 autoHold			= g_core.autoHold;
	bool		 autoFireToggle;
	bool &		 frameCounter		= g_core.frameCounter;
	bool &		 lagCounter			= g_core.lagCounter;
	bool &		 extraCounter		= g_core.extraCounter;
	bool &		 inputDisplay		= g_core.inputDisplay;
	bool &		 nextInputDisplay	= g_core.nextInputDisplay;
	bool		 movieReadOnly;
	bool		 movieEditMode;
	bool &		 movieOnEndPause		= g_core.movieOnEndPause;
	int &		 movieOnEndBehavior	= g_core.movieOnEndBehavior;
	bool		 soundRecording;
	WavWriter *	 soundRecorder;
	CString		 soundRecordName;
	ISound *	 sound;
	int			 soundApi;  // SOUND_API_DIRECTSOUND or SOUND_API_XAUDIO2
	bool		 aviRecording;
	AVIWrite *	 aviRecorder;
	CString		 aviRecordName;
	bool		 altAviRecordMethod;
	bool		 captureScaled;  // record AVI/screenshots at the window size, with filters
	bool		 nvVideoLog;
	bool		 nvAudioLog;
	bool		 painting;  // for systemDrawScreen()
	int			 mouseCounter;
	bool		 winMuteForNow;
	bool		 winPauseNextFrame;
	bool		 wasPaused;
	int			 fsMaxScale;
	int			 romSize;
	bool		 autoLoadMostRecent;
	bool		 loadMakesRecent;
	bool		 loadMakesCurrent;
	bool		 saveMakesCurrent;
	int			 currentSlot;
	bool		 showSlotTime;
	int			 filenamePreference;
	int			 LuaFastForward;
	bool &		 frameSearching			= g_core.frameSearching;
	bool &		 frameSearchSkipping	= g_core.frameSearchSkipping;
	bool		 frameSearchFirstStep;
	bool		 frameSearchLoadValid;
	int			 frameSearchLength;
	int			 frameSearchStart;
	u32			 frameSearchOldInput[4];
	char *		 frameSearchMemory;
	DWORD		 wmTimerRes;

	CList<IUpdateListener *, IUpdateListener * &> updateList;
	int updateCount;

	CAcceleratorManager winAccelMgr;
	HACCEL hAccel;

	RECT rect;
	RECT dest;

	struct EmulatedSystem &emulator;

	CString romFilename;
	CString gameFilename;
	CString exeName;
	CString exeDir;
	CString wndClass;

public:
	VBA();
	~VBA();

	void adjustDestRect();
	void recreateMenuBar();
	void updateIFB();
	void updateFilter();
	void updateMenuBar();
	void winAddUpdateListener(IUpdateListener *l);
	void winRemoveUpdateListener(IUpdateListener *l);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(VBA)
public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

	// Implementation
public:
	HINSTANCE winLoadLanguage(const char *name);
	void winSetLanguageOption(int option, bool force);
	void updatePriority();
	void shutdownDisplay();
	void winCheckFullscreen();
	bool updateRenderMethod(bool force);
	bool initDisplay();
	void updateWindowSize(int value);
	void updateVideoSize(UINT id);
	void updateFrameSkip();
	void loadSettings();
	void saveSettings();
	bool initInput();
	void addRecentFile(const CString &file);
	void saveRewindStateIfNecessary();
	//{{AFX_MSG(VBA)
	afx_msg void OnAppAbout();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern VBA theApp;

extern void DrawTextMessages(u8 *dest, int pitch, int left, int bottom);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VBA_H__57514A10_49F9_4B83_A928_0D8A4A7306A3__INCLUDED_)
