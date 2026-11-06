/* Build-time tool (runs on the build host): renders the arrow with the
 * same rasterizer as the app and writes a multi-size Windows .ico. */
#include "arrow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int SIZES[] = { 16, 20, 24, 32, 40, 48, 64, 128 };
#define NSIZES ((int)(sizeof SIZES / sizeof SIZES[0]))
#define SHADOW_MIN_SIZE 48

static void put16(FILE *f, unsigned v) { fputc((int)(v & 0xFF), f); fputc((int)((v >> 8) & 0xFF), f); }
static void put32(FILE *f, unsigned v) { put16(f, v & 0xFFFF); put16(f, v >> 16); }

static unsigned unpremultiply(unsigned p)
{
    unsigned a = p >> 24;
    if (a == 0) return 0;
    unsigned c = (p & 0xFF) * 255u / a;
    if (c > 255) c = 255;
    return (a << 24) | (c << 16) | (c << 8) | c;
}

/* s*s straight-alpha pixels, arrow fitted and centered. */
static unsigned *render_square(int s)
{
    float ppu = (float)s / (ARROW_UNITS_H + 1.0f);
    ArrowImage img;
    for (;;) {
        ArrowStyle st = { ARROW_RIM_UNITS, 1.0f, s >= SHADOW_MIN_SIZE ? 1.0f : 0.0f,
                          ARROW_BLACK };
        if (arrow_render(ppu, &st, &img) != 0) return NULL;
        if (img.width <= s && img.height <= s) break;
        arrow_free(&img);
        ppu *= 0.97f;
    }
    unsigned *out = calloc((size_t)s * (size_t)s, sizeof *out);
    if (out) {
        int ox = (s - img.width) / 2, oy = (s - img.height) / 2;
        for (int y = 0; y < img.height; y++)
            for (int x = 0; x < img.width; x++)
                out[(oy + y) * s + ox + x] = unpremultiply(img.pixels[y * img.width + x]);
    }
    arrow_free(&img);
    return out;
}

static unsigned entry_bytes(int s)
{
    unsigned mask_row = (unsigned)((s + 31) / 32) * 4u;
    return 40u + (unsigned)(s * s) * 4u + mask_row * (unsigned)s;
}

static void write_bitmap(FILE *f, int s, const unsigned *px)
{
    unsigned mask_row = (unsigned)((s + 31) / 32) * 4u;
    put32(f, 40); put32(f, (unsigned)s); put32(f, (unsigned)(2 * s));
    put16(f, 1); put16(f, 32); put32(f, 0);
    put32(f, entry_bytes(s) - 40u); put32(f, 0); put32(f, 0); put32(f, 0); put32(f, 0);
    for (int y = s - 1; y >= 0; y--)            /* bottom-up rows */
        for (int x = 0; x < s; x++) put32(f, px[y * s + x]);
    for (unsigned i = 0; i < mask_row * (unsigned)s; i++) fputc(0, f);
}

static int write_ico(FILE *f, unsigned *const imgs[])
{
    put16(f, 0); put16(f, 1); put16(f, (unsigned)NSIZES);
    unsigned offset = 6u + 16u * (unsigned)NSIZES;
    for (int i = 0; i < NSIZES; i++) {
        int s = SIZES[i];
        fputc(s >= 256 ? 0 : s, f); fputc(s >= 256 ? 0 : s, f);
        fputc(0, f); fputc(0, f);
        put16(f, 1); put16(f, 32);
        put32(f, entry_bytes(s)); put32(f, offset);
        offset += entry_bytes(s);
    }
    for (int i = 0; i < NSIZES; i++) write_bitmap(f, SIZES[i], imgs[i]);
    return ferror(f) ? -1 : 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s out.ico\n", argv[0]);
        return 2;
    }
    unsigned *imgs[NSIZES] = { 0 };
    int rc = 0;
    for (int i = 0; i < NSIZES && rc == 0; i++) {
        imgs[i] = render_square(SIZES[i]);
        if (!imgs[i]) rc = 1;
    }
    FILE *f = rc == 0 ? fopen(argv[1], "wb") : NULL;
    if (rc == 0 && (!f || write_ico(f, imgs) != 0)) rc = 1;
    if (f && fclose(f) != 0) rc = 1;
    for (int i = 0; i < NSIZES; i++) free(imgs[i]);
    if (rc != 0) fprintf(stderr, "make_icon: failed to write %s\n", argv[1]);
    return rc;
}
