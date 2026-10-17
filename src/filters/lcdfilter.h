#ifndef VBA_FILTERS_LCDFILTER_H
#define VBA_FILTERS_LCDFILTER_H

// LCD color-correction filters ported from VBA-M
// (src/components/filters_agb and src/components/filters_cgb), which adapt the
// "Color Mangler" shaders by Pokefan531 / hunterk (public domain).
//
// The filters make the emulated panel look like a real GBA/GB LCD by applying a
// per-channel gamma plus a 3x3 color matrix to the palette translation tables
// (systemColorMap16/32).  Because both the GBA and the GB/GBC renderers map
// their output through those tables, rebuilding them is enough to color-correct
// the whole frame, independent of the chosen pixel filter or render method.

// Panel variants.  These are stored in the config, so only append.
enum LcdGbaVariant {
	kLcdGbaGba = 0,
	kLcdGbaGbaSpBacklit,
	kLcdGbaMicro,
	kLcdGbaDs,
	kLcdGbaDsLite,
	kLcdGbaNso,
	kLcdGbaVariantCount
};

enum LcdGbcVariant {
	kLcdGbcCgb = 0,
	kLcdGbcNso,
	kLcdGbcVariantCount
};

// Sentinel for "no LCD color filter".
#define LCD_FILTER_OFF (-1)

// Rebuilds systemColorMap16/32 for the current color depth and shifts.
//
// gbaVariant / gbcVariant are the selected panel variants, or LCD_FILTER_OFF.
// isGba selects which family applies to the running system.  colorMode picks
// the colour space (0 = sRGB, 1 = DCI, 2 = Rec2020).  When the relevant variant
// is LCD_FILTER_OFF the tables are rebuilt without any correction.
void lcdFilterUpdateSystemColorMaps(int gbaVariant, int gbcVariant,
                                    int colorMode, bool isGba);

#endif // VBA_FILTERS_LCDFILTER_H
