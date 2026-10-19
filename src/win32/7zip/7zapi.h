#ifndef _7ZAPI_HEADER
#define _7ZAPI_HEADER

// Dynamic loader for the official 7z.dll.
//
// The archive handlers used to be linked statically (wlib7zip.lib).  They are
// now reached through GetProcAddress, so the DLL can be shipped next to the
// executable or embedded in its resources.  When the DLL cannot be loaded the
// entry points below fail and the caller has to degrade gracefully instead of
// aborting.
//
// Search order:
//   1. 7z.dll next to the executable (users may replace it, as LGPL requires)
//   2. the copy embedded in the executable's resources (SEVENZ_EMBED builds)
//   3. %LOCALAPPDATA%\VBA-M\7z-<arch>-<tag>.dll left by a previous run
//
// Bump SEVENZ_DLL_TAG whenever the embedded blob is updated, so stale copies
// from an older build are not picked up.

#include "7z/C/7zTypes.h"
#include "7z/CPP/7zip/Archive/IArchive.h"

#ifndef SEVENZ_DLL_TAG
#define SEVENZ_DLL_TAG "1"
#endif

// These three are the exports the archive code uses (see 7-Zip's
// Archive/Archive2.def); the definitions below forward to the loaded DLL.
STDAPI GetNumberOfFormats(UINT32* numFormats);
STDAPI GetHandlerProperty2(UInt32 formatIndex, PROPID propID, PROPVARIANT* value);
STDAPI CreateObject(const GUID* clsid, const GUID* iid, void** outObject);

bool SevenZipLoad();
void SevenZipUnload();
bool SevenZipIsLoaded();

#endif
