#ifndef VBA_WIN32_WINMISCUTIL_H
#define VBA_WIN32_WINMISCUTIL_H

#pragma once

extern const char IDS_ROM_DIR[];
extern const char IDS_GBXROM_DIR[];
extern const char IDS_BATTERY_DIR[];
extern const char IDS_SAVE_DIR[];
extern const char IDS_MOVIE_DIR[];
extern const char IDS_CHEAT_DIR[];
extern const char IDS_LUA_DIR[];
extern const char IDS_IPS_DIR[];
extern const char IDS_AVI_DIR[];
extern const char IDS_WAV_DIR[];
extern const char IDS_CAPTURE_DIR[];
extern const char IDS_WATCH_DIR[];

extern bool winFileExists(const CString &filename);
extern bool winIsDriveRoot(const CString &file);
extern CString winGetOriginalFilename(const CString &file);
extern CString winGetDirFromFilename(const CString &file);
extern CString winGetSavestateFilename(const CString &LogicalRomName, int nID);
extern CString winGetSavestateMenuString(const CString &LogicalRomName, int nID);
extern CString winGetDestDir(const CString &TargetDirReg);
extern CString winGetDestFilename(const CString &LogicalRomName, const CString &TargetDirReg, const CString &ext);
extern void winCorrectPath(CString &path);
extern void winCorrectPath(char *path);

int winScreenCapture(int captureNumber = 0);

// Writes the frame the "window size and filter" capture option renders: the
// active pixel filter at the size the emulator is drawn.  Returns false when no
// such frame could be produced.  Defined in WinSystem.cpp.
bool winWriteFilteredScreenshot(const char *fileName);

// Saves a screenshot to `fileName`, honouring that option; without it the plain
// emulator screenshot is written.  False when nothing could be written.
bool winWriteScreenshotFile(const CString &fileName);

// Writes a BGR image produced by one of the viewer dialogs.  The encoders can
// refuse a format, so a failure is reported the way a failed screenshot is:
// the reason goes to the trace, the message box only points at it.
void winWriteImageBGR(const CString &fileName, int width, int height,
                      const unsigned char *bgr, int stride);
bool winImportGSACodeFile(CString& fileName);
void winLoadCheatList(const char *name);
void winSaveCheatList(const char *name);
void winLoadCheatListDefault();
void winSaveCheatListDefault();
bool winReadBatteryFile();
bool winWriteBatteryFile();
bool winEraseBatteryFile();
bool winReadSaveGame(const char *name);
bool winWriteSaveGame(const char *name);
bool winEraseSaveGame(const char *name);

#endif // VBA_WIN32_WINMISCUTIL_H
