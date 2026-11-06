#include "tray.h"

#include "config.h"
#include "resource.h"

#include <shellapi.h>
#include <string.h>

#define TRAY_UID 1

static void base_nid(const App *a, NOTIFYICONDATAW *nid)
{
    memset(nid, 0, sizeof *nid);
    nid->cbSize = sizeof *nid;
    nid->hWnd = a->hwnd;
    nid->uID = TRAY_UID;
}

int tray_add(const App *a)
{
    NOTIFYICONDATAW nid;
    base_nid(a, &nid);
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP_TRAY;
    nid.hIcon = (HICON)LoadImageW(a->inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON), 0);
    lstrcpynW(nid.szTip, APP_TITLE, (int)(sizeof nid.szTip / sizeof nid.szTip[0]));
    BOOL ok = Shell_NotifyIconW(NIM_ADD, &nid);
    if (nid.hIcon) DestroyIcon(nid.hIcon);  /* the shell keeps its own copy */
    return ok ? 0 : -1;
}

void tray_remove(const App *a)
{
    NOTIFYICONDATAW nid;
    base_nid(a, &nid);
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void tray_balloon(const App *a, const wchar_t *title, const wchar_t *text)
{
    NOTIFYICONDATAW nid;
    base_nid(a, &nid);
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    lstrcpynW(nid.szInfoTitle, title, (int)(sizeof nid.szInfoTitle / sizeof nid.szInfoTitle[0]));
    lstrcpynW(nid.szInfo, text, (int)(sizeof nid.szInfo / sizeof nid.szInfo[0]));
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

static UINT checked(int on)
{
    return MF_STRING | (on ? MF_CHECKED : MF_UNCHECKED);
}

static HMENU build_sensitivity_menu(const App *a)
{
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_SENS_LOW, L"低（要晃得比較大）");
    AppendMenuW(m, MF_STRING, IDM_SENS_MEDIUM, L"中（建議）");
    AppendMenuW(m, MF_STRING, IDM_SENS_HIGH, L"高（輕輕晃就觸發）");
    CheckMenuRadioItem(m, IDM_SENS_LOW, IDM_SENS_HIGH,
                       IDM_SENS_LOW + (UINT)a->sens, MF_BYCOMMAND);
    return m;
}

static HMENU build_size_menu(const App *a)
{
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_SIZE_SMALL, L"小（3 倍）");
    AppendMenuW(m, MF_STRING, IDM_SIZE_MEDIUM, L"中（5 倍）");
    AppendMenuW(m, MF_STRING, IDM_SIZE_LARGE, L"大（7 倍）");
    CheckMenuRadioItem(m, IDM_SIZE_SMALL, IDM_SIZE_LARGE,
                       IDM_SIZE_SMALL + (UINT)a->size, MF_BYCOMMAND);
    return m;
}

static HMENU build_color_menu(const App *a)
{
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_COLOR_WHITE, L"白底黑邊");
    AppendMenuW(m, MF_STRING, IDM_COLOR_BLACK, L"黑底白邊");
    CheckMenuRadioItem(m, IDM_COLOR_BLACK, IDM_COLOR_WHITE,
                       IDM_COLOR_BLACK + (UINT)a->color, MF_BYCOMMAND);
    return m;
}

static HMENU build_menu(const App *a)
{
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, checked(a->enabled), IDM_ENABLE, L"啟用搖晃放大游標");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)build_sensitivity_menu(a), L"靈敏度");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)build_size_menu(a), L"放大倍率");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)build_color_menu(a), L"游標顏色");
    AppendMenuW(m, checked(autostart_enabled()), IDM_AUTOSTART, L"開機自動啟動");
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_TEST, L"試試放大效果");
    AppendMenuW(m, MF_STRING, IDM_EXIT, L"結束");
    return m;
}

void tray_show_menu(const App *a)
{
    POINT pt;
    if (!GetCursorPos(&pt)) return;
    HMENU m = build_menu(a);
    SetForegroundWindow(a->hwnd);   /* so the menu closes on outside click */
    TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, a->hwnd, NULL);
    PostMessageW(a->hwnd, WM_NULL, 0, 0);
    DestroyMenu(m);                 /* also destroys the submenus */
}
