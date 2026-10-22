#include "stdafx.h"
#include "WinMiscUtil.h"
#include "WinResUtil.h"
#include "WinText.h"
#include "resource.h"
#include "../NLS.h"
#include "VBA.h"
#include "Reg.h"
#include "../common/SystemGlobals.h"
#include "../common/System.h"
#include "../common/ImageWrite.h"
#include "../common/movie.h"
#include <direct.h>
#include <algorithm>

#include "Dialogs/GSACodeSelect.h"
#include "../gba/GBACheats.h"
#include "../gb/gbCheats.h"

// #undef WinDef macro garbage
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

using std::max;
using std::min;

extern int emulating;

extern const TCHAR IDS_ROM_DIR[]		= _T("romDir");
extern const TCHAR IDS_GBXROM_DIR[]	= _T("gbromDir");
extern const TCHAR IDS_BATTERY_DIR[]	= _T("batteryDir");
extern const TCHAR IDS_SAVE_DIR[]	= _T("saveDir");
extern const TCHAR IDS_MOVIE_DIR[]	= _T("moviesDir");
extern const TCHAR IDS_CHEAT_DIR[]	= _T("cheatsDir");
extern const TCHAR IDS_LUA_DIR[]		= _T("luaDir");
extern const TCHAR IDS_IPS_DIR[]		= _T("ipsDir");
extern const TCHAR IDS_AVI_DIR[]		= _T("aviRecordDir");
extern const TCHAR IDS_WAV_DIR[]		= _T("soundRecordDir");
extern const TCHAR IDS_CAPTURE_DIR[] = _T("captureDir");
extern const TCHAR IDS_WATCH_DIR[]	= _T("watchDir");

extern const TCHAR IDS_ROM_DEFAULT_DIR[]		= _T("\\roms");
extern const TCHAR IDS_GBXROM_DEFAULT_DIR[]	= _T("\\gbroms");
extern const TCHAR IDS_BATTERY_DEFAULT_DIR[]	= _T("\\battery");
extern const TCHAR IDS_SAVE_DEFAULT_DIR[]	= _T("\\save");
extern const TCHAR IDS_MOVIE_DEFAULT_DIR[]	= _T("\\movies");
extern const TCHAR IDS_CHEAT_DEFAULT_DIR[]	= _T("\\cheats");
extern const TCHAR IDS_LUA_DEFAULT_DIR[]		= _T("\\lua");
extern const TCHAR IDS_IPS_DEFAULT_DIR[]		= _T("\\ips");
extern const TCHAR IDS_AVI_DEFAULT_DIR[]		= _T("\\avi");
extern const TCHAR IDS_WAV_DEFAULT_DIR[]		= _T("\\wav");
extern const TCHAR IDS_CAPTURE_DEFAULT_DIR[] = _T("\\screen");
extern const TCHAR IDS_WATCH_DEFAULT_DIR[]	= _T("\\watches");

extern const TCHAR *IDS_tbl[] = {
	IDS_ROM_DIR,   IDS_GBXROM_DIR, IDS_BATTERY_DIR, IDS_SAVE_DIR, 
	IDS_MOVIE_DIR, IDS_CHEAT_DIR,  IDS_LUA_DIR,     IDS_IPS_DIR, 
	IDS_AVI_DIR,   IDS_WAV_DIR,    IDS_CAPTURE_DIR, IDS_WATCH_DIR
};

extern const TCHAR *IDS_def_tbl[] = {
	IDS_ROM_DEFAULT_DIR,   IDS_GBXROM_DEFAULT_DIR, IDS_BATTERY_DEFAULT_DIR, IDS_SAVE_DEFAULT_DIR, 
	IDS_MOVIE_DEFAULT_DIR, IDS_CHEAT_DEFAULT_DIR,  IDS_LUA_DEFAULT_DIR,     IDS_IPS_DEFAULT_DIR, 
	IDS_AVI_DEFAULT_DIR,   IDS_WAV_DEFAULT_DIR,    IDS_CAPTURE_DEFAULT_DIR, IDS_WATCH_DEFAULT_DIR
};

// these could be made VBA members, but  the VBA class is already oversized too much
//

