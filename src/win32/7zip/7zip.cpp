#include "7zip.h"

#include "7z/C/7zTypes.h"
#include "7z/CPP/7zip/Archive/IArchive.h"

#include <string>
#include <vector>
#include <malloc.h>

#include "7zipstreams.h" // defines OutStream and InFileStream
#include "../../common/ziparchive.h" // built-in ZIP reader used when 7z.dll is missing

#include "7zapi.h" // loads 7z.dll and forwards its exported functions

struct ArchiveFormatInfo
{
	std::string name;
	std::vector<std::string> extensions;
	std::string signature;
	GUID guid;
};

static std::vector<ArchiveFormatInfo> s_formatInfos;

static std::string wstrToStr(const wchar_t* wstr)
{
/*
	// This thing wouldn't work
	char* str = (char*)_alloca((wcslen(wstr)+1));
    sprintf(str, "%S", wstr);
	return std::string(str);
*/
//	setlocale(LC_CTYPE, ".ACP");
	size_t n = wcstombs(NULL, wstr, 0);
	if (n == (size_t)-1)	// failed
	{
//		setlocale(LC_CTYPE, "C");
		return std::string();
	}
	char* str = (char*)_alloca(n + 1);
	wcstombs(str, wstr, n);
	str[n] = '\0';
//	setlocale(LC_CTYPE, "C");
	return std::string(str);
}

static std::vector<std::string> tokenize(const std::string & str, const std::string & delim)
{
	std::vector<std::string> tokens;
	size_t p0 = 0, p1 = std::string::npos;
	while(p0 != std::string::npos)
	{
		p1 = str.find_first_of(delim, p0);
		if(p1 != p0)
		{
			std::string token = str.substr(p0, p1 - p0);
			tokens.push_back(token);
		}
		p0 = str.find_first_not_of(delim, p1);
	}
	return tokens;
}

static std::string s_supportedFormatsFilter;
const char* GetSupportedFormatsFilter()
{
	if(s_supportedFormatsFilter.empty())
	{
		s_supportedFormatsFilter = "";
		for(size_t i = 0; i < s_formatInfos.size(); i++)
		{
			for(size_t j = 0; j < s_formatInfos[i].extensions.size(); j++)
			{
				s_supportedFormatsFilter += ";*.";
				s_supportedFormatsFilter += s_formatInfos[i].extensions[j];
			}
		}
	}
	return s_supportedFormatsFilter.c_str();
}

void InitDecoder()
{
	CleanupDecoder();

	if(!SevenZipLoad())
		return; // archive support unavailable; ArchiveFile() degrades to plain files

	UINT32 numFormats = 0;
	GetNumberOfFormats(&numFormats);

	for(unsigned int i = 0; i < numFormats; i++)
	{
		PROPVARIANT var = {VT_EMPTY};
		ArchiveFormatInfo info;

		GetHandlerProperty2(i, NArchive::NHandlerPropID::kName, &var);
		if(var.vt == VT_BSTR)
			info.name = wstrToStr(var.bstrVal);

		GetHandlerProperty2(i, NArchive::NHandlerPropID::kExtension, &var);
		if(var.vt == VT_BSTR)
			info.extensions = tokenize(wstrToStr(var.bstrVal), " ");

		GetHandlerProperty2(i, NArchive::NHandlerPropID::kSignature, &var);
		if(var.vt == VT_BSTR)
			info.signature = (const char*)var.bstrVal; // note: there's no 100% correct way of doing this with the existing 7-zip interface, but this is much better than using SysStringLen() in any way (it would return 1 for the signature "BZh", for example)

		GetHandlerProperty2(i,NArchive::NHandlerPropID::kClassID,&var);
		if(var.vt == VT_BSTR)
			memcpy(&info.guid, var.bstrVal, 16);
		else
			memset(&info.guid, 0, 16);

		s_formatInfos.push_back(info);

		VariantClear((VARIANTARG*)&var);
	}
}

void CleanupDecoder()
{
	s_formatInfos.clear();
	s_supportedFormatsFilter.clear();
	SevenZipUnload();
}

bool IsArchiveSupportAvailable()
{
	return SevenZipIsLoaded();
}

// Extensions that must never be handed to the ROM loaders as raw data when
// archive support is missing.  With 7z.dll available, detection uses format
// signatures instead and this list is not consulted.
static bool LooksLikeArchive(const char* filename)
{
	static const char* extensions[] = {
		"7z", "rar", "r00", "lzma", "bz2", "bzip2", "tar", "gz", "z", "xz",
		"cab", "iso"
	};

	const char* ext = strrchr(filename, '.');
	if(!ext++)
		return false;

	for(size_t i = 0; i < sizeof(extensions) / sizeof(*extensions); i++)
	{
		if(!_stricmp(ext, extensions[i]))
			return true;
	}
	return false;
}


