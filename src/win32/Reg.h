#ifndef VBA_REG_H
#define VBA_REG_H

// Settings storage.
//
// The names and values are TCHAR, because the Win32 layer is Unicode.  What is
// stored stays in the ANSI code page, so a settings key written by an older
// build still reads back: see the note in Reg.cpp.
extern bool regEnabled;

CString regQueryStringValue(LPCTSTR key, LPCTSTR def);
DWORD regQueryDwordValue(LPCTSTR key, DWORD def, bool force = false);
BOOL regQueryBinaryValue(LPCTSTR key, char *value, int count, int *pActualCount = NULL);
void regSetStringValue(LPCTSTR key, LPCTSTR value);
void regSetDwordValue(LPCTSTR key, DWORD value, bool force = false);
void regSetBinaryValue(LPCTSTR key, char *value, int count);
void regDeleteValue(LPCTSTR key);
void regInit(const CString &path);
void regShutdown();
LPCTSTR regGetINIPath();
bool regCreateFileType(LPCTSTR ext, LPCTSTR type);
bool regAssociateType(LPCTSTR type, LPCTSTR desc, LPCTSTR application);
void regExportSettingsToINI();
#endif // VBA_REG_H
