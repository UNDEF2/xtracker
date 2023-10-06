#ifndef XT_TRACK_H
#define XT_TRACK_H

#define XT_CMD_COL_COUNT 3
// this is the size of the buffer, title can't use last byte since need NUL
#define XT_TITLE_COUNT 256

// Some program configuration.
#define XT_PHRASE_MAX_ROWS 64
#define XT_PHRASES_PER_CHANNEL 128

#define XT_TOTAL_CHANNEL_COUNT (8 + 4)

#define XT_FRAME_COUNT 128
#define XT_INSTRUMENT_COUNT 64
#define XT_SAMPLE_COUNT 64

#include "xt/instrument.h"
#include "xt/types.h"

typedef enum XtCmd
{
	XT_CMD_NONE = '\0',

	XT_CMD_TL_OP0 = '0',
	XT_CMD_TL_OP1 = '1',
	XT_CMD_TL_OP2 = '2',
	XT_CMD_TL_OP3 = '3',

	XT_CMD_MULT_OP0 = '4',
	XT_CMD_MULT_OP1 = '5',
	XT_CMD_MULT_OP2 = '6',
	XT_CMD_MULT_OP3 = '7',

	XT_CMD_BREAK = 'B',

	XT_CMD_GROOVE0 = 'E',
	XT_CMD_GROOVE1 = 'F',
	XT_CMD_TREMOLO_TYPE = 'G',
	XT_CMD_VIBRATO_TYPE = 'H',

	XT_CMD_NOISE_EN = 'N',

	XT_CMD_PAN = 'O',

	XT_CMD_PERIOD = 'P',

	XT_CMD_TREMOLO = 'T',
	XT_CMD_VIBRATO = 'V',

	XT_CMD_SLIDE = 'L',
	XT_CMD_SLIDE_UP = 'Q',
	XT_CMD_SLIDE_DOWN = 'R',
	XT_CMD_MUTE_DELAY = 'S',
	XT_CMD_NOTE_DELAY = 'W',
	XT_CMD_CUT_DELAY = 'X',
	XT_CMD_TUNE = 'Z',

	XT_CMD_MAX = 0xFF,
} XtCmd;

typedef enum XtNote
{
	XT_NOTE_NONE = 0x0,

	// Main notes.
	XT_NOTE_C    = 0x1,
	XT_NOTE_CS   = 0x2,
	XT_NOTE_D    = 0x3,
	XT_NOTE_DS   = 0x4,
	XT_NOTE_E    = 0x5,
	XT_NOTE_F    = 0x6,
	XT_NOTE_FS   = 0x7,
	XT_NOTE_G    = 0x8,
	XT_NOTE_GS   = 0x9,
	XT_NOTE_A    = 0xA,
	XT_NOTE_AS   = 0xB,
	XT_NOTE_B    = 0xC,

	XT_NOTE_NG_D = 0xD,
	XT_NOTE_NG_E = 0xE,
	XT_NOTE_NG_F = 0xF,

	XT_NOTE_OFF = 0xFE,
	XT_NOTE_CUT = 0xFF,
} XtNote;

#define XT_NOTE_TONE_MASK      0x0F
#define XT_NOTE_OCTAVE_MASK    0xF0

// A single line of a phrase pattern.
typedef struct XtCell
{
	XtNote note;
	uint8_t inst;
	uint8_t vol;  // 0x80 - 0xFF for the actual volume, displayed as 0x00-0x7F.
	struct
	{
		XtCmd cmd;
		uint8_t arg;
	} cmd[XT_CMD_COL_COUNT];
} XtCell;

// Vertical collection of cells for one channel.
typedef struct XtPhrase
{
	XtCell cells[XT_PHRASE_MAX_ROWS];
} XtPhrase;

// One row within the arrangement table.
typedef struct XtFrame
{
	int16_t phrase_id[XT_TOTAL_CHANNEL_COUNT];
} XtFrame;

typedef struct XtTrackChannelData
{
	// TODO: Dynamic phrase allocation within channel
	XtInstrumentType type;
	int16_t voice_number;  // Hardware voice number for relevant chip.
	XtPhrase phrases[XT_PHRASES_PER_CHANNEL];
	// TODO: Phrase count?
} XtTrackChannelData;

typedef struct XtTrackMeta
{
	char title[64];
	char author[64];
	char adpcm_fname[256];
	uint8_t row_highlight[2];  // Graphical highlight positions
	int16_t phrase_length;     // How many cell rows are in a phrase.
	int16_t loop_point;        // Arrangement row to return to (-1 for no loop).
	int16_t timer_period;      // Starting OPM timer period.
	uint8_t groove[2];         // Ticks per row (even / odd)
} XtTrackMeta;

// One whole song in memory.
typedef struct XtTrack
{
	XtTrackMeta meta;
	// "Frames" - top level arrangement unit.
	XtFrame frames[XT_FRAME_COUNT];
	int16_t num_frames;

	// "Phrases" - musical entries indexed by channel, referenced by frames.
	XtTrackChannelData channel_data[XT_TOTAL_CHANNEL_COUNT];

	// Instruments are patch configurations / ADPCM / MIDI mappings.
	XtInstrument instruments[XT_INSTRUMENT_COUNT];
	int16_t num_instruments;

} XtTrack;

XtPhrase *xt_track_get_phrase(XtTrack *t, uint16_t channel,
							  uint16_t frame);

// avoid type safety violation factory
const XtPhrase *xt_track_get_phrase_const(const XtTrack *t, uint16_t channel,
										  uint16_t frame);

void xt_track_init(XtTrack *t);
XtResult xt_track_save_to_file(const XtTrack *t, const char *fname);
XtResult xt_track_load_from_file(XtTrack *t, const char *fname);

#endif  // XT_TRACK_H