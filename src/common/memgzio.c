/* memgzio.c - gzip streams inside a caller-supplied memory buffer.
 *
 * The savestate, rewind and frame-search buffers are not plain gzip files:
 * an 8-byte header ("VBA " plus the compressed length) precedes the stream.
 * The original implementation was a copy of zlib's gzio.c from 2002 that
 * parsed the gzip header and trailer by hand.  zlib handles the wrapper
 * itself since 1.2 (windowBits MAX_WBITS + 16), so this file keeps only the
 * container and lets zlib do the stream work.
 *
 * Layout of the buffer:
 *   0  'V' 'B' 'A' ' '
 *   4  int32 compressed length, stored by memgzclose()
 *   8  gzip stream
 *
 * The format is unchanged, so savestates written by older builds still load
 * and the other way round.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memgzio.h"

#define VBA_HEADER_SIZE 8
/* gzip wrapper (16) around the normal 32K deflate window */
#define MEMGZ_WINDOW_BITS (MAX_WBITS + 16)
#define MEMGZ_MEM_LEVEL 8

typedef struct MemGz
{
	char    *memory;
	int      capacity;    /* payload bytes available after the header */
	int      mode;        /* 'r' or 'w' */
	int      transparent; /* payload is stored uncompressed */
	int      eof;
	int      z_err;
	int      consumed;    /* read: payload bytes handed to inflate */
	z_stream strm;
} MemGz;

/* Reads the length field and clamps it to the buffer we were given, so a
 * corrupt or truncated buffer cannot make us read past its end. */
static int memGzPayloadLength(const char *memory, int available)
{
	int length;

	if (memory == NULL || available <= VBA_HEADER_SIZE)
		return -1;

	if (memcmp(memory, "VBA ", 4) != 0)
		return -1;

	memcpy(&length, memory + 4, sizeof(length));

	if (length < 0 || length > available - VBA_HEADER_SIZE)
		length = available - VBA_HEADER_SIZE;

	return length;
}

static void memGzResetStream(z_stream *strm)
{
	strm->zalloc   = Z_NULL;
	strm->zfree    = Z_NULL;
	strm->opaque   = Z_NULL;
	strm->next_in  = Z_NULL;
	strm->avail_in = 0;
	strm->next_out  = Z_NULL;
	strm->avail_out = 0;
}

gzFile ZEXPORT memgzopen(char *memory, int available, const char *mode)
{
	MemGz     *s;
	const char *p;
	int        level    = Z_DEFAULT_COMPRESSION;
	int        strategy = Z_DEFAULT_STRATEGY;

	if (memory == NULL || mode == NULL || available <= VBA_HEADER_SIZE)
		return NULL;

	s = (MemGz *)calloc(1, sizeof(MemGz));
	if (s == NULL)
		return NULL;

	s->memory = memory;
	s->z_err  = Z_OK;

	for (p = mode; *p; p++)
	{
		if (*p == 'r')
			s->mode = 'r';
		else if (*p == 'w' || *p == 'a')
			s->mode = 'w';
		else if (*p >= '0' && *p <= '9')
			level = *p - '0';
		else if (*p == 'f')
			strategy = Z_FILTERED;
		else if (*p == 'h')
			strategy = Z_HUFFMAN_ONLY;
	}

	if (s->mode == 'w')
	{
		memory[0] = 'V';
		memory[1] = 'B';
		memory[2] = 'A';
		memory[3] = ' ';
		memset(memory + 4, 0, 4);

		s->capacity = available - VBA_HEADER_SIZE;
		memGzResetStream(&s->strm);

		if (deflateInit2(&s->strm, level, Z_DEFLATED, MEMGZ_WINDOW_BITS,
		                 MEMGZ_MEM_LEVEL, strategy) != Z_OK)
		{
			free(s);
			return NULL;
		}

		/* Compress straight into the caller's buffer; there is no reason
		 * to bounce the output through a temporary block. */
		s->strm.next_out  = (Bytef *)memory + VBA_HEADER_SIZE;
		s->strm.avail_out = (uInt)s->capacity;

		return (gzFile)s;
	}

	if (s->mode == 'r')
	{
		int length = memGzPayloadLength(memory, available);

		if (length < 0)
		{
			free(s);
			return NULL;
		}

		s->capacity = length;

		if (length >= 2 &&
		    (unsigned char)memory[VBA_HEADER_SIZE] == 0x1f &&
		    (unsigned char)memory[VBA_HEADER_SIZE + 1] == 0x8b)
		{
			memGzResetStream(&s->strm);

			if (inflateInit2(&s->strm, MEMGZ_WINDOW_BITS) != Z_OK)
			{
				free(s);
				return NULL;
			}

			s->strm.next_in  = (Bytef *)memory + VBA_HEADER_SIZE;
			s->strm.avail_in = (uInt)length;
		}
		else
		{
			/* Older builds could store the payload verbatim; keep reading
			 * those instead of rejecting the buffer. */
			s->transparent = 1;
			if (length == 0)
				s->eof = 1;
		}

		return (gzFile)s;
	}

	free(s);
	return NULL;
}

