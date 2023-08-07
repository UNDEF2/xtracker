#include "util/txprint.h"

#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <iocs.h>

static char s_buffer[256];

void txprint_init(void)
{
	_iocs_b_curoff();  // Turn off cursor
	_dos_c_window(64, 16);
	_dos_c_cls_al();
}

void txprintf(int16_t x, int16_t y, int16_t color, const char *s, ...)
{
	va_list va;
	va_start(va, s);
	vsnprintf(s_buffer, ARRAYSIZE(s_buffer), s, va);
	va_end(va);

	_dos_c_locate(x, y);
	_dos_c_color(color);
	_dos_c_print(s_buffer);

}

