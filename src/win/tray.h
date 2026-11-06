/* System tray icon and its right-click menu. */
#ifndef TRAY_H
#define TRAY_H

#include "app.h"

enum {
    IDM_ENABLE = 100,
    IDM_SENS_LOW, IDM_SENS_MEDIUM, IDM_SENS_HIGH,
    IDM_SIZE_SMALL, IDM_SIZE_MEDIUM, IDM_SIZE_LARGE,
    IDM_COLOR_BLACK, IDM_COLOR_WHITE,   /* order matches ArrowColor */
    IDM_AUTOSTART,
    IDM_TEST,
    IDM_EXIT,
};

int tray_add(const App *a);                 /* 0 on success */
void tray_remove(const App *a);
void tray_balloon(const App *a, const wchar_t *title, const wchar_t *text);
void tray_show_menu(const App *a);          /* result arrives as WM_COMMAND */

#endif
