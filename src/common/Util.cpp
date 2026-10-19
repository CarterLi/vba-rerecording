#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <zlib.h>

#include "ImageWrite.h"

#if 0
#include "unrarlib.h"
#endif

#include "ziparchive.h"

#include "../NLS.h"
#include "System.h"
#include "Util.h"
#include "../gba/Flash.h"
#include "../gba/RTC.h"

extern "C" {
#include "memgzio.h"
}

#ifndef _MSC_VER
#define _stricmp strcasecmp
#endif // ! _MSC_VER

extern int32 cpuSaveType;

extern int systemColorDepth;
extern int systemRedShift;
extern int systemGreenShift;
extern int systemBlueShift;

extern u16 systemColorMap16[0x10000];
extern u32 systemColorMap32[0x10000];

static int	   (ZEXPORT *utilGzWriteFunc)(gzFile, voidp, unsigned int) = NULL;
static int	   (ZEXPORT *utilGzReadFunc)(gzFile, voidp, unsigned int)  = NULL;
static int	   (ZEXPORT *utilGzCloseFunc)(gzFile) = NULL;
static z_off_t (ZEXPORT *utilGzSeekFunc)(gzFile, z_off_t, int) = NULL;
static z_off_t (ZEXPORT *utilGzTellFunc)(gzFile) = NULL;

//Kludge to get it to compile in Linux, GCC cannot convert
//gzwrite function pointer to the type of utilGzWriteFunc
//due to void* and const void* differences
//--Felipe 
int gzWrite(gzFile file, void* buf, unsigned len){
	return gzwrite(file,buf,len);
}

void utilPutDword(u8 *p, u32 value)
{
	*p++ = value & 255;
	*p++ = (value >> 8) & 255;
	*p++ = (value >> 16) & 255;
	*p	 = (value >> 24) & 255;
}

void utilPutWord(u8 *p, u16 value)
{
	*p++ = value & 255;
	*p	 = (value >> 8) & 255;
}

void utilWriteBMP(u8 *b, int w, int h, int dstDepth, const u8 *pix)
{
	int sizeX = w;
	int sizeY = h;

	switch (dstDepth > 0 ? dstDepth : systemColorDepth)
	{
	case 16:
	{
		const u16 *p = (const u16 *)(pix + (w + 2) * (h) * 2); // skip first black line
		for (int y = 0; y < sizeY; y++)
		{
			for (int x = 0; x < sizeX; x++)
			{
				u16 v = *p++;

				*b++ = ((v >> systemBlueShift) & 0x01f) << 3; // B
				*b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
				*b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
			}
			p++; // skip black pixel for filters
			p++; // skip black pixel for filters
			p -= 2 * (w + 2);
		}
		break;
	}
	case 24:
	{
		const u8 *pixU8 = (const u8 *)pix + 3 * w * (h - 1);
		for (int y = 0; y < sizeY; y++)
		{
			for (int x = 0; x < sizeX; x++)
			{
				if (systemRedShift > systemBlueShift)
				{
					*b++ = *pixU8++; // B
					*b++ = *pixU8++; // G
					*b++ = *pixU8++; // R
				}
				else
				{
					int red	  = *pixU8++;
					int green = *pixU8++;
					int blue  = *pixU8++;

					*b++ = blue;
					*b++ = green;
					*b++ = red;
				}
			}
			pixU8 -= 2 * 3 * w;
		}
		break;
	}
	case 32:
	{
		const u32 *pixU32 = (const u32 *)(pix + 4 * (w + 1) * (h));
		for (int y = 0; y < sizeY; y++)
		{
			for (int x = 0; x < sizeX; x++)
			{
				u32 v = *pixU32++;

				*b++ = ((v >> systemBlueShift) & 0x001f) << 3; // B
				*b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
				*b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
			}
			pixU32++;
			pixU32 -= 2 * (w + 1);
		}
		break;
	}
	}
}

