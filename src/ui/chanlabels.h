#ifndef UI_CHANLABELS_H
#define UI_CHANLABELS_H

#include "xt/instrument.h"
#include <stdint.h>

// Renders channel label index i to reflect displayed channel num.
void ui_chanlabel_set(uint16_t i, uint16_t num, XtChannelType type);

#endif  // UI_CHANLABELS_H
