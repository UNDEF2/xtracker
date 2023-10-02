#ifndef UI_SECTION_TITLE_H
#define UI_SECTION_TITLE_H

#include <stdint.h>

// Draws a section header with title and underline chrome, and clears the entire
// specified render area to black.
// s: section title name string
// x: X position (pixels)
// y: Y "
// w: Width (pixels; from top-left including header to right side)
// h: Height "
void ui_section_title_draw(const char *s, int16_t x, int16_t y, int16_t w, int16_t h);


#endif  // UI_SECTION_TITLE_H
