#include "syscursor.h"

#include "app.h"
#include "config.h"

#include <string.h>

#define BLANK_SIZE 32

/* Standard system cursor ids (OCR_* from winuser.h with OEMRESOURCE). */
static const DWORD CURSOR_IDS[] = {
    32512, /* NORMAL */      32513, /* IBEAM */      32514, /* WAIT */
    32515, /* CROSS */       32516, /* UP */         32642, /* SIZENWSE */
    32643, /* SIZENESW */    32644, /* SIZEWE */     32645, /* SIZENS */
    32646, /* SIZEALL */     32648, /* NO */         32649, /* HAND */
    32650, /* APPSTARTING */ 32651, /* HELP */       32631, /* PEN */
    32671, /* PIN */         32672, /* PERSON */
};
#define NCURSORS ((int)(sizeof CURSOR_IDS / sizeof CURSOR_IDS[0]))

static volatile LONG g_hidden;

static HCURSOR make_blank_cursor(void)
{
    BYTE and_mask[BLANK_SIZE * BLANK_SIZE / 8];
    BYTE xor_mask[BLANK_SIZE * BLANK_SIZE / 8];
    memset(and_mask, 0xFF, sizeof and_mask);   /* keep screen pixels */
    memset(xor_mask, 0x00, sizeof xor_mask);   /* ...unchanged */
    return CreateCursor(GetModuleHandleW(NULL), 0, 0, BLANK_SIZE, BLANK_SIZE,
                        and_mask, xor_mask);
}

int syscursor_is_hidden(void)
{
    return g_hidden != 0;
}

int syscursor_hide(void)
{
    if (g_hidden) return 0;
    /* the recovery flag must be on disk before any cursor disappears */
    if (cursor_flag_set(1) != 0) return -1;
    InterlockedExchange(&g_hidden, 1);
    for (int i = 0; i < NCURSORS; i++) {
        HCURSOR blank = make_blank_cursor();
        /* SetSystemCursor takes ownership of `blank` on success */
        if (blank && !SetSystemCursor(blank, CURSOR_IDS[i])) DestroyCursor(blank);
    }
    return 0;
}

int syscursor_restore(void)
{
    if (!g_hidden) return 0;
    /* reloads the user's cursor scheme from HKCU\Control Panel\Cursors */
    if (!SystemParametersInfoW(SPI_SETCURSORS, 0, NULL, 0)) return -1;
    InterlockedExchange(&g_hidden, 0);
    cursor_flag_set(0);   /* a stale 1 only causes one harmless reload */
    return 0;
}

void syscursor_recover(void)
{
    if (!cursor_flag_get()) return;
    if (SystemParametersInfoW(SPI_SETCURSORS, 0, NULL, 0)) cursor_flag_set(0);
}

static LONG WINAPI crash_filter(EXCEPTION_POINTERS *info)
{
    (void)info;
    (void)syscursor_restore();
    return EXCEPTION_CONTINUE_SEARCH;
}

void syscursor_install_crash_guard(void)
{
    SetUnhandledExceptionFilter(crash_filter);
}
