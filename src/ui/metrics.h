#ifndef UI_METRICS_H
#define UI_METRICS_H

// Backing area
#define XT_UI_PLANE 0
#define XT_UI_AREA_X 0
#define XT_UI_AREA_Y 0
#define XT_UI_AREA_W 512
#define XT_UI_AREA_H 64

// Channel labels
#define XT_UI_CHANLABEL_W 64
#define XT_UI_CHANLABEL_H 8
#define XT_UI_CHANLABEL_Y (XT_UI_AREA_Y + XT_UI_AREA_H - XT_UI_CHANLABEL_H)

// Margins
#define XT_UI_MARGIN_SIDE 8
#define XT_UI_MARGIN_TOP 4


// Section title bars
#define XT_UI_TITLEBAR_TEXT_OFFS_X 2
#define XT_UI_TITLEBAR_TEXT_OFFS_Y 0
#define XT_UI_TITLEBAR_LINE_H 8
#define XT_UI_TITLEBAR_CONTENT_OFFS_Y 12

// Left side elements.
#define XT_UI_LOGO_X 8
#define XT_UI_LOGO_Y 4

// Middle.
#define XT_UI_INSTRUMENT_LIST_X (XT_UI_MARGIN_SIDE + 136)
#define XT_UI_INSTRUMENT_LIST_Y XT_UI_MARGIN_TOP

// Right.
#define XT_UI_ARRANGEMENT_X (XT_UI_MARGIN_SIDE + 256)
#define XT_UI_ARRANGEMENT_TABLE_OFFS_X 24
#define XT_UI_ARRANGEMENT_Y XT_UI_MARGIN_TOP
#define XT_UI_ARRANGEMENT_TABLE_OFFS_Y XT_UI_TITLEBAR_CONTENT_OFFS_Y

#define XT_UI_REGISTER_DATA_X XT_UI_ARRANGEMENT_X
#define XT_UI_REGISTER_DATA_Y XT_UI_ARRANGEMENT_Y

// Spacing for number arrays (register data, ararnge, etc)
#define XT_UI_COL_SPACING 16
#define XT_UI_ROW_SPACING 8


#endif  // UI_METRICS_H