bool winFileExists(const CString &filename)
{
	FILE *f = _wfopen(filename, L"rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

bool winIsDriveRoot(const CString &file)
{
	if (file.GetLength() == 3)
	{
		if (file[1] == ':' && file[2] == '\\')
			return true;
	}
	return false;
}

CString winGetOriginalFilename(const CString &file)
{
	int index = file.Find('|');

	if (index != -1)
		return file.Left(index);
	else
		return file;
}

CString winGetDirFromFilename(const CString &file)
{
	CString temp  = winGetOriginalFilename(file);
	int		index = max(temp.ReverseFind('/'), temp.ReverseFind('\\'));
	if (index != -1)
	{
		temp = temp.Left(index);
		if (temp.GetLength() == 2 && temp[1] == ':')
			temp += "\\";
	}

	return temp;
}

CString winGetDestDir(const CString &TargetDirReg)
{
	CString targetDir = regQueryStringValue(TargetDirReg, NULL);
	int pos = targetDir.ReverseFind('\\');
	if (pos > 0 && pos == targetDir.GetLength() - 1)
		targetDir.Delete(pos);

	// it makes no sense to create rom directories
	// see MainWnd::winFileOpenSelect for more info
	if (!TargetDirReg.Compare(IDS_ROM_DIR) || !TargetDirReg.Compare(IDS_GBXROM_DIR))
		return targetDir;

	if (targetDir.IsEmpty())
	{
		targetDir = theApp.exeDir;		// reset the targetDir to the application's path
		if (!TargetDirReg.Compare(IDS_BATTERY_DIR))
		{
			targetDir += IDS_BATTERY_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_SAVE_DIR))
		{
			targetDir += IDS_SAVE_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_MOVIE_DIR))
		{
			targetDir += IDS_MOVIE_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_CHEAT_DIR))
		{
			targetDir += IDS_CHEAT_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_LUA_DIR))
		{
			targetDir += IDS_LUA_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_IPS_DIR))
		{
			targetDir += IDS_IPS_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_AVI_DIR))
		{
			targetDir += IDS_AVI_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_WAV_DIR))
		{
			targetDir += IDS_WAV_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_CAPTURE_DIR))
		{
			targetDir += IDS_CAPTURE_DEFAULT_DIR;
		}
		else if (!TargetDirReg.Compare(IDS_WATCH_DIR))
		{
			targetDir += IDS_WATCH_DEFAULT_DIR;
		}
		regSetStringValue(TargetDirReg, targetDir);	// Add the directory to the INI file
	}

	_wmkdir(targetDir);			// make the directory

	return targetDir;
}

CString winGetDestFilename(const CString &LogicalRomName, const CString &TargetDirReg, const CString &ext)
{
	if (LogicalRomName.GetLength() == 0)
		return CString();

	CString targetDir = winGetDestDir(TargetDirReg);
	targetDir += '\\';

	CString buffer = LogicalRomName;

	int index = max(buffer.ReverseFind('/'), max(buffer.ReverseFind('\\'), buffer.ReverseFind('|')));
	if (index != -1)
		buffer = buffer.Right(buffer.GetLength() - index - 1);

	index = buffer.ReverseFind('.');
	if (index != -1)
		buffer = buffer.Left(index);

	CString filename;
	filename.Format(_T("%s%s%s"), targetDir, buffer, ext);
	bool fileExists = winFileExists(filename);

	// check for old style of naming, for better backward compatibility
	if (!fileExists || theApp.filenamePreference == 0)
	{
		index = LogicalRomName.Find('|');
		if (index != -1)
		{
			buffer = LogicalRomName.Left(index);
			index  = max(buffer.ReverseFind('/'), buffer.ReverseFind('\\'));

			int dotIndex = buffer.ReverseFind('.');
			if (dotIndex > index)
				buffer = buffer.Left(dotIndex);

			if (index != -1)
				buffer = buffer.Right(buffer.GetLength() - index - 1);

			CString filename2;
			filename2.Format(_T("%s%s%s"), targetDir, buffer, ext);
			bool file2Exists = winFileExists(filename2);

			if ((file2Exists && !fileExists) || (theApp.filenamePreference == 0 && (file2Exists || !fileExists)))
				return filename2;
		}
	}

	return filename;
}

