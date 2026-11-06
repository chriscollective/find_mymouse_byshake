/* Click-through, always-on-top layered window that draws the big cursor. */
#ifndef OVERLAY_H
#define OVERLAY_H

#include "app.h"

int overlay_init(HINSTANCE inst);           /* 0 on success */
UINT dpi_at(POINT pt);                      /* effective DPI of pt's monitor */
/* Arrow tip at pt. handoff: the real cursor is back on top, so the arrow
 * drops its rim and fades out underneath it. */
void overlay_show(float scale, ArrowColor color, int handoff, POINT pt);
void overlay_move(POINT pt);                /* reposition only (cheap) */
void overlay_hide(void);                    /* also frees cached frames */
void overlay_shutdown(void);

#endif