ArchiveFile::ArchiveFile(const char* filename, const char* displayFilename)
{
	m_typeIndex = -1;
	m_numItems = 0;
	m_items = NULL;
	m_filename = NULL;
	m_displayFilename = NULL;
	m_userMadeSelection = false;
	m_unsupportedArchive = false;
	m_zip = NULL;

	if(!SevenZipIsLoaded())
	{
		// No 7z.dll: fall back to the built-in ZIP reader for .zip files.
		const char* ext = strrchr(filename, '.');
		if(ext && !_stricmp(ext, ".zip"))
		{
			m_zip = new ZipArchive(filename);
			if(!m_zip->IsValid())
			{
				delete m_zip;
				m_zip = NULL;
				m_unsupportedArchive = true;
				return;
			}

			m_numItems = m_zip->GetEntryCount();
			m_items = new ArchiveItem[m_numItems];

			for(int i = 0; i < m_numItems; i++)
			{
				ZipEntryInfo info;
				m_zip->GetEntryInfo(i, &info);

				m_items[i].size = info.uncompressedSize > 0x7FFFFFFFu
				                ? 0 : (int)info.uncompressedSize;
				m_items[i].name = new char[strlen(info.name) + 1];
				strcpy(m_items[i].name, info.name);
			}

			m_filename = new char[strlen(filename)+1];
			strcpy(m_filename, filename);

			if(displayFilename)
			{
				m_displayFilename = new char[strlen(displayFilename)+1];
				strcpy(m_displayFilename, displayFilename);
			}
			return;
		}

		if(LooksLikeArchive(filename))
		{
			m_unsupportedArchive = true;
			return;
		}
	}

	FILE* file = fopen(filename, "rb");
	if(!file)
		return;

	m_filename = new char[strlen(filename)+1];
	strcpy(m_filename, filename);

	if(displayFilename)
	{
		m_displayFilename = new char[strlen(displayFilename)+1];
		strcpy(m_displayFilename, displayFilename);
	}

	// detect archive type using format signature in file
	for(size_t i = 0; i < s_formatInfos.size() && m_typeIndex < 0; i++)
	{
		fseek(file, 0, SEEK_SET);
		
		std::string& formatSig = s_formatInfos[i].signature;
		int len = (int)formatSig.size();

		if(len == 0)
			continue; // because some formats have no signature

		char* fileSig = (char*)_alloca(len);
		fread(fileSig, 1, len, file);

		if(!memcmp(formatSig.c_str(), fileSig, len))
			m_typeIndex = (int)i;
	}

	// if no signature match has been found, detect archive type using filename.
	// this is only for signature-less formats
	const char* fileExt = strrchr(filename, '.');
	if(fileExt++)
	{
		for(size_t i = 0; i < s_formatInfos.size() && m_typeIndex < 0; i++)
		{
			if(s_formatInfos[i].signature.empty())
			{
				std::vector<std::string>& formatExts = s_formatInfos[i].extensions;
				for(size_t j = 0; j < formatExts.size(); j++)
				{
					if(!_stricmp(formatExts[j].c_str(), fileExt))
					{
						m_typeIndex = (int)i;
						break;
					}
				}
			}
		}
	}

	if(m_typeIndex < 0)
	{
		// uncompressed

		m_numItems = 1;
		m_items = new ArchiveItem[m_numItems];

		fseek(file, 0, SEEK_END);
		m_items[0].size = ftell(file);

		m_items[0].name = new char[strlen(filename)+1];
		strcpy(m_items[0].name, filename);
	}
	else
	{
		IInArchive* object = NULL;
		if(SUCCEEDED(CreateObject(&s_formatInfos[m_typeIndex].guid, &IID_IInArchive, (void**)&object)))
		{
			InFileStream* ifs = new InFileStream(filename);
			if(SUCCEEDED(object->Open(ifs,0,0)))
			{
				UInt32 numItems = 0;
				object->GetNumberOfItems(&numItems);
				m_numItems = numItems;
				m_items = new ArchiveItem[m_numItems];

				for(int i = 0; i < m_numItems; i++)
				{
					PROPVARIANT var = {VT_EMPTY};
					ArchiveItem& item = m_items[i];

					object->GetProperty(i, kpidSize, &var);
					item.size = var.uhVal.LowPart;

					object->GetProperty(i, kpidPath, &var);
					const std::string path = wstrToStr(var.bstrVal);
					item.name = new char[path.size()+1];
					strcpy(item.name, path.c_str());

					//object->GetProperty(i, kpidMethod, &var);
					//std::string& method = wstrToStr(var.bstrVal);
					//item.method = new char[method.size()+1];
					//strcpy(item.method, method.c_str());

					object->GetProperty(i, kpidEncrypted, &var);
					if(var.boolVal)
						item.size = 0; // encrypted entries cannot be extracted, hide them

					VariantClear((VARIANTARG*)&var);
				}

				object->Close();
			}

			object->Release();
		}
	}

	fclose(file);
}

