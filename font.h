#ifndef FONTS_H
#define FONTS_H

#define SMALL_FONT_WIDTH  3
#define SMALL_FONT_HEIGHT 5
#define LARGE_FONT_WIDTH  12
#define LARGE_FONT_HEIGHT 10

typedef const char SmallFontChar[SMALL_FONT_HEIGHT][SMALL_FONT_WIDTH+1];
typedef const char LargeFontChar[LARGE_FONT_HEIGHT][LARGE_FONT_WIDTH+1];

extern SmallFontChar g_font_small[256];
extern LargeFontChar g_font_large[256];

#endif
