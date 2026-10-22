#include "stdafx.h"
#include "Reg.h"

// What the settings store holds is deliberately still ANSI.
//
// A key written by an older build is a byte string in the code page the user's
// machine had at the time, and reading it back as UTF-16 would turn every path
// with a non-ASCII character in it into noise.  The narrow calls below keep the
// on-disk format exactly as it was, and only the names and values the Win32
// layer passes in and gets back are TCHAR.  Moving the store to Unicode needs a
// one-time conversion of the existing values, which is a change of its own.
static char    buffer[2048];
static HKEY    vbKey = NULL;
static CString regVbaPath;

#define VBA_PREF "preferences"

bool regEnabled = true;

void regInit(const CString &path)
{
	if (regEnabled)
	{
		DWORD disp = 0;
		LONG  res  = RegCreateKeyExA(HKEY_CURRENT_USER,
		                             "Software\\Emulators\\VisualBoyAdvance",
		                             0,
		                             "",
		                             REG_OPTION_NON_VOLATILE,
		                             KEY_ALL_ACCESS,
		                             NULL,
		                             &vbKey,
		                             &disp);
	}
	regVbaPath.Format(_T("%s\\vba.ini"), (LPCTSTR)path);
}

void regShutdown()
{
	LONG res = RegCloseKey(vbKey);
}

LPCTSTR regGetINIPath()
{
	return regVbaPath;
}

CString regQueryStringValue(LPCTSTR key, LPCTSTR def)
{
	const CStringA name = winAnsiFromWide(key);
	const CStringA ini  = winAnsiFromWide(regVbaPath);

	if (regEnabled)
	{
		DWORD type = 0;
		DWORD size = sizeof(buffer);

		LONG res = RegQueryValueExA(vbKey,
		                            name,
		                            NULL,
		                            &type,
		                            (UCHAR *)buffer,
		                            &size);

		if (res == ERROR_SUCCESS && type == REG_SZ)
			return winWideFromAnsi(buffer);

		return CString(def);
	}

	DWORD res = GetPrivateProfileStringA(VBA_PREF,
	                                     name,
	                                     winAnsiFromWide(def),
	                                     buffer,
	                                     sizeof(buffer),
	                                     ini);

	if (res)
		return winWideFromAnsi(buffer);

	return CString(def);
}

DWORD regQueryDwordValue(LPCTSTR key, DWORD def, bool force)
{
	const CStringA name = winAnsiFromWide(key);

	if (regEnabled || force)
	{
		DWORD type   = 0;
		DWORD size   = sizeof(DWORD);
		DWORD result = 0;

		LONG res = RegQueryValueExA(vbKey,
		                            name,
		                            NULL,
		                            &type,
		                            (UCHAR *)&result,
		                            &size);

		if (res == ERROR_SUCCESS && type == REG_DWORD)
			return result;

		return def;
	}

	return GetPrivateProfileIntA(VBA_PREF,
	                             name,
	                             def,
	                             winAnsiFromWide(regVbaPath));
}

BOOL regQueryBinaryValue(LPCTSTR key, char *value, int count, int *pActualCount)
{
	const CStringA name = winAnsiFromWide(key);

	if (pActualCount != NULL)
		*pActualCount = 0;

	if (regEnabled)
	{
		DWORD type   = 0;
		DWORD size   = count;
		DWORD result = 0;

		LONG res = RegQueryValueExA(vbKey,
		                            name,
		                            NULL,
		                            &type,
		                            (UCHAR *)value,
		                            &size);

		if (res == ERROR_SUCCESS && type == REG_BINARY)
		{
			if (pActualCount != NULL)
				*pActualCount = (int)size;
			return TRUE;
		}

		return FALSE;
	}
	CStringA k = name;
	k += "Count";
	int size = GetPrivateProfileIntA(VBA_PREF,
	                                 k,
	                                 -1,
	                                 winAnsiFromWide(regVbaPath));
	if (size >= 0 && size < count)
		count = size;
	BOOL res = GetPrivateProfileStructA(VBA_PREF,
	                                    name,
	                                    value,
	                                    count,
	                                    winAnsiFromWide(regVbaPath));
	if (res && pActualCount != NULL)
		*pActualCount = count;
	return res;
}

void regSetStringValue(LPCTSTR key, LPCTSTR value)
{
	const CStringA name = winAnsiFromWide(key);
	const CStringA data = winAnsiFromWide(value);

	if (regEnabled)
	{
		LONG res = RegSetValueExA(vbKey,
		                          name,
		                          NULL,
		                          REG_SZ,
		                          (const UCHAR *)data.GetString(),
		                          (DWORD)(data.GetLength() + 1));
	}
	else
	{
		WritePrivateProfileStringA(VBA_PREF,
		                           name,
		                           data,
		                           winAnsiFromWide(regVbaPath));
	}
}

void regSetDwordValue(LPCTSTR key, DWORD value, bool force)
{
	const CStringA name = winAnsiFromWide(key);

	if (regEnabled || force)
	{
		LONG res = RegSetValueExA(vbKey,
		                          name,
		                          NULL,
		                          REG_DWORD,
		                          (const UCHAR *)&value,
		                          sizeof(DWORD));
	}
	else
	{
		wsprintfA(buffer, "%u", value);
		WritePrivateProfileStringA(VBA_PREF,
		                           name,
		                           buffer,
		                           winAnsiFromWide(regVbaPath));
	}
}