ArchiveFile::~ArchiveFile()
{
	for(int i = 0; i < m_numItems; i++)
	{
		delete[] m_items[i].name;
	}
	delete[] m_items;
	delete m_zip;
	delete[] m_filename;
	delete[] m_displayFilename;
}

const char* ArchiveFile::GetArchiveTypeName()
{
	if(m_zip)
		return "Zip";

	if((size_t)m_typeIndex >= s_formatInfos.size())
		return "";

	return s_formatInfos[m_typeIndex].name.c_str();
}

int ArchiveFile::GetNumItems()
{
	return m_numItems;
}

int ArchiveFile::GetItemSize(int item)
{
	if(!(item >= 0 && item < m_numItems)) return 0;
	return m_items[item].size;
}

const char* ArchiveFile::GetItemName(int item)
{
	//assert(item >= 0 && item < m_numItems);
	if(!(item >= 0 && item < m_numItems)) return "";
	return m_items[item].name;
}

bool ArchiveFile::IsCompressed()
{
	return (m_typeIndex >= 0) || (m_zip != NULL);
}

int ArchiveFile::ExtractItem(int index, unsigned char* outBuffer, int bufSize) const
{
	//assert(index >= 0 && index < m_numItems);
	if(!(index >= 0 && index < m_numItems)) return 0;

	ArchiveItem& item = m_items[index];

	if(bufSize < item.size)
		return 0;

	if(m_zip)
		return m_zip->ExtractEntry(index, outBuffer, (unsigned int)item.size);

	if(m_typeIndex < 0)
	{
		// uncompressed
		FILE* file = fopen(m_filename, "rb");
		fread(outBuffer, 1, item.size, file);
		fclose(file);
	}
	else
	{
		IInArchive* object = NULL;
		HRESULT hr = E_FAIL;
		if(SUCCEEDED(CreateObject(&s_formatInfos[m_typeIndex].guid, &IID_IInArchive, (void**)&object)))
		{
			InFileStream* ifs = new InFileStream(m_filename);
			if(SUCCEEDED(object->Open(ifs,0,0)))
			{
				OutStream* os = new OutStream(index, outBuffer, item.size);
				const UInt32 indices [1] = {(UInt32)index};
				hr = object->Extract(indices, 1, 0, os);
				object->Close();
			}
			object->Release();
		}
		if(FAILED(hr))
			return 0;
	}

	return item.size;
}



int ArchiveFile::ExtractItem(int index, const char* outFilename) const
{
	//assert(index >= 0 && index < m_numItems);
	if(!(index >= 0 && index < m_numItems)) return 0;

	ArchiveItem& item = m_items[index];
	int rv = item.size;

	DWORD outAttributes = GetFileAttributesA(outFilename);
	// INVALID_FILE_ATTRIBUTES is all ones, so testing the bit alone would
	// wrongly mark a freshly created file read-only afterwards.
	bool restoreReadOnly = outAttributes != INVALID_FILE_ATTRIBUTES &&
	                       (outAttributes & FILE_ATTRIBUTE_READONLY) != 0;
	if(restoreReadOnly)
		SetFileAttributesA(outFilename, outAttributes & ~FILE_ATTRIBUTE_READONLY); // temporarily remove read-only attribute so we can decompress to there

	if(m_zip)
	{
		unsigned char* buffer = (unsigned char*)malloc(item.size);
		if(!buffer)
			rv = 0;
		else
		{
			int got = m_zip->ExtractEntry(index, buffer, (unsigned int)item.size);
			if(got == item.size)
			{
				FILE* file = fopen(outFilename, "wb");
				if(!file || fwrite(buffer, 1, item.size, file) != (size_t)item.size)
					rv = 0;
				if(file)
					fclose(file);
			}
			else
				rv = 0;

			free(buffer);
		}
	}
	else if(m_typeIndex < 0)
	{
		// uncompressed
		if(!CopyFileA(m_filename, outFilename, false))
			rv = 0;
	}
	else
	{
		IInArchive* object = NULL;
		HRESULT hr = E_FAIL;
		if(SUCCEEDED(CreateObject(&s_formatInfos[m_typeIndex].guid, &IID_IInArchive, (void**)&object)))
		{
			InFileStream* ifs = new InFileStream(m_filename);
			if(SUCCEEDED(object->Open(ifs,0,0)))
			{
				OutStream* os = new OutStream(index, outFilename);
				const UInt32 indices [1] = {(UInt32)index};
				hr = object->Extract(indices, 1, 0, os);
				object->Close();
			}
			object->Release();
		}
		if(FAILED(hr))
			rv = 0;
	}

	if(restoreReadOnly)
		SetFileAttributesA(outFilename, outAttributes); // restore read-only attribute

	return rv;
}
