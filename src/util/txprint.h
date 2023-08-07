#ifndef UTIL_TXPRINT_H
#define UTIL_TXPRINT_H

#include <stdint.h>

void txprint_init(void);

// s has a max length of 256, four times the screen's width.
void txprintf(int16_t x, int16_t y, int16_t color, const char *s, ...);

#endif  // UTIL_TXPRINT_H