void regSetBinaryValue(LPCTSTR key, char *value, int count)
{
	const CStringA name = winAnsiFromWide(key);

	if (regEnabled)
	{
		LONG res = RegSetValueExA(vbKey,
		                          name,
		                          NULL,
		                          REG_BINARY,
		                          (const UCHAR *)value,
		                          count);
	}
	else
	{
		CStringA k = name;
		k += "Count";
		wsprintfA(buffer, "%u", count);

		WritePrivateProfileStringA(VBA_PREF,
		                           k,
		                           buffer,
		                           winAnsiFromWide(regVbaPath));

		WritePrivateProfileStructA(VBA_PREF,
		                           name,
		                           value,
		                           count,
		                           winAnsiFromWide(regVbaPath));
	}
}

void regDeleteValue(LPCTSTR key)
{
	const CStringA name = winAnsiFromWide(key);

	if (regEnabled)
	{
		LONG res = RegDeleteValueA(vbKey,
		                           name);
	}
	else
	{
		WritePrivateProfileStringA(VBA_PREF,
		                           name,
		                           NULL,
		                           winAnsiFromWide(regVbaPath));
	}
}

bool regCreateFileType(LPCTSTR ext, LPCTSTR type)
{
	const CStringA extName = winAnsiFromWide(ext);
	const CStringA typeStr = winAnsiFromWide(type);

	DWORD disp = 0;
	HKEY  key;
	LONG  res = RegCreateKeyExA(HKEY_CLASSES_ROOT,
	                            extName,
	                            0,
	                            "",
	                            REG_OPTION_NON_VOLATILE,
	                            KEY_ALL_ACCESS,
	                            NULL,
	                            &key,
	                            &disp);
	if (res == ERROR_SUCCESS)
	{
		res = RegSetValueExA(key,
		                     "",
		                     0,
		                     REG_SZ,
		                     (const UCHAR *)typeStr.GetString(),
		                     (DWORD)(typeStr.GetLength() + 1));
		RegCloseKey(key);
		return true;
	}
	return false;
}

bool regAssociateType(LPCTSTR type, LPCTSTR desc, LPCTSTR application)
{
	const CStringA typeName = winAnsiFromWide(type);
	const CStringA descStr  = winAnsiFromWide(desc);
	const CStringA appStr   = winAnsiFromWide(application);

	DWORD disp = 0;
	HKEY  key;
	LONG  res = RegCreateKeyExA(HKEY_CLASSES_ROOT,
	                            typeName,
	                            0,
	                            "",
	                            REG_OPTION_NON_VOLATILE,
	                            KEY_ALL_ACCESS,
	                            NULL,
	                            &key,
	                            &disp);
	if (res == ERROR_SUCCESS)
	{
		res = RegSetValueExA(key,
		                     "",
		                     0,
		                     REG_SZ,
		                     (const UCHAR *)descStr.GetString(),
		                     (DWORD)(descStr.GetLength() + 1));
		HKEY key2;
		res = RegCreateKeyExA(key,
		                      "Shell\\Open\\Command",
		                      0,
		                      "",
		                      REG_OPTION_NON_VOLATILE,
		                      KEY_ALL_ACCESS,
		                      NULL,
		                      &key2,
		                      &disp);
		if (res == ERROR_SUCCESS)
		{
			res = RegSetValueExA(key2,
			                     "",
			                     0,
			                     REG_SZ,
			                     (const UCHAR *)appStr.GetString(),
			                     (DWORD)(appStr.GetLength() + 1));
			RegCloseKey(key2);
			RegCloseKey(key);
			return true;
		}

		RegCloseKey(key);
	}
	return false;
}

static void regExportSettingsToINI(HKEY key, const char *section)
{
	char valueName[256];
	int  index = 0;
	while (1)
	{
		DWORD nameSize = sizeof(valueName);
		DWORD size     = sizeof(buffer);
		DWORD type;
		LONG  res = RegEnumValueA(key,
		                          index,
		                          valueName,
		                          &nameSize,
		                          NULL,
		                          &type,
		                          (LPBYTE)buffer,
		                          &size);

		if (res == ERROR_SUCCESS)
		{
			switch (type)
			{
			case REG_DWORD:
			{
				char temp[256];
				wsprintfA(temp, "%u", *((DWORD *)buffer));
				WritePrivateProfileStringA(section,
				                           valueName,
				                           temp,
				                           winAnsiFromWide(regVbaPath));
				break;
			}
			case REG_SZ:
				WritePrivateProfileStringA(section,
				                           valueName,
				                           buffer,
				                           winAnsiFromWide(regVbaPath));
				break;
			case REG_BINARY:
			{
				char temp[256];

				wsprintfA(temp, "%u", size);
				CStringA k = valueName;
				k += "Count";
				WritePrivateProfileStringA(section,
				                           k,
				                           temp,
				                           winAnsiFromWide(regVbaPath));
				WritePrivateProfileStructA(section,
				                           valueName,
				                           buffer,
				                           size,
				                           winAnsiFromWide(regVbaPath));
				break;
			}
			}
			index++;
		}
		else
			break;
	}
}

void regExportSettingsToINI()
{
	if (vbKey != NULL)
	{
		regExportSettingsToINI(vbKey, VBA_PREF);
	}

	HKEY key;

	if (RegOpenKeyA(HKEY_CURRENT_USER,
	                "Software\\Emulators\\VisualBoyAdvance\\Viewer", &key) ==
	    ERROR_SUCCESS)
	{
		regExportSettingsToINI(key, "Viewer");
		RegCloseKey(key);
	}
}
