/* Temporarily hides the real system cursor while the big one is shown,
 * so the enlarged arrow replaces the cursor instead of sitting under it.
 *
 * Safety: a registry flag records "cursor hidden"; if the process dies
 * while hidden, the next start restores the cursors. A crash handler and
 * session-end handler restore them too. */
#ifndef SYSCURSOR_H
#define SYSCURSOR_H

/* Hides only after the recovery flag is durably written; if that fails
 * the real cursor simply stays visible (safe fallback). */
int syscursor_hide(void);             /* 0 if hidden (now or already) */
int syscursor_restore(void);          /* 0 on success (or nothing to do) */
int syscursor_is_hidden(void);
void syscursor_recover(void);         /* call once at startup */
void syscursor_install_crash_guard(void);

#endif
