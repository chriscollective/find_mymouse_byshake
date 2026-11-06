/* User-facing presets mapped to detector / renderer parameters. */
#ifndef SETTINGS_H
#define SETTINGS_H

#include "arrow.h"
#include "shake.h"

typedef enum { SENS_LOW = 0, SENS_MEDIUM = 1, SENS_HIGH = 2 } Sensitivity;
typedef enum { SIZE_SMALL = 0, SIZE_MEDIUM = 1, SIZE_LARGE = 2 } CursorSize;

/* dpi 0 is treated as 96. Pixel thresholds scale with dpi. */
ShakeConfig settings_shake_config(Sensitivity s, unsigned dpi);
float settings_max_scale(CursorSize z);

/* Sanitize values read from the registry. */
Sensitivity settings_clamp_sensitivity(int v);
CursorSize settings_clamp_size(int v);
ArrowColor settings_clamp_color(int v);     /* default: ARROW_WHITE */

#endif
