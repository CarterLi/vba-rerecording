#ifndef MEMGZIO_H
#define MEMGZIO_H

/* gzip streams inside a caller-supplied memory buffer.
 *
 * The buffer starts with an 8-byte header ("VBA " plus the compressed
 * length) followed by a gzip stream; memgzopen() reads and writes that
 * container without touching the file system.  The layout is unchanged
 * from the original memgzio.c, so old savestates still load.
 *
 * The entry points mirror zlib's gzFile functions, which lets
 * utilGzOpen()/utilGzRead()/utilGzWrite() drive either kind of stream.
 */

#include <zlib.h>

gzFile ZEXPORT memgzopen(char *memory, int available, const char *mode);
int ZEXPORT memgzread(gzFile file, voidp buf, unsigned len);
int ZEXPORT memgzwrite(gzFile file, const voidp buf, unsigned len);
int ZEXPORT memgzclose(gzFile file);
z_off_t ZEXPORT memtell(gzFile file);

#endif // MEMGZIO_H
