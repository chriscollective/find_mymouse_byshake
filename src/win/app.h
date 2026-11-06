/* Shared app state for the Windows front end. */
#ifndef APP_H
#define APP_H

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "settings.h"
#include "shake.h"

#define APP_NAME     L"find_mymouse_byshake"
#define APP_TITLE    L"Find My Mouse by Shake"
#define WND_CLASS    L"find_mymouse_byshake.main"

#define WM_APP_TRAY  (WM_APP + 1)   /* tray icon callback */
#define WM_APP_PING  (WM_APP + 2)   /* a second instance was launched */
#define WM_APP_FRAME (WM_APP + 3)   /* vsync-paced animation tick */

typedef struct {
    HINSTANCE inst;
    HWND hwnd;
    int enabled;
    Sensitivity sens;
    CursorSize size;
    ArrowColor color;
    ShakeDetector det;
    UINT det_dpi;         /* DPI the detector thresholds were scaled for */
    float scale;          /* current cursor scale (1 == normal size) */
    float target;         /* scale we are animating toward */
    DWORD last_shake;     /* tick of the most recent shake event */
    double last_frame_ms; /* precise time of the previous animation frame */
    int animating;
    int hide_failed;      /* hiding failed this effect: don't retry per frame */
    int restore_backoff;  /* a cursor restore failed: wait before retrying */
    DWORD restore_failed_at;
    float shrink_from;    /* scale when the timed shrink started */
    double shrink_t0;     /* precise_ms() when the timed shrink started */
    UINT msg_taskbar_created;
} App;

#endif
