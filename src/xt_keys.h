#ifndef _XT_KEYS_H
#define _XT_KEYS_H

#include <stdint.h>

typedef enum XtKeyName
{
	XT_KEY_6,
	XT_KEY_5,
	XT_KEY_4,
	XT_KEY_3,
	XT_KEY_2,
	XT_KEY_1,
	XT_KEY_ESC,

	XT_KEY_BS,
	XT_KEY_PIPE,
	XT_KEY_TILDE,
	XT_KEY_MINUS,
	XT_KEY_0,
	XT_KEY_9,
	XT_KEY_8,
	XT_KEY_7,

	XT_KEY_U,
	XT_KEY_Y,
	XT_KEY_T,
	XT_KEY_R,
	XT_KEY_E,
	XT_KEY_W,
	XT_KEY_Q,
	XT_KEY_TAB,

	XT_KEY_S,
	XT_KEY_A,
	XT_KEY_CR,
	XT_KEY_OPEN_BRACKET,
	XT_KEY_AT,
	XT_KEY_P,
	XT_KEY_O,
	XT_KEY_I,

	XT_KEY_SEMICOLON,
	XT_KEY_L,
	XT_KEY_K,
	XT_KEY_J,
	XT_KEY_H,
	XT_KEY_G,
	XT_KEY_F,
	XT_KEY_D,

	XT_KEY_N,
	XT_KEY_B,
	XT_KEY_V,
	XT_KEY_C,
	XT_KEY_X,
	XT_KEY_Z,
	XT_KEY_CLOSED_BRACKED,
	XT_KEY_COLON,

	XT_KEY_DEL,
	XT_KEY_HOME,
	XT_KEY_SPACE,
	XT_KEY_UNDERSCORE,
	XT_KEY_FORWARD_SLASH,
	XT_KEY_PERIOD,
	XT_KEY_COMMA,
	XT_KEY_M,

	XT_KEY_CLR,
	XT_KEY_DOWN,
	XT_KEY_RIGHT,
	XT_KEY_UP,
	XT_KEY_LEFT,
	XT_KEY_UNDO,
	XT_KEY_R_DOWN,
	XT_KEY_R_UP,

	XT_KEY_NUMPAD_4,
	XT_KEY_NUMPAD_PLUS,
	XT_KEY_NUMPAD_9,
	XT_KEY_NUMPAD_8,
	XT_KEY_NUMPAD_7,
	XT_KEY_NUMPAD_MINUS,
	XT_KEY_NUMPAD_MULTIPLY,
	XT_KEY_NUMPAD_DIVIDE,

	XT_KEY_NUMPAD_0,
	XT_KEY_ENTER,
	XT_KEY_NUMPAD_3,
	XT_KEY_NUMPAD_2,
	XT_KEY_NUMPAD_1,
	XT_KEY_NUMPAD_EQUALS,
	XT_KEY_NUMPAD_6,
	XT_KEY_NUMPAD_5,

	XT_KEY_XF3,
	XT_KEY_XF2,
	XT_KEY_XF1,
	XT_KEY_HELP,
	XT_KEY_TOROKU,
	XT_KEY_KIGO,
	XT_KEY_NUMPAD_DECIMAL,
	XT_KEY_NUMPAD_COMMA,

	XT_KEY_HIRA,
	XT_KEY_INS,
	XT_KEY_CAPS,
	XT_KEY_CODE,
	XT_KEY_ROMA,
	XT_KEY_KANA,
	XT_KEY_XF5,
	XT_KEY_XF4,
	
	XT_KEY_F4,
	XT_KEY_F3,
	XT_KEY_F2,
	XT_KEY_F1,
	XT_KEY_F0,
	XT_KEY_COPY,
	XT_KEY_BREAK,
	XT_KEY_ZENKAKU,

	XT_KEY_F9,
	XT_KEY_F8,
	XT_KEY_F7,
	XT_KEY_F6,
	XT_KEY_F5,

	XT_KEY_OPT2,
	XT_KEY_OPT1,
	XT_KEY_CTRL,
	XT_KEY_SHIFT,

	XT_KEY_INVALID,
} XtKeyName;

// Bitfield for modifier keys that have been pressed.
typedef enum XtKeyModifier
{
	XT_KEY_MOD_SHIFT     = 0x0001,
	XT_KEY_MOD_CTRL      = 0x0002,
	XT_KEY_MOD_IS_REPEAT = 0x1000,
} XtKeyModifier;

typedef struct XtKeyID
{
	uint8_t group;
	uint8_t mask;
} XtKeyID;

typedef struct XtKeyEvent
{
	XtKeyName name;
	XtKeyModifier modifiers;
} XtKeyEvent;

typedef struct XtKeys
{
	// State of all keys in X68k-native bitfield format
	uint8_t key_bits[15];
	uint8_t key_bits_prev[15];

	// Key event FIFO
	XtKeyEvent key_events[16];
	int16_t key_w;
	int16_t key_r;

	// Key repeat state
	XtKeyName repeat_name;
	int16_t repeat_cnt;
	int16_t repeat_delay;  // TODO: Load from application config.
	int16_t repeat_period;  // TODO: Load from application config.
} XtKeys;

void xt_keys_init(XtKeys *k);
void xt_keys_poll(XtKeys *k);

// Get the next key event logged since the last poll.
// Returns 1 if `out` was populated.
int16_t xt_keys_event_pop(XtKeys *k, XtKeyEvent *out);

uint8_t xt_keys_pressed(const XtKeys *k, XtKeyName name);
uint8_t xt_keys_held(const XtKeys *k, XtKeyName name);

#endif  // _XT_KEYS_H