// Reports a failed image save.  No WIC at all (XP) and a missing or broken
// codec each get their own message, because the fix is different; everything
// else is a plain failure and the reason is in the trace either way.
void utilReportImageSaveFailure(const char *fileName)
{
	if (!imageWriteAvailable())
	{
		systemMessage(MSG_ERROR_NO_WIC,
		              N_("Image export needs the Windows Imaging Component, which this "
		                 "version of Windows does not have.\n\n"
		                 "Windows XP has no WIC, so screenshots cannot be saved there. "
		                 "They work on Windows Vista and later."));
		return;
	}

	if (imageWriteEncoderUnavailable())
	{
		systemMessage(MSG_ERROR_ENCODER_MISSING,
		              N_("Image encoder for %s is not installed.\n\n"
		                 "Windows could not start the codec for this format. Install the "
		                 "matching Windows image codec (the HEIF and JPEG XL ones are "
		                 "Microsoft Store extensions), then try again.\n\n"
		                 "See vba-trace.log for details."),
		              imageCaptureExtension());
		return;
	}

	systemMessage(MSG_ERROR_SAVING_IMAGE,
	              N_("Error saving image %s\n\nSee vba-trace.log for details."),
	              fileName);
}

bool utilWriteImageFile(const char *fileName, int w, int h, const u8 *pix)
{
	if (!utilWriteImageFromDisplay(fileName, w, h, pix,
	                               systemColorDepth, systemRedShift,
	                               systemGreenShift, systemBlueShift))
	{
		utilReportImageSaveFailure(fileName);
		return false;
	}

	return true;
}

static int utilReadInt2(FILE *f)
{
	int res = 0;
	int c	= fgetc(f);
	if (c == EOF)
		return -1;
	res = c;
	c	= fgetc(f);
	if (c == EOF)
		return -1;
	return c + (res << 8);
}

static int utilReadInt3(FILE *f)
{
	int res = 0;
	int c	= fgetc(f);
	if (c == EOF)
		return -1;
	res = c;
	c	= fgetc(f);
	if (c == EOF)
		return -1;
	res = c + (res << 8);
	c	= fgetc(f);
	if (c == EOF)
		return -1;
	return c + (res << 8);
}

void utilApplyIPS(const char *ips, u8 * *r, int *s)
{
	// from the IPS spec at http://zerosoft.zophar.net/ips.htm
	FILE *f = fopen(ips, "rb");
	if (!f)
		return;
	u8 *rom	 = *r;
	int size = *s;
	if (fgetc(f) == 'P' &&
	    fgetc(f) == 'A' &&
	    fgetc(f) == 'T' &&
	    fgetc(f) == 'C' &&
	    fgetc(f) == 'H')
	{
		int b;
		int offset;
		int len;
		for (;; )
		{
			// read offset
			offset = utilReadInt3(f);
			// if offset == EOF, end of patch
			if (offset == 0x454f46)
				break;
			// read length
			len = utilReadInt2(f);
			if (!len)
			{
				// len == 0, RLE block
				len = utilReadInt2(f);
				// byte to fill
				int c = fgetc(f);
				if (c == -1)
					break;
				b = (u8)c;
			}
			else
				b = -1;
			// check if we need to reallocate our ROM
			if ((offset + len) >= size)
			{
				size *= 2;
				void *tmp = realloc(rom, size);
				if (!tmp) free(rom);	// crash is better than a security hole
				rom	  = (u8 *)tmp;
				*r	  = rom;
				*s	  = size;
			}
			if (b == -1)
			{
				// normal block, just read the data
				if (fread(&rom[offset], 1, len, f) != (size_t)len)
					break;
			}
			else
			{
				// fill the region with the given byte
				while (len--)
				{
					rom[offset++] = b;
				}
			}
		}
	}
	// close the file
	fclose(f);
}

extern bool8 cpuIsMultiBoot;

bool utilIsGBAImage(const char *file)
{
	cpuIsMultiBoot = false;
	if (strlen(file) > 4)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".gba") == 0)
				return true;
			if (_stricmp(p, ".agb") == 0)
				return true;
			if (_stricmp(p, ".bin") == 0)
				return true;
			if (_stricmp(p, ".elf") == 0)
				return true;
			if (_stricmp(p, ".mb") == 0)
			{
				cpuIsMultiBoot = true;
				return true;
			}
		}
	}

	return false;
}

bool utilIsGBImage(const char *file)
{
	if (strlen(file) > 4)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".gb") == 0)
				return true;
			if (_stricmp(p, ".gbc") == 0)
				return true;
			if (_stricmp(p, ".cgb") == 0)
				return true;
			if (_stricmp(p, ".sgb") == 0)
				return true;
		}
	}

	return false;
}

