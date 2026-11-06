#include "test_util.h"
#include "arrow.h"

static int render(float ppu, float rim, float shadow, ArrowColor color, ArrowImage *out)
{
    ArrowStyle st = { rim, 1.0f, shadow, color };
    return arrow_render(ppu, &st, out);
}

static unsigned px(const ArrowImage *img, int x, int y)
{
    return img->pixels[y * img->width + x];
}

static unsigned alpha(unsigned p) { return p >> 24; }
static unsigned red(unsigned p) { return (p >> 16) & 0xFF; }

/* Pixel at arrow-space unit coordinate (u, v). */
static unsigned at_unit(const ArrowImage *img, float ppu, float u, float v)
{
    int x = img->hot_x + (int)(u * ppu);
    int y = img->hot_y + (int)(v * ppu);
    return px(img, x, y);
}

static void sdf_sign_matches_inside_outside(void)
{
    CHECK(arrow_sdf(2.0f, 9.0f) > 0.0f);    /* body */
    CHECK(arrow_sdf(7.5f, 16.0f) > 0.0f);   /* tail */
    CHECK(arrow_sdf(10.0f, 3.0f) < 0.0f);   /* above the diagonal edge */
    CHECK(arrow_sdf(-1.0f, 5.0f) < 0.0f);   /* left of the left edge */
    CHECK(arrow_sdf(1.0f, 18.5f) < 0.0f);   /* under the notch */
}

static void sdf_magnitude_is_distance_to_edge(void)
{
    float d = arrow_sdf(-2.0f, 8.0f);       /* 2 units left of x=0 edge */
    CHECK(d < -1.95f && d > -2.05f);
}

static void render_rejects_bad_scale(void)
{
    ArrowImage img;
    CHECK(render(0.0f, ARROW_RIM_UNITS, 1, ARROW_BLACK, &img) == -1);
    CHECK(render(-3.0f, ARROW_RIM_UNITS, 1, ARROW_BLACK, &img) == -1);
}

static void render_size_scales_with_ppu(void)
{
    ArrowImage a, b;
    CHECK(render(2.0f, ARROW_RIM_UNITS, 1, ARROW_BLACK, &a) == 0);
    CHECK(render(6.0f, ARROW_RIM_UNITS, 1, ARROW_BLACK, &b) == 0);
    CHECK(a.height >= (int)(ARROW_UNITS_H * 2.0f));
    CHECK(b.height >= (int)(ARROW_UNITS_H * 6.0f));
    CHECK(b.width > a.width * 2);
    CHECK(a.hot_x >= 0 && a.hot_x < a.width && a.hot_y >= 0 && a.hot_y < a.height);
    arrow_free(&a);
    arrow_free(&b);
}

static void render_colors_are_right(void)
{
    const float ppu = 6.0f;
    ArrowImage img;
    CHECK(render(ppu, ARROW_RIM_UNITS, 0, ARROW_BLACK, &img) == 0);
    unsigned core = at_unit(&img, ppu, 3.0f, 10.0f);
    CHECK(alpha(core) == 255 && red(core) < 10);        /* opaque black */
    unsigned rim = at_unit(&img, ppu, 0.3f, 9.0f);
    CHECK(alpha(rim) == 255 && red(rim) > 245);         /* white border */
    unsigned out = at_unit(&img, ppu, 10.0f, 3.0f);
    CHECK(alpha(out) == 0);                             /* transparent */
    arrow_free(&img);
}

static void white_style_inverts_fill_and_rim(void)
{
    const float ppu = 6.0f;
    ArrowImage img;
    CHECK(render(ppu, ARROW_RIM_UNITS, 0, ARROW_WHITE, &img) == 0);
    unsigned core = at_unit(&img, ppu, 3.0f, 10.0f);
    CHECK(alpha(core) == 255 && red(core) > 245);       /* opaque white */
    unsigned rim = at_unit(&img, ppu, 0.3f, 9.0f);
    CHECK(alpha(rim) == 255 && red(rim) < 10);          /* black border */
    unsigned out = at_unit(&img, ppu, 10.0f, 3.0f);
    CHECK(alpha(out) == 0);
    arrow_free(&img);
}

static void unknown_style_is_rejected(void)
{
    ArrowImage img;
    CHECK(render(4.0f, ARROW_RIM_UNITS, 1, (ArrowColor)7, &img) == -1);
}

static void render_is_premultiplied(void)
{
    const ArrowColor colors[] = { ARROW_BLACK, ARROW_WHITE };
    for (int c = 0; c < 2; c++) {
        ArrowImage img;
        CHECK(render(4.0f, ARROW_RIM_UNITS, 1, colors[c], &img) == 0);
        int ok = 1;
        for (int i = 0; i < img.width * img.height; i++) {
            unsigned p = img.pixels[i];
            if (red(p) > alpha(p)) ok = 0;
        }
        CHECK(ok);
        arrow_free(&img);
    }
}

