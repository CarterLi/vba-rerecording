// XAudio2Sound.cpp: XAudio2 (2.9) sound backend.
//
// XAudio2_9.dll is loaded at run time and the entry point is resolved with
// GetProcAddress, so the executable has no import dependency on it.  On any
// failure (no DLL, no audio device, no voices) init() returns false and
// systemSoundInit() falls back to DirectSound.
//
// The DLL is loaded and the failures are reported exactly the way the
// DirectSound backend does it: the name of what could not be resolved goes to
// directXMessage(), a failing call reports its HRESULT.
//
// The emulator hands us one block of soundBufferLen bytes per emulated frame.
// Each block is copied into a round-robin pool of buffers and submitted to a
// source voice; a free slot is waited for exactly where the DirectSound
// backend waited for its play cursor.

#include "stdafx.h"

// The SDK xaudio2.h refuses to compile below Windows 8 while this project
// still declares an XP compatible target.  Raise the version for this header
// only (it merely picks a DLL name and a GUID we do not use) and restore it
// afterwards.  The DLL itself is only loaded on the systems that have it.
#define VBA_XAUDIO2_SAVED_WIN32_WINNT _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#include <xaudio2.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT VBA_XAUDIO2_SAVED_WIN32_WINNT

#include "resource.h"
#include "AVIWrite.h"
#include "Sound.h"
#include "WavWriter.h"
#include "VBA.h"

#include "../common/SystemGlobals.h"
#include "../common/nesvideos-piece.h"

// Number of blocks the voice may hold at once.  This is the ring size the
// DirectSound backend uses (soundBufferTotalLen / soundBufferLen).
#define XAUDIO2_SOUND_BUFFERS 10

typedef HRESULT (WINAPI *XAudio2CreateFn)(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR);

extern void directXMessage(const char *);

// Counters shared with the DirectSound backend (defined there) that pace the
// emulator against the audio clock.
extern u32 linearFrameCount;
extern u32 linearSoundByteCount;
extern u32 linearSoundFrameCount;

class XAudio2Sound : public ISound, public IXAudio2VoiceCallback
{
private:
	HMODULE					 hXAudio2;
	IXAudio2 *				 pXAudio2;
	IXAudio2MasteringVoice * pMasteringVoice;
	IXAudio2SourceVoice *	 pSourceVoice;
	WAVEFORMATEX			 wfx;
	float					 curRate;
	u8 *					 buffers[XAUDIO2_SOUND_BUFFERS];
	int						 nextBuffer;
	HANDLE					 bufferFreed;
	bool					 playing;

public:
	XAudio2Sound();
	virtual ~XAudio2Sound();

	bool init();
	void pause();
	void reset();
	void resume();
	void write();
	void setSpeed(float rate);
	bool isPlaying();
	void clearAudioBuffer();

	// IXAudio2VoiceCallback
	void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 bytesRequired);
	void STDMETHODCALLTYPE OnVoiceProcessingPassEnd();
	void STDMETHODCALLTYPE OnStreamEnd();
	void STDMETHODCALLTYPE OnBufferStart(void *pBufferContext);
	void STDMETHODCALLTYPE OnBufferEnd(void *pBufferContext);
	void STDMETHODCALLTYPE OnLoopEnd(void *pBufferContext);
	void STDMETHODCALLTYPE OnVoiceError(void *pBufferContext, HRESULT error);
};

XAudio2Sound::XAudio2Sound()
{
	hXAudio2		 = NULL;
	pXAudio2		 = NULL;
	pMasteringVoice	 = NULL;
	pSourceVoice	 = NULL;
	curRate			 = 0.0f;
	nextBuffer		 = 0;
	bufferFreed		 = NULL;
	playing			 = false;
	memset(&wfx, 0, sizeof(wfx));

	for (int i = 0; i < XAUDIO2_SOUND_BUFFERS; i++)
		buffers[i] = NULL;
}

XAudio2Sound::~XAudio2Sound()
{
	if (theApp.aviRecorder != NULL)
	{
		delete theApp.aviRecorder;
		theApp.aviRecorder	= NULL;
		theApp.aviRecording = false;
	}

	if (theApp.soundRecording)
	{
		if (theApp.soundRecorder != NULL)
		{
			delete theApp.soundRecorder;
			theApp.soundRecorder = NULL;
		}
		theApp.soundRecording = false;
	}

	if (pSourceVoice != NULL)
	{
		pSourceVoice->Stop(0);
		pSourceVoice->FlushSourceBuffers();
		pSourceVoice->DestroyVoice();
		pSourceVoice = NULL;
	}

	if (pMasteringVoice != NULL)
	{
		pMasteringVoice->DestroyVoice();
		pMasteringVoice = NULL;
	}

	if (pXAudio2 != NULL)
	{
		pXAudio2->Release();
		pXAudio2 = NULL;
	}

	if (hXAudio2 != NULL)
	{
		FreeLibrary(hXAudio2);
		hXAudio2 = NULL;
	}

	if (bufferFreed != NULL)
	{
		CloseHandle(bufferFreed);
		bufferFreed = NULL;
	}

	for (int i = 0; i < XAUDIO2_SOUND_BUFFERS; i++)
	{
		if (buffers[i] != NULL)
		{
			delete [] buffers[i];
			buffers[i] = NULL;
		}
	}
}

