#ifndef XT_VBL_H
#define XT_VBL_H

#include <stdint.h>
#include <iocs.h>

extern volatile uint16_t g_xt_vbl_pending;

// Register the XT VBlank ISR.
void xt_vbl_init(void);

// Wait for n vblanks.
void xt_vbl_wait(uint16_t n);

// Unregister the XT VBlank ISR and put back whatever was there before.
void xt_vbl_shutdown(void);

#endif  // XT_VBL_H