bool utilIsGBABios(const char *file)
{
	if (strlen(file) > 4)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".gba") == 0)
				return true;
			if (_stricmp(p, ".agb") == 0)
				return true;
			if (_stricmp(p, ".bin") == 0)
				return true;
			if (_stricmp(p, ".bios") == 0)
				return true;
			if (_stricmp(p, ".rom") == 0)
				return true;
		}
	}

	return false;
}

bool utilIsGBBios(const char *file)
{
	if (strlen(file) > 4)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".gb") == 0)
				return true;
			if (_stricmp(p, ".bin") == 0)
				return true;
			if (_stricmp(p, ".bios") == 0)
				return true;
			if (_stricmp(p, ".rom") == 0)
				return true;
		}
	}

	return false;
}

bool utilIsELF(const char *file)
{
	if (strlen(file) > 4)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".elf") == 0)
				return true;
		}
	}
	return false;
}

bool utilIsZipFile(const char *file)
{
	if (strlen(file) > 4)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".zip") == 0)
				return true;
		}
	}

	return false;
}

#if 0
bool utilIsRarFile(const char *file)
{
	if (strlen(file) > 4)
	{
		char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".rar") == 0)
				return true;
		}
	}

	return false;
}

#endif

bool utilIsGzipFile(const char *file)
{
	if (strlen(file) > 3)
	{
		const char *p = strrchr(file, '.');

		if (p != NULL)
		{
			if (_stricmp(p, ".gz") == 0)
				return true;
			if (_stricmp(p, ".z") == 0)
				return true;
		}
	}

	return false;
}

void utilGetBaseName(const char *file, char *buffer)
{
	strcpy(buffer, file);

	if (utilIsGzipFile(file))
	{
		char *p = strrchr(buffer, '.');

		if (p)
			*p = 0;
	}
}

IMAGE_TYPE utilFindType(const char *file)
{
	char buffer[2048];

	if (utilIsZipFile(file))
	{
		ZipArchive zip(file);

		if (!zip.IsValid())
		{
			systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), file);
			return IMAGE_UNKNOWN;
		}

		IMAGE_TYPE found = IMAGE_UNKNOWN;

		for (int i = 0; i < zip.GetEntryCount(); i++)
		{
			ZipEntryInfo info;

			if (!zip.GetEntryInfo(i, &info))
			{
				systemMessage(MSG_BAD_ZIP_FILE, N_("Bad ZIP file %s"), file);
				return IMAGE_UNKNOWN;
			}

			if (utilIsGBAImage(info.name))
			{
				found = IMAGE_GBA;
				break;
			}

			if (utilIsGBImage(info.name))
			{
				found = IMAGE_GB;
				break;
			}
		}

		if (found == IMAGE_UNKNOWN)
		{
			systemMessage(MSG_NO_IMAGE_ON_ZIP,
			              N_("No image found on ZIP file %s"), file);
			return found;
		}
		return found;
#if 0
	}
	else if (utilIsRarFile(file))
	{
		IMAGE_TYPE found = IMAGE_UNKNOWN;

		ArchiveList_struct *rarList = NULL;
		if (urarlib_list((void *)file, (ArchiveList_struct *)&rarList))
		{
			ArchiveList_struct *p = rarList;

			while (p)
			{
				if (utilIsGBAImage(p->item.Name))
				{
					found = IMAGE_GBA;
					break;
				}

				if (utilIsGBImage(p->item.Name))
				{
					found = IMAGE_GB;
					break;
				}
				p = p->next;
			}

			urarlib_freelist(rarList);
		}
		return found;
#endif
	}
	else
	{
		if (utilIsGzipFile(file))
			utilGetBaseName(file, buffer);
		else
			strcpy(buffer, file);

		if (utilIsGBAImage(buffer))
			return IMAGE_GBA;
		if (utilIsGBImage(buffer))
			return IMAGE_GB;
	}
	return IMAGE_UNKNOWN;
}

static int utilGetSize(int size)
{
	int res = 1;
	while (res < size)
		res <<= 1;
	return res;
}

