#ifndef UTIL_CGPRINT_SUB
#define UTIL_CGPRINT_SUB

#include <stdint.h>

// Subroutine for printing a monochromatic 6x8 area of an 8x8 fonrt tile.
// Accepts an attribute argument to set the color and set/disable alpha.
void cgprint_6x8_sub(const uint8_t *src_addr, volatile uint16_t *dest_addr,
                     uint16_t attr);

// Subroutine for blitting w x h tiles to the CG plane. These tiles are 256
// color linearly-packed tiles, and they are drawn as-is.
void cgprint_8x8_sub(const uint8_t *src_addr, volatile uint16_t *dest_addr,
                     uint16_t w, uint16_t h);

#endif  // UTIL_CGPRINT_SUB
