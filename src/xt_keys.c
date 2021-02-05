#include "xt_keys.h"
#include "common.h"

#include <string.h>
#include <iocs.h>



static const XtKeyID key_lookup[] =
{
	[XT_KEY_6] = {0x0, 0x80},
	[XT_KEY_5] = {0x0, 0x40},
	[XT_KEY_4] = {0x0, 0x20},
	[XT_KEY_3] = {0x0, 0x10},
	[XT_KEY_2] = {0x0, 0x08},
	[XT_KEY_1] = {0x0, 0x04},
	[XT_KEY_ESC] = {0x0, 0x02},

	[XT_KEY_BS] = {0x1, 0x80},
	[XT_KEY_PIPE] = {0x1, 0x40},
	[XT_KEY_TILDE] = {0x1, 0x20},
	[XT_KEY_MINUS] = {0x1, 0x10},
	[XT_KEY_0] = {0x1, 0x08},
	[XT_KEY_9] = {0x1, 0x04},
	[XT_KEY_8] = {0x1, 0x02},
	[XT_KEY_7] = {0x1, 0x01},

	[XT_KEY_U] = {0x2, 0x80},
	[XT_KEY_Y] = {0x2, 0x40},
	[XT_KEY_T] = {0x2, 0x20},
	[XT_KEY_R] = {0x2, 0x10},
	[XT_KEY_E] = {0x2, 0x08},
	[XT_KEY_W] = {0x2, 0x04},
	[XT_KEY_Q] = {0x2, 0x02},
	[XT_KEY_TAB] = {0x2, 0x01},

	[XT_KEY_S] = {0x3, 0x80},
	[XT_KEY_A] = {0x3, 0x40},
	[XT_KEY_CR] = {0x3, 0x20},
	[XT_KEY_OPEN_BRACKET] = {0x3, 0x10},
	[XT_KEY_AT] = {0x3, 0x08},
	[XT_KEY_P] = {0x3, 0x04},
	[XT_KEY_O] = {0x3, 0x02},
	[XT_KEY_I] = {0x3, 0x01},

	[XT_KEY_SEMICOLON] = {0x4, 0x80},
	[XT_KEY_L] = {0x4, 0x40},
	[XT_KEY_K] = {0x4, 0x20},
	[XT_KEY_J] = {0x4, 0x10},
	[XT_KEY_H] = {0x4, 0x08},
	[XT_KEY_G] = {0x4, 0x04},
	[XT_KEY_F] = {0x4, 0x02},
	[XT_KEY_D] = {0x4, 0x01},

	[XT_KEY_N] = {0x5, 0x80},
	[XT_KEY_B] = {0x5, 0x40},
	[XT_KEY_V] = {0x5, 0x20},
	[XT_KEY_C] = {0x5, 0x10},
	[XT_KEY_X] = {0x5, 0x08},
	[XT_KEY_Z] = {0x5, 0x04},
	[XT_KEY_CLOSED_BRACKED] = {0x5, 0x02},
	[XT_KEY_COLON] = {0x5, 0x01},

	[XT_KEY_DEL] = {0x6, 0x80},
	[XT_KEY_HOME] = {0x6, 0x40},
	[XT_KEY_SPACE] = {0x6, 0x20},
	[XT_KEY_UNDERSCORE] = {0x6, 0x10},
	[XT_KEY_FORWARD_SLASH] = {0x6, 0x08},
	[XT_KEY_PERIOD] = {0x6, 0x04},
	[XT_KEY_COMMA] = {0x6, 0x02},
	[XT_KEY_M] = {0x6, 0x01},

	[XT_KEY_CLR] = {0x7, 0x80},
	[XT_KEY_DOWN] = {0x7, 0x40},
	[XT_KEY_RIGHT] = {0x7, 0x20},
	[XT_KEY_UP] = {0x7, 0x10},
	[XT_KEY_LEFT] = {0x7, 0x08},
	[XT_KEY_UNDO] = {0x7, 0x04},
	[XT_KEY_R_DOWN] = {0x7, 0x02},
	[XT_KEY_R_UP] = {0x7, 0x01},

	[XT_KEY_NUMPAD_4] = {0x8, 0x80},
	[XT_KEY_NUMPAD_PLUS] = {0x8, 0x40},
	[XT_KEY_NUMPAD_9] = {0x8, 0x20},
	[XT_KEY_NUMPAD_8] = {0x8, 0x10},
	[XT_KEY_NUMPAD_7] = {0x8, 0x08},
	[XT_KEY_NUMPAD_MINUS] = {0x8, 0x04},
	[XT_KEY_NUMPAD_MULTIPLY] = {0x8, 0x02},
	[XT_KEY_NUMPAD_DIVIDE] = {0x8, 0x01},

	[XT_KEY_NUMPAD_0] = {0x9, 0x80},
	[XT_KEY_ENTER] = {0x9, 0x40},
	[XT_KEY_NUMPAD_3] = {0x9, 0x20},
	[XT_KEY_NUMPAD_2] = {0x9, 0x10},
	[XT_KEY_NUMPAD_1] = {0x9, 0x08},
	[XT_KEY_NUMPAD_EQUALS] = {0x9, 0x04},
	[XT_KEY_NUMPAD_6] = {0x9, 0x02},
	[XT_KEY_NUMPAD_5] = {0x9, 0x01},

	[XT_KEY_XF3] = {0xA, 0x80},
	[XT_KEY_XF2] = {0xA, 0x40},
	[XT_KEY_XF1] = {0xA, 0x20},
	[XT_KEY_HELP] = {0xA, 0x10},
	[XT_KEY_TOROKU] = {0xA, 0x08},
	[XT_KEY_KIGO] = {0xA, 0x04},
	[XT_KEY_NUMPAD_DECIMAL] = {0xA, 0x02},
	[XT_KEY_NUMPAD_COMMA] = {0xA, 0x01},

	[XT_KEY_HIRA] = {0xB, 0x80},
	[XT_KEY_INS] = {0xB, 0x40},
	[XT_KEY_CAPS] = {0xB, 0x20},
	[XT_KEY_CODE] = {0xB, 0x10},
	[XT_KEY_ROMA] = {0xB, 0x08},
	[XT_KEY_KANA] = {0xB, 0x04},
	[XT_KEY_XF5] = {0xB, 0x02},
	[XT_KEY_XF4] = {0xB, 0x01},

	[XT_KEY_F4] = {0xC, 0x80},
	[XT_KEY_F3] = {0xC, 0x40},
	[XT_KEY_F2] = {0xC, 0x20},
	[XT_KEY_F1] = {0xC, 0x10},
	[XT_KEY_F0] = {0xC, 0x08},
	[XT_KEY_COPY] = {0xC, 0x04},
	[XT_KEY_BREAK] = {0xC, 0x02},
	[XT_KEY_ZENKAKU] = {0xC, 0x01},

	[XT_KEY_F9] = {0xD, 0x10},
	[XT_KEY_F8] = {0xD, 0x08},
	[XT_KEY_F7] = {0xD, 0x04},
	[XT_KEY_F6] = {0xD, 0x02},
	[XT_KEY_F5] {0xD, 0x01},

	[XT_KEY_OPT2] = {0xE, 0x08},
	[XT_KEY_OPT1] = {0xE, 0x04},
	[XT_KEY_CTRL] = {0xE, 0x02},
	[XT_KEY_SHIFT] = {0xE, 0x01},
};