static u8 *utilLoadFromZip(const char *file,
                           bool (*accept)(const char *),
                           u8 *data,
                           int &size)
{
	ZipArchive zip(file);

	if (!zip.IsValid())
	{
		systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), file);
		return NULL;
	}

	int found = -1;
	ZipEntryInfo info;

	for (int i = 0; i < zip.GetEntryCount(); i++)
	{
		if (!zip.GetEntryInfo(i, &info))
		{
			systemMessage(MSG_BAD_ZIP_FILE, N_("Bad ZIP file %s"), file);
			return NULL;
		}

		if (accept(info.name))
		{
			found = i;
			break;
		}
	}

	if (found < 0)
	{
		systemMessage(MSG_NO_IMAGE_ON_ZIP,
		              N_("No image found on ZIP file %s"), file);
		return NULL;
	}

	int fileSize = (int)info.uncompressedSize;
	if (size == 0)
		size = fileSize;

	u8 *image = data;

	if (image == NULL)
	{
		image = (u8 *)malloc(utilGetSize(size));
		if (image == NULL)
		{
			systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
			              "data");
			return NULL;
		}
		size = fileSize;
	}
	int read = fileSize <= size ? fileSize : size;
	int r = zip.ExtractEntry(found, image, (unsigned int)read);

	if (r != read)
	{
		systemMessage(MSG_ERROR_READING_IMAGE,
		              N_("Error reading image %s"), info.name);
		if (data == NULL)
			free(image);
		return NULL;
	}

	size = fileSize;

	return image;
}

static u8 *utilLoadGzipFile(const char *file,
                            bool (*accept)(const char *),
                            u8 *data,
                            int &size)
{
	FILE *f = fopen(file, "rb");

	if (f == NULL)
	{
		systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), file);
		return NULL;
	}

	fseek(f, -4, SEEK_END);
	int fileSize = fgetc(f) | (fgetc(f) << 8) | (fgetc(f) << 16) | (fgetc(f) << 24);
	fclose(f);
	if (size == 0)
		size = fileSize;

	gzFile gz = gzopen(file, "rb");

	if (gz == NULL)
	{
		// should not happen, but who knows?
		systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), file);
		return NULL;
	}

	u8 *image = data;

	if (image == NULL)
	{
		image = (u8 *)malloc(utilGetSize(size));
		if (image == NULL)
		{
			systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
			              "data");
			return NULL;
		}
		size = fileSize;
	}
	int read = fileSize <= size ? fileSize : size;
	int r	 = gzread(gz, image, read);
	gzclose(gz);

	if (r != (int)read)
	{
		systemMessage(MSG_ERROR_READING_IMAGE,
		              N_("Error reading image %s"), file);
		if (data == NULL)
			free(image);
		return NULL;
	}

	size = fileSize;

	return image;
}

#if 0
static u8 *utilLoadRarFile(const char *file,
                           bool (*accept)(const char *),
                           u8 *data,
                           int &size)
{
	char buffer[2048];

	ArchiveList_struct *rarList = NULL;
	if (urarlib_list((void *)file, (ArchiveList_struct *)&rarList))
	{
		ArchiveList_struct *p = rarList;

		bool found = false;
		while (p)
		{
			if (accept(p->item.Name))
			{
				strcpy(buffer, p->item.Name);
				found = true;
				break;
			}
			p = p->next;
		}
		if (found)
		{
			void *memory		= NULL;
			unsigned long lsize = 0;
			size = p->item.UnpSize;
			int r = urarlib_get((void *)&memory, &lsize, buffer, (void *)file, "");
			if (!r)
			{
				systemMessage(MSG_ERROR_READING_IMAGE,
				              N_("Error reading image %s"), buffer);
				urarlib_freelist(rarList);
				return NULL;
			}
			u8 *image = (u8 *)memory;
			if (data != NULL)
			{
				memcpy(image, data, size);
			}
			urarlib_freelist(rarList);
			return image;
		}
		systemMessage(MSG_NO_IMAGE_ON_ZIP,
		              N_("No image found on RAR file %s"), file);
		urarlib_freelist(rarList);
		return NULL;
	}
	// nothing found
	return NULL;
}

#endif

