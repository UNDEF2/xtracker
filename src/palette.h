#ifndef PALETTE_H
#define PALETTE_H

typedef enum XtPaletteColor
{
	XT_PAL_TRANSPARENT = 0,
	XT_PAL_BACK,
	XT_PAL_MAIN,
	XT_PAL_ACCENT1,
	XT_PAL_ACCENT2,
	XT_PAL_INACTIVE,
} XtPaletteColor;

void xt_palette_init(void);

#endif  // PALETTE_H
