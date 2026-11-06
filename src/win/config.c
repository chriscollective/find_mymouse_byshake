#include "config.h"

#include "app.h"
#include "settings.h"

#include <stdio.h>
#include <wchar.h>

#define CFG_KEY   L"Software\\find_mymouse_byshake"
#define RUN_KEY   L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RUN_VALUE APP_NAME
#define PATH_CAP  1024

static int read_dword(const wchar_t *name, int fallback)
{
    DWORD v = 0, sz = sizeof v;
    LONG r = RegGetValueW(HKEY_CURRENT_USER, CFG_KEY, name, RRF_RT_REG_DWORD, NULL, &v, &sz);
    return r == ERROR_SUCCESS ? (int)v : fallback;
}

static LONG write_dword(HKEY key, const wchar_t *name, int value)
{
    DWORD v = (DWORD)value;
    return RegSetValueExW(key, name, 0, REG_DWORD, (const BYTE *)&v, sizeof v);
}

Config config_load(void)
{
    Config c;
    c.enabled = read_dword(L"Enabled", 1) != 0;
    c.sensitivity = read_dword(L"Sensitivity", SENS_MEDIUM);
    c.size = read_dword(L"Size", SIZE_MEDIUM);
    c.color = read_dword(L"Color", ARROW_WHITE);
    c.welcomed = read_dword(L"Welcomed", 0) != 0;
    return c;
}

int config_save(const Config *c)
{
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, CFG_KEY, 0, NULL, 0, KEY_SET_VALUE,
                        NULL, &key, NULL) != ERROR_SUCCESS) {
        return -1;
    }
    LONG r = write_dword(key, L"Enabled", c->enabled);
    if (r == ERROR_SUCCESS) r = write_dword(key, L"Sensitivity", c->sensitivity);
    if (r == ERROR_SUCCESS) r = write_dword(key, L"Size", c->size);
    if (r == ERROR_SUCCESS) r = write_dword(key, L"Color", c->color);
    if (r == ERROR_SUCCESS) r = write_dword(key, L"Welcomed", c->welcomed);
    RegCloseKey(key);
    return r == ERROR_SUCCESS ? 0 : -1;
}

int cursor_flag_get(void)
{
    return read_dword(L"CursorHidden", 0) != 0;
}

int cursor_flag_set(int on)
{
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, CFG_KEY, 0, NULL, 0, KEY_SET_VALUE,
                        NULL, &key, NULL) != ERROR_SUCCESS) {
        return -1;
    }
    /* No RegFlushKey: a killed process never loses registry writes (the
     * kernel owns them), and after an OS crash the cursors reset anyway. */
    LONG r = write_dword(key, L"CursorHidden", on);
    RegCloseKey(key);
    if (r != ERROR_SUCCESS) return -1;
    return cursor_flag_get() == (on != 0) ? 0 : -1; /* read back to be sure */
}

/* "C:\path\to\find_mymouse_byshake.exe" including the quotes. */
static int exe_command(wchar_t *buf, size_t cap)
{
    wchar_t path[PATH_CAP];
    DWORD n = GetModuleFileNameW(NULL, path, PATH_CAP);
    if (n == 0 || n >= PATH_CAP) return -1;
    int w = _snwprintf(buf, cap, L"\"%ls\"", path);
    return (w > 0 && (size_t)w < cap) ? 0 : -1;
}

int autostart_enabled(void)
{
    return RegGetValueW(HKEY_CURRENT_USER, RUN_KEY, RUN_VALUE, RRF_RT_REG_SZ,
                        NULL, NULL, NULL) == ERROR_SUCCESS;
}

static LONG set_run_value(HKEY key)
{
    wchar_t cmd[PATH_CAP + 4];
    if (exe_command(cmd, PATH_CAP + 4) != 0) return ERROR_BAD_PATHNAME;
    DWORD bytes = (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t));
    return RegSetValueExW(key, RUN_VALUE, 0, REG_SZ, (const BYTE *)cmd, bytes);
}

int autostart_set(int on)
{
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, NULL, 0, KEY_SET_VALUE,
                        NULL, &key, NULL) != ERROR_SUCCESS) {
        return -1;
    }
    LONG r;
    if (on) {
        r = set_run_value(key);
    } else {
        r = RegDeleteValueW(key, RUN_VALUE);
        if (r == ERROR_FILE_NOT_FOUND) r = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return r == ERROR_SUCCESS ? 0 : -1;
}

void autostart_refresh_path(void)
{
    if (autostart_enabled()) autostart_set(1);
}