CString winGetSavestateFilename(const CString &LogicalRomName, int nID)
{
	CString		ext;
//	size_t		startindex;	// forget about C89/ANSI-C
//	size_t		endindex;
	if (VBAMovieIsActive() && theApp.AsscWithSaveState)
	{
		std::string fs(VBAMovieGetFilename());	// RVO tip
		size_t startindex = fs.find_last_of("/\\");
		if (startindex < fs.length())
			++startindex;	// luckily the found character can't be at the end of fs
		else
			startindex = 0;
		size_t endindex = fs.find_last_of(".");
		if (endindex < fs.length() && endindex > startindex)
			endindex;	//??
		else
			endindex = fs.length();
		fs = fs.substr(startindex, endindex - startindex);
		// The name came from the narrow core side, so it crosses through the
		// helper rather than pasting its bytes into a wide format: a plain %s
		// in a wide format reads them as UTF-16 mules, see WinText.h.
		ext.Format(_T("-%s-%d.sgm"), (LPCTSTR)winWideFromAnsi(fs), nID);
	}
	else
	{
		ext.Format(_T("%d.sgm"), nID);
	}
	return winGetDestFilename(LogicalRomName, IDS_SAVE_DIR, ext);
}

CString winGetSavestateMenuString(const CString &LogicalRomName, int nID)
{
	CString str;
	if (theApp.showSlotTime)
	{
		CFileStatus status;
		if (emulating && CFile::GetStatus(winGetSavestateFilename(LogicalRomName, nID), status))
		{
			str.Format(_T("#&%d %s"), nID, status.m_mtime.Format(_T("%Y/%m/%d %H:%M:%S")));
		}
		else
		{
			str.Format(_T("#&%d ----/--/-- --:--:--"), nID);
		}
	}
	else
	{
		str.Format(_T("Slot #&%d"), nID);
	}

	return str;
}

void winCorrectPath(CString &path)
{
	if (winFileExists(path))
	{
		return;
	}

	CString tempStr = theApp.exeDir;
	tempStr += "\\";
	tempStr += path;

	if (winFileExists(tempStr))
	{
		path = tempStr;
		return;
	}

	for (int i = 0; i < _countof(IDS_tbl); ++i)
	{
		tempStr = winGetDestDir(IDS_tbl[i]);
		tempStr += "\\";
		tempStr += path;

		if (winFileExists(tempStr))
		{
			path = tempStr;
			return;
		}
	}
}

void winCorrectPath(char *path)
{
	CString pathCStr(path);
	winCorrectPath(pathCStr);
	strcpy(path, winAnsiFromWide(pathCStr));
}

// some file I/O

// Saves a screenshot to `fileName`.  With the "window size and filter" capture
// option on this is exactly what an AVI frame would contain (pixel filter plus
// window magnification); otherwise it is the plain emulator screenshot.
bool winWriteScreenshotFile(const CString &fileName)
{
	if (theApp.captureScaled && winWriteFilteredScreenshot(fileName))
		return true;

	return theApp.emulator.emuWriteImage(winAnsiFromWide(fileName));
}

// Writes an image a viewer dialog built.  These used to ignore the result and
// fail without a word; now the reason is in the trace and the user is told
// where to look, same as for a failed screenshot.
void winWriteImageBGR(const CString &fileName, int width, int height,
                      const unsigned char *bgr, int stride)
{
	if (utilWriteImageBGR(winAnsiFromWide(fileName), width, height, bgr, stride))
		return;

	utilReportImageSaveFailure(winAnsiFromWide(fileName));
}

int winScreenCapture(int captureNumber)
{
	CString captureName;

	do
	{
		CString ext;
		ext.Format(_T("_%03d%s"), captureNumber, imageCaptureExtension());

		captureName = winGetDestFilename(theApp.gameFilename, IDS_CAPTURE_DIR, ext);
		++captureNumber;
	} while (winFileExists(captureName) && captureNumber > 0);

	if (captureNumber < 0)
	{
		systemMessage(0, "Too many existing files (not less than %d)! Screen capture failed!", captureNumber - 1);
		return 0;
	}

	if (!winWriteScreenshotFile(captureName))
		return captureNumber;

	systemScreenMessage(winAnsiFromWide(winResLoadString(IDS_SCREEN_CAPTURE)));

	return captureNumber;
}