void xt_keys_init(XtKeys *k)
{
	memset(k, 0, sizeof(*k));
	k->repeat_delay = 12;
	k->repeat_period = 1;
}

static inline uint8_t key_is_held(const XtKeys *k, XtKeyName name)
{
	const XtKeyID *id = &key_lookup[name];
	return k->key_bits[id->group] & id->mask;
}

static inline XtKeyModifier get_modifiers(XtKeys *k)
{
	XtKeyModifier ret = 0;
	if (key_is_held(k, XT_KEY_SHIFT))
	{
		ret |= XT_KEY_MOD_SHIFT;
	}
	if (key_is_held(k, XT_KEY_CTRL))
	{
		ret |= XT_KEY_MOD_CTRL;
	}
	return ret;
}

static inline void event_push(XtKeys *k, XtKeyName name, int16_t repeat)
{
	const int16_t next_w = (k->key_w + 1) % ARRAYSIZE(k->key_events);
	if (next_w == k->key_r)
	{
		return;
	}

	k->key_events[k->key_w].name = name;
	k->key_events[k->key_w].modifiers = get_modifiers(k);
	if (repeat) k->key_events[k->key_w].modifiers |= XT_KEY_MOD_IS_REPEAT;
	k->key_w = next_w;
}

void xt_keys_poll(XtKeys *k)
{
	// Update key matrix bitfields
	for (int i = 0; i < ARRAYSIZE(k->key_bits); i++)
	{
		k->key_bits_prev[i] = k->key_bits[i];
		k->key_bits[i] = _iocs_bitsns(i);
	}

	// Generate events
	for (XtKeyName i = 0; i < XT_KEY_INVALID; i++)
	{
		if (xt_keys_pressed(k, i))
		{
			event_push(k, i, 0);
			if (k->repeat_name != i)
			{
				k->repeat_name = i;
				k->repeat_cnt = 0;
			}
		}
	}

	// Key repeat
	if (k->repeat_name != XT_KEY_INVALID)
	{
		if (xt_keys_held(k, k->repeat_name))
		{
			k->repeat_cnt++;
			if (k->repeat_cnt >= k->repeat_delay)
			{
				event_push(k, k->repeat_name, 1);
				k->repeat_cnt -= k->repeat_period;
			}
		}
		else
		{
			k->repeat_name = XT_KEY_INVALID;
		}
	}
}

// Get the next key event logged since the last poll.
// Returns 1 if `out` was populated.
int16_t xt_keys_event_pop(XtKeys *k, XtKeyEvent *out)
{
	if (k->key_r == k->key_w)
	{
		return 0;
	}
	const int16_t next_r = (k->key_r + 1) % ARRAYSIZE(k->key_events);

	*out = k->key_events[k->key_r];

	k->key_r = next_r;
	return 1;
}

uint8_t xt_keys_pressed(const XtKeys *k, XtKeyName name)
{
	const XtKeyID *id = &key_lookup[name];
	return key_is_held(k, name) & ~(k->key_bits_prev[id->group]);
}

uint8_t xt_keys_held(const XtKeys *k, XtKeyName name)
{
	return key_is_held(k, name);
}
