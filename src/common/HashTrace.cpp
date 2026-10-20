#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "zlib.h"

#include "HashTrace.h"
#include "System.h"
#include "SystemGlobals.h"
#include "Util.h" // IMAGE_GBA

#include "../gba/GBAGlobals.h" // reg / internalRAM / workRAM / vram / paletteRAM / oam / ioMem

// Same size as REWIND_SIZE: the project already treats this as large enough to
// hold a complete state.
#define HASH_TRACE_STATE_SIZE 400000

// GBA region sizes, taken from CPUWriteStateToStream in gba/V7|V8/GBA.cpp so
// they match the actual allocations.
#define HT_INTERNAL_RAM 0x08000
#define HT_WORK_RAM		0x40000
#define HT_VRAM			0x20000
#define HT_PALETTE_RAM	0x00400
#define HT_OAM			0x00400
#define HT_IO_MEM		0x00400

namespace
{
	FILE *s_file		 = NULL;
	char *s_buf			 = NULL;
	unsigned int s_frame = 0;
	unsigned int s_failed = 0;
	bool s_ready		 = false;

	void hashTraceAtExit()
	{
		hashTraceClose();
	}

	unsigned int crcOf(unsigned int crc, const void *data, size_t size)
	{
		if (!data || !size)
			return crc;
		return (unsigned int)crc32(crc, (const Bytef *)data, (uInt)size);
	}

	// "Gameplay memory": the regions that actually determine GBA behaviour.
	//
	// pix (4*241*162) is deliberately excluded. CPUWriteStateToStream serialises
	// pix as well, and pix carries OSD text whose lifetime is measured in wall
	// clock milliseconds (DrawTextMessages), so the same message survives a
	// different number of frames on a fast run and a slow one. That is a dirty
	// framebuffer, not emulation drift - see tests/baseline/README.md.
	unsigned int ramHash()
	{
		if (systemCartridgeType != IMAGE_GBA)
			return 0; // GB mode not covered yet: different region layout

		unsigned int crc = (unsigned int)crc32(0L, Z_NULL, 0);
		crc				 = crcOf(crc, internalRAM, HT_INTERNAL_RAM);
		crc				 = crcOf(crc, workRAM, HT_WORK_RAM);
		crc				 = crcOf(crc, vram, HT_VRAM);
		crc				 = crcOf(crc, paletteRAM, HT_PALETTE_RAM);
		crc				 = crcOf(crc, oam, HT_OAM);
		crc				 = crcOf(crc, ioMem, HT_IO_MEM);
		crc				 = crcOf(crc, reg, sizeof(reg));
		return crc;
	}
}

bool hashTraceEnabled()
{
	return s_file != NULL;
}

void hashTraceEnable(const char *path)
{
	if (s_file || !path || !path[0])
		return;

	s_file = fopen(path, "wb");
	if (!s_file)
	{
		systemMessage(0, "Cannot open hash trace file: %s", path);
		return;
	}

	s_buf = (char *)malloc(HASH_TRACE_STATE_SIZE);
	if (!s_buf)
	{
		fclose(s_file);
		s_file = NULL;
		systemMessage(0, "Cannot allocate %d bytes for the hash trace buffer", HASH_TRACE_STATE_SIZE);
		return;
	}

	fprintf(s_file, "# vba-rr state hash trace v2\n");
	fprintf(s_file, "# state_size=%d\n", HASH_TRACE_STATE_SIZE);
	fprintf(s_file, "# ram = internalRAM+workRAM+vram+paletteRAM+oam+ioMem+reg (no pix/sound/rtc)\n");
	fprintf(s_file, "# full = crc32 over the gzipped emuWriteMemState blob (includes pix: may vary with OSD timing)\n");
	fprintf(s_file, "frame,ram,full,ok\n");

	s_frame	 = 0;
	s_failed = 0;
	s_ready	 = true;

	atexit(hashTraceAtExit);
}

void hashTraceFrameBoundary()
{
	if (!s_ready)
		return;

	// Zero first: emuWriteMemState does not report how many bytes it wrote, so
	// the unused tail has to be deterministic.
	memset(s_buf, 0, HASH_TRACE_STATE_SIZE);

	bool ok = false;
	if (theEmulator.emuWriteMemState)
		ok = theEmulator.emuWriteMemState(s_buf, HASH_TRACE_STATE_SIZE);

	unsigned int ram  = ramHash();
	unsigned int full = (unsigned int)crc32(0L, (const Bytef *)s_buf, HASH_TRACE_STATE_SIZE);

	fprintf(s_file, "%u,%08x,%08x,%d\n", s_frame, ram, full, ok ? 1 : 0);
	if (!ok)
		++s_failed;

	++s_frame;

	// Flush about every 10 seconds so a crash late in a long replay does not
	// lose the whole trace.
	if ((s_frame % 600) == 0)
		fflush(s_file);
}

void hashTraceClose()
{
	if (!s_ready)
		return;

	s_ready = false;

	if (s_file)
	{
		fprintf(s_file, "# end frames=%u failed=%u\n", s_frame, s_failed);
		fflush(s_file);
		fclose(s_file);
		s_file = NULL;
	}

	if (s_buf)
	{
		free(s_buf);
		s_buf = NULL;
	}
}
