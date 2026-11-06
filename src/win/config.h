/* Settings persisted under HKCU, plus the "start with Windows" entry. */
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int enabled;
    int sensitivity;   /* Sensitivity, unvalidated: clamp before use */
    int size;          /* CursorSize, unvalidated: clamp before use */
    int color;         /* ArrowColor, unvalidated: clamp before use */
    int welcomed;      /* first-run welcome already shown */
} Config;

Config config_load(void);
int config_save(const Config *c);           /* 0 on success */

/* "System cursor is currently hidden by us" crash-recovery flag. */
int cursor_flag_get(void);
int cursor_flag_set(int on);                /* 0 on success, verified */

int autostart_enabled(void);
int autostart_set(int on);                  /* 0 on success */
void autostart_refresh_path(void);          /* re-point to this exe if enabled */

#endif