bool XAudio2Sound::init()
{
	HRESULT hr;

	// Same format and buffer sizes as the DirectSound backend: the emulation
	// core and the recorders depend on soundBufferLen.
	memset(&wfx, 0, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels  = 2;
	switch (soundQuality)
	{
	case 2:
		wfx.nSamplesPerSec	= 22050;
		soundBufferLen		= 736 * 2;
		soundBufferTotalLen = 7360 * 2;
		break;
	case 4:
		wfx.nSamplesPerSec	= 11025;
		soundBufferLen		= 368 * 2;
		soundBufferTotalLen = 3680 * 2;
		break;
	default:
		soundQuality		= 1;
		wfx.nSamplesPerSec	= 44100;
		soundBufferLen		= 1470 * 2;
		soundBufferTotalLen = 14700 * 2;
	}
	wfx.wBitsPerSample	= 16;
	wfx.nBlockAlign		= (wfx.wBitsPerSample / 8) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	hXAudio2 = LoadLibrary("XAudio2_9.dll");
	if (hXAudio2 == NULL)
	{
		directXMessage("XAudio2_9.dll");
		return false;
	}

	XAudio2CreateFn xaudio2Create = (XAudio2CreateFn)GetProcAddress(hXAudio2, "XAudio2Create");
	if (xaudio2Create == NULL)
	{
		directXMessage("XAudio2Create");
		return false;
	}

	if (FAILED(hr = xaudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		systemMessage(0, "Cannot create XAudio2 %08x", hr);
		pXAudio2 = NULL;
		return false;
	}

	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
	{
		systemMessage(0, "Cannot create XAudio2 mastering voice %08x", hr);
		pMasteringVoice = NULL;
		return false;
	}

	// MaxFrequencyRatio has to cover the fastest speedup the emulator uses.
	if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &wfx, 0, XAUDIO2_MAX_FREQ_RATIO,
	                                            this, NULL, NULL)))
	{
		systemMessage(0, "Cannot create XAudio2 source voice %08x", hr);
		pSourceVoice = NULL;
		return false;
	}

	bufferFreed = CreateEvent(NULL, FALSE, FALSE, NULL);

	for (int i = 0; i < XAUDIO2_SOUND_BUFFERS; i++)
		buffers[i] = new u8 [soundBufferLen];

	nextBuffer		 = 0;
	curRate			 = 0.0f;
	playing			 = false;

	systemSoundOn = true;

	return true;
}

void XAudio2Sound::pause()
{
	if (pSourceVoice != NULL && playing)
		pSourceVoice->Stop(0);

	playing = false;
}

void XAudio2Sound::resume()
{
	if (pSourceVoice != NULL)
		pSourceVoice->Start(0);

	playing = true;
}

void XAudio2Sound::reset()
{
	if (pSourceVoice != NULL)
	{
		pSourceVoice->Stop(0);
		pSourceVoice->FlushSourceBuffers();
	}

	playing = false;
}

void XAudio2Sound::setSpeed(float rate)
{
	if (pSourceVoice == NULL || wfx.nSamplesPerSec <= 0)
		return;

	if (rate != curRate)
	{
		curRate = rate;

		if (rate > 4.0f)
			rate = 4.0f;
		if (rate < 0.06f)
			rate = 0.06f;

		pSourceVoice->SetFrequencyRatio(rate);
	}
}

bool XAudio2Sound::isPlaying()
{
	return pSourceVoice != NULL && playing;
}

void XAudio2Sound::clearAudioBuffer()
{
	for (int i = 0; i < XAUDIO2_SOUND_BUFFERS; i++)
	{
		if (buffers[i] != NULL)
			memset(buffers[i], 0, soundBufferLen);
	}
}