bool winImportGSACodeFile(CString &fileName)
{
	FILE *f = _wfopen(fileName, L"rb");

	if (f == NULL)
	{
		systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
		return false;
	}

	if (systemCartridgeType == 1)
	{
		fclose(f);
		return gbCheatReadGSCodeFile(winAnsiFromWide(fileName));
	}

	u32 len;
	fread(&len, 1, 4, f);
	if (len != 14)
	{
		fclose(f);
		systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
		              fileName);
		return false;
	}
	char buffer[16];
	fread(buffer, 1, 14, f);
	buffer[14] = 0;
	if (memcmp(buffer, "SharkPortCODES", 14))
	{
		fclose(f);
		systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
		              fileName);
		return false;
	}
	fseek(f, 0x1e, SEEK_SET);
	fread(&len, 1, 4, f);
	int game = 0;
	if (len > 1)
	{
		GSACodeSelect dlg(f);
		game = (int)dlg.DoModal();
	}
	fclose(f);

	bool v3 = false;

	int index = fileName.ReverseFind('.');

	if (index != -1)
	{
		if (fileName.Right(3).CompareNoCase(_T("XPC")) == 0)
			v3 = true;
	}

	if (game != -1)
	{
		return cheatsImportGSACodeFile(winAnsiFromWide(fileName), game, v3);
	}

	return true;
}

void winLoadCheatList(LPCTSTR name)
{
	bool res = false;

	if (systemCartridgeType == IMAGE_GBA)
		res = cheatsLoadCheatList(winAnsiFromWide(name));
	else
		res = gbCheatsLoadCheatList(winAnsiFromWide(name));

	if (res)
		systemScreenMessage(winAnsiFromWide(winResLoadString(IDS_LOADED_CHEATS)));
}

void winSaveCheatList(LPCTSTR name)
{
	if (systemCartridgeType == IMAGE_GBA)
		cheatsSaveCheatList(winAnsiFromWide(name));
	else
		gbCheatsSaveCheatList(winAnsiFromWide(name));
}

void winLoadCheatListDefault()
{
	CString cheatName = winGetDestFilename(theApp.gameFilename, IDS_CHEAT_DIR, ".clt");

	winLoadCheatList(cheatName);
}

void winSaveCheatListDefault()
{
	CString cheatName = winGetDestFilename(theApp.gameFilename, IDS_CHEAT_DIR, ".clt");

	winSaveCheatList(cheatName);
}

bool winReadBatteryFile()
{
	CString batteryName = winGetDestFilename(theApp.gameFilename, IDS_BATTERY_DIR, ".sav");

	bool res = false;

	if (theApp.emulator.emuReadBattery)
		res = theApp.emulator.emuReadBattery(winAnsiFromWide(batteryName));

	if (res)
		systemScreenMessage(winAnsiFromWide(winResLoadString(IDS_LOADED_BATTERY)));

	return res;
}

bool winWriteBatteryFile()
{
	CString batteryName = winGetDestFilename(theApp.gameFilename, IDS_BATTERY_DIR, ".sav");

	if (theApp.emulator.emuWriteBattery)
		return theApp.emulator.emuWriteBattery(winAnsiFromWide(batteryName));

	return false;
}

bool winEraseBatteryFile()
{
	CString batteryName = winGetDestFilename(theApp.gameFilename, IDS_BATTERY_DIR, ".sav");
	return !_wremove(batteryName);
}

bool winReadSaveGame(LPCTSTR name)
{
	if (theApp.emulator.emuReadState)
		return theApp.emulator.emuReadState(winAnsiFromWide(name));
	return false;
}

bool winWriteSaveGame(LPCTSTR name)
{
	if (theApp.emulator.emuWriteState)
		return theApp.emulator.emuWriteState(winAnsiFromWide(name));
	return false;
}

bool winEraseSaveGame(LPCTSTR name)
{
	return !_wremove(name);
}
