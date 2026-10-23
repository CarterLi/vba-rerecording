#ifndef VBA_TEXT_H
#define VBA_TEXT_H

#pragma once

extern void drawText(u8 *, int, int, int, const char *, const char*cl = NULL, int scalePercent = 100);
extern void drawTextTransp(u8 *, int, int, int, const char *, const char*cl = NULL, int scalePercent = 100);

// Height of one row of the built-in bitmap font at the given scale, outline
// included.  100 percent is the historical 8x8 glyph plus its one pixel
// outline, i.e. 10 rows.
extern int drawTextLineHeight(int scalePercent = 100);

// The palette entry used by drawText()/drawTextTransp(), exposed for the
// win32 system-font renderer so both paths pick exactly the same colours.
extern void drawTextGetColors(int colorNum, int &lo, int &hi, int &out);

extern bool outlinedText, transparentText;
extern int  textColor, textMethod;

#endif // VBA_TEXT_H
