#include "xt/mod.h"

#define MOD_TABLE_SIZE 32

static const int8_t mod_table_square[MOD_TABLE_SIZE] =
{
	-127, -127, -127, -127, -127, -127, -127, -127,
	-127, -127, -127, -127, -127, -127, -127, -127,
	127, 127, 127, 127, 127, 127, 127, 127,
	127, 127, 127, 127, 127, 127, 127, 127,
};

static inline void tick_internal(volatile XtMod *mod)
{
	if (mod->accumulator >= 0xF - mod->speed)
	{
		mod->accumulator = 0;
		if (mod->index >= MOD_TABLE_SIZE) mod->index = 0;
		else mod->index++;
	}
	else mod->accumulator++;
}

void xt_mod_tick(volatile XtMod *mod)
{
	// Repeated internal ticks effectively multiply the speed.
	tick_internal(mod);
	tick_internal(mod);
	tick_internal(mod);

	// TODO: Wave selection.
	mod->value = mod_table_square[mod->index];
}
