#ifndef UTIL_CGPRINT_SUB
#define UTIL_CGPRINT_SUB

#include <stdint.h>

// Prints string str to dest_addr, using tile data file cgdat.
void cgprint_string_sub(const uint8_t *cgdat, volatile uint16_t *dest_addr,
                        const char *str, uint16_t attr);

// Prints string str to dest_addr, using tile data file cgdat.
// colors should contain the FG color in the low word and the BG color in the upper word.
void cgprint_string_noalpha_sub(const uint8_t *cgdat, volatile uint16_t *dest_addr,
                                const char *str, uint32_t colors);

// Subroutine for blitting w x h tiles to the CG plane. These tiles are 256
// color linearly-packed tiles, and they are drawn as-is.
void cgprint_8x8_sub(const uint8_t *src_addr, volatile uint16_t *dest_addr,
                     uint16_t w, uint16_t h);

// Routine to fill one line with pixel value `val`.
void cgprint_line_fill_sub(volatile uint16_t *dest_addr, uint8_t val,
                           uint16_t w);

#endif  // UTIL_CGPRINT_SUB
