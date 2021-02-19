#ifndef XT_PALETTE_H
#define XT_PALETTE_H

typedef enum XtPaletteColor
{
	XT_PAL_TRANSPARENT = 0,
	XT_PAL_UI_FG = 1,
	XT_PAL_UI_BG = 2,
	XT_PAL_UI_HIGHLIGHT_FG = 3,
	XT_PAL_UI_HIGHLIGHT_BG = 4,
	XT_PAL_UI_BORDER = 5,
	XT_PAL_UI_LABEL = 6,
} XtPaletteColor;

void xt_palette_init(void);

#endif  // XT_PALETTE_H