int ZEXPORT memgzread(gzFile file, voidp buf, unsigned len)
{
	MemGz *s = (MemGz *)file;

	if (s == NULL || s->mode != 'r')
		return Z_STREAM_ERROR;

	if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO)
		return -1;

	if (s->eof || s->z_err == Z_STREAM_END)
		return 0;

	if (buf == NULL && len != 0)
		return Z_STREAM_ERROR;

	if (s->transparent)
	{
		unsigned int left = (unsigned int)(s->capacity - s->consumed);
		unsigned int take = len < left ? len : left;

		if (take != 0)
		{
			memcpy(buf, s->memory + VBA_HEADER_SIZE + s->consumed, take);
			s->consumed += (int)take;
		}

		if (s->consumed >= s->capacity)
			s->eof = 1;

		return (int)take;
	}

	s->strm.next_out  = (Bytef *)buf;
	s->strm.avail_out = len;

	while (s->strm.avail_out != 0)
	{
		int ret;

		if (s->strm.avail_in == 0)
		{
			/* Truncated stream: report what was decoded so far, but make the
			 * failure sticky so the caller does not treat a cut savestate as
			 * complete. */
			s->eof   = 1;
			s->z_err = Z_DATA_ERROR;
			break;
		}

		ret = inflate(&s->strm, Z_NO_FLUSH);

		if (ret == Z_STREAM_END)
		{
			/* Accept concatenated members, as the old reader did. */
			if (s->strm.avail_in >= 2 &&
			    s->strm.next_in[0] == 0x1f && s->strm.next_in[1] == 0x8b)
			{
				inflateReset(&s->strm);
				continue;
			}

			s->eof   = 1;
			s->z_err = Z_STREAM_END;
			break;
		}

		if (ret == Z_BUF_ERROR)
		{
			/* Input ran out in the middle of the stream. */
			s->eof   = 1;
			s->z_err = Z_DATA_ERROR;
			break;
		}

		if (ret != Z_OK)
		{
			s->z_err = Z_DATA_ERROR;
			break;
		}
	}

	s->consumed = s->capacity - (int)s->strm.avail_in;

	return (int)(len - s->strm.avail_out);
}

int ZEXPORT memgzwrite(gzFile file, const voidp buf, unsigned len)
{
	MemGz *s = (MemGz *)file;

	if (s == NULL || s->mode != 'w')
		return Z_STREAM_ERROR;

	if (s->z_err != Z_OK)
		return 0;

	if (buf == NULL && len != 0)
		return 0;

	s->strm.next_in  = (Bytef *)buf;
	s->strm.avail_in = len;

	while (s->strm.avail_in != 0)
	{
		int ret;

		if (s->strm.avail_out == 0)
		{
			/* The caller's buffer is full; nothing was lost silently,
			 * memtell() lets CPUWriteMemState() notice. */
			s->z_err = Z_ERRNO;
			break;
		}

		ret = deflate(&s->strm, Z_NO_FLUSH);

		if (ret == Z_BUF_ERROR)
		{
			s->z_err = Z_ERRNO;
			break;
		}

		if (ret != Z_OK)
		{
			s->z_err = (ret == Z_MEM_ERROR) ? Z_MEM_ERROR : Z_STREAM_ERROR;
			break;
		}
	}

	return (int)(len - s->strm.avail_in);
}

int ZEXPORT memgzclose(gzFile file)
{
	MemGz *s = (MemGz *)file;
	int    err = Z_OK;

	if (s == NULL)
		return Z_STREAM_ERROR;

	if (s->mode == 'w')
	{
		int ret;

		do
		{
			if (s->strm.avail_out == 0)
			{
				err = Z_ERRNO; /* the state does not fit in the buffer */
				break;
			}

			ret = deflate(&s->strm, Z_FINISH);

			if (ret != Z_OK && ret != Z_STREAM_END)
			{
				err = ret;
				break;
			}
		}
		while (ret != Z_STREAM_END);

		if (err == Z_OK)
		{
			int length = s->capacity - (int)s->strm.avail_out;
			memcpy(s->memory + 4, &length, sizeof(length));
		}

		deflateEnd(&s->strm);
	}
	else if (!s->transparent)
	{
		inflateEnd(&s->strm);
	}

	if (s->z_err < Z_OK && err == Z_OK)
		err = s->z_err;

	free(s);
	return err;
}

z_off_t ZEXPORT memtell(gzFile file)
{
	MemGz *s = (MemGz *)file;

	if (s == NULL)
		return Z_STREAM_ERROR;

	if (s->mode == 'w')
		return (z_off_t)(s->capacity - (int)s->strm.avail_out);

	return (z_off_t)s->consumed;
}