static void shadow_only_adds_alpha_outside(void)
{
    const float ppu = 6.0f;
    ArrowImage plain, shadow;
    CHECK(render(ppu, ARROW_RIM_UNITS, 0, ARROW_BLACK, &plain) == 0);
    CHECK(render(ppu, ARROW_RIM_UNITS, 1, ARROW_BLACK, &shadow) == 0);
    /* just below-right of the diagonal edge: shadow region */
    unsigned p = at_unit(&plain, ppu, 6.6f, 5.9f);
    unsigned s = at_unit(&shadow, ppu, 6.6f, 5.9f);
    CHECK(alpha(s) > alpha(p));
    CHECK(red(s) <= red(p) + 1);
    arrow_free(&plain);
    arrow_free(&shadow);
}

static void partial_shadow_is_between(void)
{
    const float ppu = 6.0f;
    ArrowImage none, half, full;
    CHECK(render(ppu, ARROW_RIM_UNITS, 0.0f, ARROW_WHITE, &none) == 0);
    CHECK(render(ppu, ARROW_RIM_UNITS, 0.5f, ARROW_WHITE, &half) == 0);
    CHECK(render(ppu, ARROW_RIM_UNITS, 1.0f, ARROW_WHITE, &full) == 0);
    /* same canvas for every non-zero strength, so fading frames line up */
    CHECK(half.width == full.width && half.height == full.height);
    CHECK(half.hot_x == full.hot_x && half.hot_y == full.hot_y);
    unsigned n = alpha(at_unit(&none, ppu, 6.6f, 5.9f));
    unsigned h = alpha(at_unit(&half, ppu, 6.6f, 5.9f));
    unsigned f = alpha(at_unit(&full, ppu, 6.6f, 5.9f));
    CHECK(n < h && h < f);
    arrow_free(&none);
    arrow_free(&half);
    arrow_free(&full);
}

static void thinner_rim_is_thinner(void)
{
    const float ppu = 6.0f;
    ArrowImage thick, thin;
    CHECK(render(ppu, 1.0f, 0.0f, ARROW_WHITE, &thick) == 0);
    CHECK(render(ppu, 0.5f, 0.0f, ARROW_WHITE, &thin) == 0);
    /* 0.75 units inside the left edge: black with a 1.0 rim, white with 0.5 */
    CHECK(red(at_unit(&thick, ppu, 0.6f, 9.0f)) < 10);
    CHECK(red(at_unit(&thin, ppu, 0.6f, 9.0f)) > 245);
    arrow_free(&thick);
    arrow_free(&thin);
}

static void bad_rim_is_rejected(void)
{
    ArrowImage img;
    CHECK(render(4.0f, 0.0f, 0.0f, ARROW_WHITE, &img) == -1);
    CHECK(render(4.0f, -1.0f, 0.0f, ARROW_WHITE, &img) == -1);
    CHECK(render(4.0f, 9.0f, 0.0f, ARROW_WHITE, &img) == -1);
}

static void faded_rim_takes_fill_colour(void)
{
    const float ppu = 6.0f;
    ArrowStyle faded = { ARROW_RIM_UNITS, 0.0f, 0.0f, ARROW_WHITE };
    ArrowStyle half = { ARROW_RIM_UNITS, 0.5f, 0.0f, ARROW_WHITE };
    ArrowImage a, b;
    CHECK(arrow_render(ppu, &faded, &a) == 0);
    CHECK(arrow_render(ppu, &half, &b) == 0);
    unsigned rim_a = at_unit(&a, ppu, 0.3f, 9.0f);
    unsigned rim_b = at_unit(&b, ppu, 0.3f, 9.0f);
    CHECK(alpha(rim_a) == 255 && red(rim_a) > 245);     /* rim gone: all white */
    CHECK(red(rim_b) > 100 && red(rim_b) < 155);        /* half way: grey */
    ArrowStyle black_faded = { ARROW_RIM_UNITS, 0.0f, 0.0f, ARROW_BLACK };
    ArrowImage c;
    CHECK(arrow_render(ppu, &black_faded, &c) == 0);
    CHECK(red(at_unit(&c, ppu, 0.3f, 9.0f)) < 10);      /* black style: all black */
    arrow_free(&a);
    arrow_free(&b);
    arrow_free(&c);
}

static void bad_style_is_rejected(void)
{
    ArrowImage img;
    ArrowStyle bad = { ARROW_RIM_UNITS, 1.5f, 0.0f, ARROW_WHITE };
    CHECK(arrow_render(4.0f, &bad, &img) == -1);
    CHECK(arrow_render(4.0f, NULL, &img) == -1);
}

void run_arrow_tests(void)
{
    printf("[arrow]\n");
    RUN(sdf_sign_matches_inside_outside);
    RUN(sdf_magnitude_is_distance_to_edge);
    RUN(render_rejects_bad_scale);
    RUN(render_size_scales_with_ppu);
    RUN(render_colors_are_right);
    RUN(white_style_inverts_fill_and_rim);
    RUN(unknown_style_is_rejected);
    RUN(render_is_premultiplied);
    RUN(shadow_only_adds_alpha_outside);
    RUN(partial_shadow_is_between);
    RUN(thinner_rim_is_thinner);
    RUN(bad_rim_is_rejected);
    RUN(faded_rim_takes_fill_colour);
    RUN(bad_style_is_rejected);
}