// the caller is responsible for caling free(return value) to release the memory
u8 *utilLoad(const char *file,
             bool (*accept)(const char *),
             u8 *data,
             int &size)
{
	if (utilIsZipFile(file))
	{
		return utilLoadFromZip(file, accept, data, size);
	}
	if (utilIsGzipFile(file))
	{
		return utilLoadGzipFile(file, accept, data, size);
	}
#if 0
	if (utilIsRarFile(file))
	{
		return utilLoadRarFile(file, accept, data, size);
	}
#endif

	u8 *image = data;

	FILE *f = fopen(file, "rb");

	if (!f)
	{
		systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), file);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size == 0)
		size = fileSize;

	if (image == NULL)
	{
		image = (u8 *)malloc(utilGetSize(size));
		if (image == NULL)
		{
			systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
			              "data");
			fclose(f);
			return NULL;
		}
		size = fileSize;
	}
	int read = fileSize <= size ? fileSize : size;
	int r	 = (int)fread(image, 1, read, f);
	fclose(f);

	if (r != (int)read)
	{
		systemMessage(MSG_ERROR_READING_IMAGE,
		              N_("Error reading image %s"), file);
		if (data == NULL)
			free(image);
		return NULL;
	}

	size = fileSize;

	return image;
}

void utilWriteInt(gzFile gzFile, int32 i)
{
	utilGzWrite(gzFile, &i, sizeof(int32));
}

int32 utilReadInt(gzFile gzFile)
{
	int32 i = 0;
	utilGzRead(gzFile, &i, sizeof(int32));
	return i;
}

void utilReadData(gzFile gzFile, variable_desc *data)
{
	while (data->address)
	{
		utilGzRead(gzFile, data->address, data->size);
		data++;
	}
}

void utilReadDataSkip(gzFile gzFile, variable_desc *data)
{
	while (data->address)
	{
		utilGzSeek(gzFile, data->size, SEEK_CUR);
		data++;
	}
}

void utilWriteData(gzFile gzFile, variable_desc *data)
{
	while (data->address)
	{
		utilGzWrite(gzFile, data->address, data->size);
		data++;
	}
}

gzFile utilGzOpen(const char *file, const char *mode)
{
	utilGzWriteFunc = gzWrite;
	utilGzReadFunc	= gzread;
	utilGzCloseFunc = gzclose;
	utilGzSeekFunc	= gzseek;
	utilGzTellFunc	= gztell;

	return gzopen(file, mode);
}

gzFile utilGzReopen(int id, const char *mode)
{
	utilGzWriteFunc = gzWrite;
	utilGzReadFunc	= gzread;
	utilGzCloseFunc = gzclose;
	utilGzSeekFunc	= gzseek;
	utilGzTellFunc	= gztell;

	return gzdopen(id, mode);
}

gzFile utilMemGzOpen(char *memory, int available, char *mode)
{
	utilGzWriteFunc = memgzwrite;
	utilGzReadFunc	= memgzread;
	utilGzCloseFunc = memgzclose;
	utilGzSeekFunc	= NULL;	// FIXME: not implemented...
	utilGzTellFunc	= memtell;

	return memgzopen(memory, available, mode);
}

int utilGzWrite(gzFile file, voidp buffer, unsigned int len)
{
	return utilGzWriteFunc(file, buffer, len);
}

int utilGzRead(gzFile file, voidp buffer, unsigned int len)
{
	return utilGzReadFunc(file, buffer, len);
}

int utilGzClose(gzFile file)
{
	return utilGzCloseFunc(file);
}

z_off_t utilGzSeek(gzFile file, z_off_t offset, int whence)
{
	return utilGzSeekFunc(file, offset, whence);
}

z_off_t utilGzTell(gzFile file)
{
	return utilGzTellFunc(file);
}

void utilGBAFindSave(const u8 *data, const int size)
{
	u32 *p		   = (u32 *)data;
	u32 *end	   = (u32 *)(data + size);
	int	 saveType  = 0;
	int	 flashSize = 0x10000;
	bool rtcFound  = false;

	while (p  < end)
	{
		u32 d = READ32LE(p);

		if (d == 0x52504545)
		{
			if (memcmp(p, "EEPROM_", 7) == 0)
			{
				if (saveType == 0)
					saveType = 1;
			}
		}
		else if (d == 0x4D415253)
		{
			if (memcmp(p, "SRAM_", 5) == 0)
			{
				if (saveType == 0)
					saveType = 2;
			}
		}
		else if (d == 0x53414C46)
		{
			if (memcmp(p, "FLASH1M_", 8) == 0)
			{
				if (saveType == 0)
				{
					saveType  = 3;
					flashSize = 0x20000;
				}
			}
			else if (memcmp(p, "FLASH", 5) == 0)
			{
				if (saveType == 0)
				{
					saveType  = 3;
					flashSize = 0x10000;
				}
			}
		}
		else if (d == 0x52494953)
		{
			if (memcmp(p, "SIIRTC_V", 8) == 0)
				rtcFound = true;
		}
		p++;
	}
	// if no matches found, then set it to NONE
	if (saveType == 0)
	{
		saveType = 5;
	}
	rtcEnable(rtcFound);
	cpuSaveType = saveType;
	flashSetSize(flashSize);
}

