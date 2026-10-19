#ifndef VBA_IMAGE_WRITE_H
#define VBA_IMAGE_WRITE_H

// Image writing through the Windows Imaging Component (WIC).
//
// This replaces the bundled libpng writer and the hand-rolled BMP writers.  It
// ships with Windows, so there is no third party dependency to maintain, and it
// also gives JPEG/GIF/TIFF and whatever else the machine can encode.
//
// The format list is not hardcoded: it is the set of encoders WIC reports at
// run time, so a screenshot can be saved in anything WIC can write.

// One container a WIC encoder can write.
struct ImageWriteFormatInfo
{
	char name[64];         // localized friendly name, e.g. "PNG Encoder"
	char extensions[256];  // comma separated, e.g. ".jpeg,.jpe,.jpg"
};

// Enumerates the WIC encoders, building the list on first use and caching it.
// Returns the number of formats, which is all of them when `out` is NULL, and
// fills at most `max` entries otherwise.  The default (PNG) comes first.
int imageWriteFormats(ImageWriteFormatInfo *out, int max);

// False when this Windows has no WIC at all, which is the case on XP: it was
// introduced in Vista.  The writers are still linked there, they just cannot
// find the factory, so callers grey the capture commands out and say why
// instead of letting a save fail.
bool imageWriteAvailable();

// Index into the enumeration above of the capture format, or -1 when no format
// could be selected (which makes the writers fall back to PNG).
int imageCaptureFormatIndex();

// The extension used to name captures, e.g. ".png".  Always the one belonging
// to the container the writers below use, so a name and its contents agree.
const char *imageCaptureExtension();

// Selects the capture format by any of its extensions (e.g. ".png", ".jpeg").
// Returns false and keeps the current selection if no encoder claims it.
bool imageCaptureSelect(const char *extension);

// Selects the capture format by its index in the enumeration above.  Returns
// false and keeps the current selection when the index is out of range.
bool imageCaptureSelectIndex(int index);

// Writes a loosely-strided, top-down 24-bit BGR image in the capture format.
bool utilWriteImageBGR(const char *fileName, int width, int height,
                       const unsigned char *bgr, int stride);

// Writes the emulator's display buffer in the capture format.  The platform
// pixel layout is 5-bit channels placed at the given bit shifts.  Row 0 of the
// buffer is the filters' over-read border, the LCD image starts one row in;
// 24bpp output is stored without that border.
bool utilWriteImageFromDisplay(const char *fileName, int width, int height,
                               const unsigned char *pixels, int colorDepth,
                               int redShift, int greenShift, int blueShift);

// Diagnostics.
//
// Encoders are picky: GIF only takes indexed 8bpp, the HEIF encoder wants
// planar YCbCr, and so on.  When a write fails the reason is rarely obvious
// from the outside, so every step reports through this callback.  The win32
// front end points it at the trace log; without it the writers stay quiet.
// Callers only say "it failed, look at the log".
typedef void (*ImageWriteLogFunc)(const char *message);
void imageWriteSetLogger(ImageWriteLogFunc logger);

// True when the last write failed because the codec for the chosen format is
// missing or would not start, as opposed to a bad path, a full disk or a pixel
// format the codec refuses.  Callers show the "install the encoder" message in
// that case and the generic one otherwise.
bool imageWriteEncoderUnavailable();

#endif // VBA_IMAGE_WRITE_H