void XAudio2Sound::write()
{
	u32 len		   = soundBufferLen;
	double frameRate = systemGetFrameRate();
	do
	{
		if (theApp.soundRecording)
		{
			if (theApp.soundRecorder == NULL)
			{
				theApp.soundRecorder = new WavWriter;

				if (theApp.soundRecorder->Open(theApp.soundRecordName))
					theApp.soundRecorder->SetFormat(&wfx);
			}

			if (theApp.soundRecorder)
			{
				theApp.soundRecorder->AddSound((u8 *)soundFinalWave, len);
			}
		}

		if (theApp.nvAudioLog)
		{
			NESVideoLoggingAudio((u8 *)soundFinalWave, wfx.nSamplesPerSec, wfx.wBitsPerSample, wfx.nChannels, len /
			                     (wfx.nChannels * (wfx.wBitsPerSample / 8)));
		}

		// alternate avi record routine has been added in VBA.cpp
		if (!theApp.altAviRecordMethod && theApp.aviRecording)
		{
			if (theApp.aviRecorder && !theApp.aviRecorder->IsPaused())
			{
				if (!theApp.aviRecorder->IsSoundAdded())
					theApp.aviRecorder->SetSoundFormat(&wfx);

				theApp.aviRecorder->AddSound((u8 *)soundFinalWave, len);
			}
		}

		linearSoundByteCount += len;
		linearSoundFrameCount = u32(frameRate * double(linearSoundByteCount) / double(wfx.nAvgBytesPerSec));
	}
	while (linearSoundFrameCount <= linearFrameCount);

	// arbitrarily wrap counters at systemGetFrameRateDividend() frames to avoid mismatching wrap-around freeze
	u32 wrap = systemGetFrameRateDividend();
	if (linearSoundFrameCount >= wrap && linearFrameCount >= wrap)
	{
		linearFrameCount	  -= wrap;
		linearSoundByteCount  -= wfx.nAvgBytesPerSec * systemGetFrameRateDivisor();
		linearSoundFrameCount -= wrap;
	}

	if (pSourceVoice == NULL)
		return;

	// Number of blocks the voice still has to play.  NOSAMPLESPLAYED keeps the
	// query from computing the sample position, which is the expensive part.
	XAUDIO2_VOICE_STATE state;
	memset(&state, 0, sizeof(state));
	pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	bool fastForward = speedup;
	fastForward |= theApp.frameSearchSkipping;

	// Waits for the device to have played a block, the role the DirectSound
	// play cursor had.  Skipped while fast forwarding or while the voice is
	// stopped, in which case nothing would ever be played and this would block
	// forever.
	if (!fastForward && synchronize && !(theApp.throttle > 100 && theApp.accuratePitchThrottle)
		&& theApp.throttle >= 6 && theApp.throttle <= 400)
	{
		if (playing)
		{
			if (!soundPaused)
			{
				while (state.BuffersQueued >= XAUDIO2_SOUND_BUFFERS)
				{
					if (bufferFreed != NULL)
						WaitForSingleObject(bufferFreed, 50);
					else
						Sleep(10);

					pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
				}
			}
		}
		else
		{
			soundPaused = 1;
		}
	}

	// Without the wait above the oldest block may still be queued; dropping
	// this one is better than overwriting audio that has not been played.
	if (state.BuffersQueued >= XAUDIO2_SOUND_BUFFERS)
		return;

	int slot  = nextBuffer;
	nextBuffer = (nextBuffer + 1) % XAUDIO2_SOUND_BUFFERS;

	// note: with the old frame timing, this can be entered after theApp.winPauseNextFrame was set to false...
	if (theApp.muteFrameAdvance && theApp.winPauseNextFrame || theApp.winMuteForNow)
		memset(buffers[slot], 0, len);
	else
		memcpy(buffers[slot], soundFinalWave, len);

	XAUDIO2_BUFFER buffer;
	memset(&buffer, 0, sizeof(buffer));
	buffer.AudioBytes = len;
	buffer.pAudioData = buffers[slot];
	buffer.pContext	  = buffers[slot];

	pSourceVoice->SubmitSourceBuffer(&buffer);

	soundNextPosition += soundBufferLen;
	soundNextPosition %= soundBufferTotalLen;
}

void STDMETHODCALLTYPE XAudio2Sound::OnVoiceProcessingPassStart(UINT32 bytesRequired)
{
}

void STDMETHODCALLTYPE XAudio2Sound::OnVoiceProcessingPassEnd()
{
}

void STDMETHODCALLTYPE XAudio2Sound::OnStreamEnd()
{
}

void STDMETHODCALLTYPE XAudio2Sound::OnBufferStart(void *pBufferContext)
{
}

// Called on an XAudio2 thread once a submitted block has been played.
void STDMETHODCALLTYPE XAudio2Sound::OnBufferEnd(void *pBufferContext)
{
	if (bufferFreed != NULL)
		SetEvent(bufferFreed);
}

void STDMETHODCALLTYPE XAudio2Sound::OnLoopEnd(void *pBufferContext)
{
}

void STDMETHODCALLTYPE XAudio2Sound::OnVoiceError(void *pBufferContext, HRESULT error)
{
}

ISound *newXAudio2()
{
	return new XAudio2Sound();
}
