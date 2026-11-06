/* Software rasterizer for a big, anti-aliased arrow cursor.
 * 1 unit == 1 pixel of the standard 100%-scale Windows arrow. */
#ifndef ARROW_H
#define ARROW_H

#define ARROW_UNITS_W 12.2f
#define ARROW_UNITS_H 19.0f
#define ARROW_RIM_UNITS 1.0f   /* classic rim width for big / icon arrows */

typedef enum {
    ARROW_BLACK = 0,   /* black fill, white rim */
    ARROW_WHITE = 1,   /* white fill, black rim (Windows default look) */
} ArrowColor;

typedef struct {
    int width, height;
    int hot_x, hot_y;     /* pixel where the arrow tip (cursor hotspot) is */
    unsigned *pixels;     /* width*height, 0xAARRGGBB premultiplied, top-down */
} ArrowImage;

/* Signed distance (in units) from (x, y) to the arrow outline.
 * Positive inside the arrow, negative outside. */
float arrow_sdf(float x, float y);

#define ARROW_MAX_RIM_UNITS 4.0f

typedef struct {
    float rim_units;     /* rim width, 0 < w <= ARROW_MAX_RIM_UNITS */
    float rim_strength;  /* 1 = contrasting rim, 0 = rim takes the fill colour */
    float shadow;        /* drop-shadow strength 0..1 (0 = none, tighter canvas) */
    ArrowColor color;
} ArrowStyle;

/* Render the arrow at px_per_unit scale. Returns 0 on success, -1 on bad
 * input or allocation failure. Free with arrow_free(). */
int arrow_render(float px_per_unit, const ArrowStyle *style, ArrowImage *out);

void arrow_free(ArrowImage *img);

#endif