void utilUpdateSystemColorMaps()
{
	switch (systemColorDepth)
	{
	case 16:
	{
		for (int i = 0; i < 0x10000; i++)
		{
			systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
			                      (((i & 0x3e0) >> 5) << systemGreenShift) |
			                      (((i & 0x7c00) >> 10) << systemBlueShift);
		}
		break;
	}
	case 24:
	case 32:
	{
		for (int i = 0; i < 0x10000; i++)
		{
			systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
			                      (((i & 0x3e0) >> 5) << systemGreenShift) |
			                      (((i & 0x7c00) >> 10) << systemBlueShift);
		}
		break;
	}
	}
}

//// BIOS stuff
// systemType uses the same enum values as gbEmulatorType does

bool utilLoadBIOS(u8 *bios, const char *biosFileName, int systemType)
{
	if (bios == NULL || strlen(biosFileName) == 0)
		return false;

	if (systemType == 4)
	{
		int biosSize = 0x4000;
		if (utilLoad(biosFileName, utilIsGBABios, bios, biosSize))
		{
			if (biosSize == 0x4000)
				return true;
		}
		systemMessage(MSG_INVALID_BIOS_FILE_SIZE, N_("Invalid GBA BIOS file!"));
	}
	else
	{
		int biosSize = 0x100;
		if (utilLoad(biosFileName, utilIsGBBios, bios, biosSize))
		{
			if (biosSize == 0x100)
				return true;
		}
		systemMessage(MSG_INVALID_BIOS_FILE_SIZE, N_("Invalid BOOTROM file"));
	}

	return false;
}

bool utilCheckBIOS(const char *biosFileName, int systemType)
{
	if (strlen(biosFileName) == 0)
		return false;

	u8 * tempBIOS = (u8 *)malloc(systemType == 4 ? 0x4000 : 0x100);
	bool result	  = utilLoadBIOS(tempBIOS, biosFileName, systemType);
	free(tempBIOS);

	return result;
}

#if 0
// returns the checksum of the BIOS that will be loaded after the next restart
u16 utilCalcBIOSChecksum(const u8 *bios, int systemType)
{
	u32	biosChecksum = 0;
	if (bios)
	{
		int biosSize	= (systemType == 4 ? 0x4000 : 0x100);
		const u16 *data = reinterpret_cast<const u16 *>(bios);
		for (int i = biosSize; i > 0; i -= 2)
			biosChecksum += *data++;
	}

	while ((biosChecksum >> 16) & 0xFFFF)
		biosChecksum = (biosChecksum & 0xFFFF) + ((biosChecksum >> 16) & 0xFFFF);

	return biosChecksum & 0xFFFF;
}
#else
// returns the checksum of the BIOS that will be loaded after the next restart
u16 utilCalcBIOSChecksum(const u8 *bios, int systemType)
{
	u32	biosChecksum = 0;
	if (bios)
	{
		int biosSize	= (systemType == 4 ? 0x4000 : 0x100);
		const u32 *data = reinterpret_cast<const u32 *>(bios);
		for (int i = biosSize; i > 0; i -= 4)
			biosChecksum += *data++;
	}

	return biosChecksum & 0xFFFF;
}
#endif

// returns the checksum of the BIOS file
u16 utilCalcBIOSFileChecksum(const char *biosFileName, int systemType)
{
	if (strlen(biosFileName) == 0)
		return 0;

	u16		  biosChecksum = 0;
	const int biosSize	   = (systemType == 4 ? 0x4000 : 0x100);
	u8 *	  tempBIOS	   = (u8 *)malloc(biosSize);
	bool	  hasBIOS	   = utilLoadBIOS(tempBIOS, biosFileName, systemType);
	if (hasBIOS)
	{
		biosChecksum = utilCalcBIOSChecksum(tempBIOS, systemType);
	}
	free(tempBIOS);

	return biosChecksum;
}

