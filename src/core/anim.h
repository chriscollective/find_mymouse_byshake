/* Smooth scale animation for the enlarged cursor. */
#ifndef ANIM_H
#define ANIM_H

/* Move `cur` toward `target` over dt_ms using exponential easing.
 * Growing is faster than shrinking, like macOS. Never overshoots. */
float anim_step(float cur, float target, float dt_ms);

/* Timed shrink back to normal size: scale after elapsed_ms, landing on
 * exactly 1.0 at ANIM_SHRINK_MS. The curve starts briskly and still has
 * some speed when it lands, so the end is a crisp settle instead of an
 * endless crawl toward 1.0 (which is what exponential easing does). */
#define ANIM_SHRINK_MS 260.0f
float anim_shrink_scale(float from, float elapsed_ms);

/* Drop-shadow strength (0..1) for the big arrow at `scale`. Fades out
 * toward normal size so the hand-off to the real cursor is seamless;
 * if the system pointer shadow is on, it fades to a matching level. */
float anim_shadow_strength(float scale, int system_shadow);

/* Rim width in arrow units at `scale` on a `dpi` monitor (0 = 96). At
 * normal size it is exactly 1 physical pixel, like the real Windows
 * cursor at any DPI; it widens smoothly to ARROW_RIM_UNITS when big. */
float anim_rim_units(float scale, unsigned dpi);

/* Hand-off to the real cursor. While the arrow grows or is big, the real
 * cursor is hidden. Once shrinking below ANIM_HANDOFF_SCALE, the real
 * cursor (always drawn on top) comes back and the big arrow fades out
 * underneath it, so its outline never visibly snaps. */
#define ANIM_HANDOFF_SCALE 1.1f
#define ANIM_FADE_END      1.03f   /* overlay fully transparent at/below this */
int anim_hide_real_cursor(float scale, float target);
float anim_handoff_opacity(float scale);   /* 1 at hand-off .. 0 at FADE_END */
/* Rim strength during the hand-off: the big arrow's rim fades into its
 * fill first, so no second outline shows around the real cursor. */
float anim_handoff_rim(float scale);

#endif
