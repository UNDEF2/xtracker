#include "xt/mod.h"

#include "core/macro.h"

static const int8_t mod_table_square[] =
{
	-8, -8, -8, -8,
	8, 8, 8, 8,
};

static inline void tick_internal(volatile XtMod *mod)
{
	if (mod->accumulator >= 0xF - mod->speed)
	{
		mod->accumulator = 0;
		if (mod->index >= ARRAYSIZE(mod_table_square)) mod->index = 0;
		else mod->index++;
	}
	else mod->accumulator++;
}

void xt_mod_tick(volatile XtMod *mod)
{
	// Repeated internal ticks effectively multiply the speed.
	tick_internal(mod);

	// TODO: Wave selection.
	mod->value = mod_table_square[mod->index] * mod->intensity;
}
