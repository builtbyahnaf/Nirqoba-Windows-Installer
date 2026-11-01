// =============================================================================
// Nirqoba Uninstaller - Modern Windows UI
// A sleek, minimalistic uninstaller for the Nirqoba compiler suite.
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ole32.lib")

namespace Color {
    static const D2D1_COLOR_F Background    = D2D1::ColorF(0x0D1B2A);
    static const D2D1_COLOR_F Panel         = D2D1::ColorF(0x0A1520);
    static const D2D1_COLOR_F Surface       = D2D1::ColorF(0x11243A);
    static const D2D1_COLOR_F SurfaceLight  = D2D1::ColorF(0x162F4F);
    static const D2D1_COLOR_F Indigo1       = D2D1::ColorF(0x4D4A9F);
    static const D2D1_COLOR_F Indigo2       = D2D1::ColorF(0x6A62B8);
    static const D2D1_COLOR_F Blue1         = D2D1::ColorF(0x7EA3E2);
    static const D2D1_COLOR_F Violet1       = D2D1::ColorF(0xA98CE8);
    static const D2D1_COLOR_F Violet2       = D2D1::ColorF(0x9A86E0);
    static const D2D1_COLOR_F White         = D2D1::ColorF(D2D1::ColorF::White);
    static const D2D1_COLOR_F TextPrimary   = D2D1::ColorF(0xF0F4FF);
    static const D2D1_COLOR_F TextSecondary = D2D1::ColorF(0x8A9FBF);
    static const D2D1_COLOR_F TextMuted     = D2D1::ColorF(0x4A6080);
    static const D2D1_COLOR_F Accent        = D2D1::ColorF(0x7A6FE0);
    static const D2D1_COLOR_F AccentBorder  = D2D1::ColorF(0x4A4A9A);
    static const D2D1_COLOR_F Separator     = D2D1::ColorF(0x1A2D45);
    static const D2D1_COLOR_F ProgressBg    = D2D1::ColorF(0x0F1E30);
    static const D2D1_COLOR_F Success       = D2D1::ColorF(0x5BC899);
    static const D2D1_COLOR_F Danger        = D2D1::ColorF(0xDF5252);
}

static const int WINDOW_W = 1000;
static const int WINDOW_H = 600;
static const int SIDEBAR_W = 280;
static const int TITLEBAR_H = 52;

static const int ID_BTN_UNINSTALL = 201;
static const int ID_BTN_CANCEL    = 202;
static const int ID_TIMER_ANIM    = 1;
static const UINT WM_UNINSTALL_PROGRESS = WM_USER + 10;
static const UINT WM_UNINSTALL_COMPLETE = WM_USER + 11;

enum UninstallPage {
    PAGE_CONFIRM = 0,
    PAGE_UNINSTALLING,
    PAGE_COMPLETE
};

static HWND g_hWnd = nullptr;
static HINSTANCE g_hInst = nullptr;
static ID2D1Factory* g_d2dFactory = nullptr;
static ID2D1HwndRenderTarget* g_renderTarget = nullptr;
static IDWriteFactory* g_dwFactory = nullptr;
static IWICImagingFactory* g_wicFactory = nullptr;

static ID2D1SolidColorBrush* g_brushWhite = nullptr;
static ID2D1SolidColorBrush* g_brushBg = nullptr;
static ID2D1SolidColorBrush* g_brushSurface = nullptr;
static ID2D1SolidColorBrush* g_brushSurfaceLight = nullptr;
static ID2D1SolidColorBrush* g_brushAccent = nullptr;
static ID2D1SolidColorBrush* g_brushTextPri = nullptr;
static ID2D1SolidColorBrush* g_brushTextSec = nullptr;
static ID2D1SolidColorBrush* g_brushTextMuted = nullptr;
static ID2D1SolidColorBrush* g_brushSeparator = nullptr;
static ID2D1SolidColorBrush* g_brushProgressBg = nullptr;
static ID2D1SolidColorBrush* g_brushSuccess = nullptr;
static ID2D1SolidColorBrush* g_brushPanel = nullptr;
static ID2D1SolidColorBrush* g_brushAccentBorder = nullptr;
static ID2D1SolidColorBrush* g_brushDanger = nullptr;
static ID2D1LinearGradientBrush* g_brushSidebarGrad = nullptr;
static ID2D1LinearGradientBrush* g_brushProgressGrad = nullptr;

static UninstallPage g_currentPage = PAGE_CONFIRM;
static float g_progress = 0.0f;
static float g_animT = 0.0f;
static std::wstring g_installPath;
static std::wstring g_statusText = L"Preparing uninstallation...";
static HWND g_hBtnAction = nullptr;
static HWND g_hBtnCancel = nullptr;
static HFONT g_hBtnFont = nullptr;
static bool g_hoverCloseBtn = false;
static bool g_hoverMinBtn   = false;

static D2D1_RECT_F RectF(float l, float t, float r, float b) { return D2D1::RectF(l, t, r, b); }
static D2D1_ROUNDED_RECT RoundedRect(float l, float t, float r, float b, float rx, float ry = -1.f) {
    D2D1_ROUNDED_RECT rr;
    rr.rect = D2D1::RectF(l, t, r, b);
    rr.radiusX = rx;
    rr.radiusY = (ry < 0) ? rx : ry;
    return rr;
}
static D2D1_ELLIPSE Ellipse(float cx, float cy, float rx, float ry) {
    D2D1_ELLIPSE e;
    e.point = D2D1::Point2F(cx, cy);
    e.radiusX = rx;
    e.radiusY = ry;
    return e;
}

// ---------------------------------------------------------------------------
// UAC Elevation Helpers
// ---------------------------------------------------------------------------
static bool IsElevated() {
    BOOL elevated = FALSE;
    HANDLE token  = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elev = {};
        DWORD cbSize = sizeof(elev);
        if (GetTokenInformation(token, TokenElevation, &elev, cbSize, &cbSize))
            elevated = elev.TokenIsElevated;
        CloseHandle(token);
    }
    return elevated != FALSE;
}

static bool RelaunchElevated() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize       = sizeof(sei);
    sei.lpVerb       = L"runas";
    sei.lpFile       = exePath;
    sei.nShow        = SW_NORMAL;
    sei.fMask        = SEE_MASK_NOASYNC;
    return ShellExecuteExW(&sei) != FALSE;
}

static bool RemoveFromPathSafely(const std::wstring& dirToRemove, bool perUser) {
    HKEY hRoot = perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    const wchar_t* subKey = perUser
        ? L"Environment"
        : L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(hRoot, subKey, 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;

    std::wstring normRemove = dirToRemove;
    while (!normRemove.empty() && (normRemove.back() == L'\\' || normRemove.back() == L'/'))
        normRemove.pop_back();

    DWORD type = REG_EXPAND_SZ;
    DWORD cbData = 0;
    if (RegQueryValueExW(hKey, L"Path", nullptr, &type, nullptr, &cbData) != ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"PATH", nullptr, &type, nullptr, &cbData);
    }
    if (cbData == 0) {
        RegCloseKey(hKey);
        return true;
    }

    std::vector<wchar_t> buffer(cbData / sizeof(wchar_t) + 2, 0);
    RegQueryValueExW(hKey, L"Path", nullptr, &type, (LPBYTE)buffer.data(), &cbData);
    std::wstring currentPath = buffer.data();

    std::wstring newPath;
    size_t start = 0;
    while (start < currentPath.size()) {
        size_t end = currentPath.find(L';', start);
        if (end == std::wstring::npos) end = currentPath.size();
        std::wstring part = currentPath.substr(start, end - start);
        std::wstring trimmed = part;
        while (!trimmed.empty() && trimmed.front() == L' ') trimmed.erase(0, 1);
        while (!trimmed.empty() && (trimmed.back() == L' ' || trimmed.back() == L'\\' || trimmed.back() == L'/'))
            trimmed.pop_back();

        if (!trimmed.empty() && _wcsicmp(trimmed.c_str(), normRemove.c_str()) != 0) {
            if (!newPath.empty()) newPath += L';';
            newPath += part;
        }
        start = end + 1;
    }

    DWORD writeType = (type == REG_SZ) ? REG_SZ : REG_EXPAND_SZ;
    RegSetValueExW(hKey, L"Path", 0, writeType,
        (const BYTE*)newPath.c_str(),
        (DWORD)((newPath.size() + 1) * sizeof(wchar_t)));

    RegCloseKey(hKey);

    DWORD_PTR dwResult = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, &dwResult);
    return true;
}

static void DeleteDirectoryRecursive(const std::wstring& path) {
    std::wstring search = path + L"\\*.*";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
            continue;
        std::wstring item = path + L"\\" + ffd.cFileName;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DeleteDirectoryRecursive(item);
            RemoveDirectoryW(item.c_str());
        } else {
            SetFileAttributesW(item.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(item.c_str());
        }
    } while (FindNextFileW(hFind, &ffd));
    FindClose(hFind);
}

static DWORD WINAPI UninstallWorkerThread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;
    Sleep(300);

    // 1. Remove PATH entries
    PostMessageW(hWnd, WM_UNINSTALL_PROGRESS, 20, (LPARAM)L"Cleaning environment PATH...");
    std::wstring binDir = g_installPath + L"\\bin";
    RemoveFromPathSafely(binDir, true);  // HKCU
    RemoveFromPathSafely(binDir, false); // HKLM
    Sleep(200);

    // 2. Remove Registry entries
    PostMessageW(hWnd, WM_UNINSTALL_PROGRESS, 45, (LPARAM)L"Removing registry uninstall keys...");
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Nirqoba");
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Nirqoba");
    Sleep(200);

    // 3. Remove files except uninstall.exe itself
    PostMessageW(hWnd, WM_UNINSTALL_PROGRESS, 70, (LPARAM)L"Removing compiler files and libraries...");
    DeleteDirectoryRecursive(g_installPath + L"\\bin");
    RemoveDirectoryW((g_installPath + L"\\bin").c_str());
    DeleteDirectoryRecursive(g_installPath + L"\\lib");
    RemoveDirectoryW((g_installPath + L"\\lib").c_str());
    DeleteDirectoryRecursive(g_installPath + L"\\toolchain");
    RemoveDirectoryW((g_installPath + L"\\toolchain").c_str());
    DeleteDirectoryRecursive(g_installPath + L"\\examples");
    RemoveDirectoryW((g_installPath + L"\\examples").c_str());
    DeleteDirectoryRecursive(g_installPath + L"\\docs");
    RemoveDirectoryW((g_installPath + L"\\docs").c_str());
    DeleteFileW((g_installPath + L"\\README.md").c_str());
    Sleep(300);

    PostMessageW(hWnd, WM_UNINSTALL_PROGRESS, 100, (LPARAM)L"Finalizing uninstallation...");
    Sleep(200);

    PostMessageW(hWnd, WM_UNINSTALL_COMPLETE, 0, 0);
    return 0;
}

static void ScheduleSelfDelete() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Launch background cmd to wait 1 second and remove install folder and this uninstaller
    wchar_t cmd[1024];
    swprintf_s(cmd, L"/c timeout /t 1 /nobreak >nul & del /f /q \"%ls\" & rd /s /q \"%ls\"",
        exePath, g_installPath.c_str());

    ShellExecuteW(nullptr, L"open", L"cmd.exe", cmd, nullptr, SW_HIDE);
}

static void DrawStyledButton(const DRAWITEMSTRUCT* item) {
    if (!item || !item->hwndItem) return;
    const bool primary = (item->CtlID == ID_BTN_UNINSTALL);
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;

    COLORREF fillColor = disabled ? RGB(45, 54, 70)
        : primary ? (pressed ? RGB(160, 40, 40) : RGB(190, 50, 50))
        : (pressed ? RGB(30, 48, 70) : RGB(20, 37, 56));
    COLORREF borderColor = disabled ? RGB(57, 67, 82)
        : primary ? RGB(220, 90, 90) : RGB(58, 78, 103);
    COLORREF textColor = disabled ? RGB(132, 145, 162) : RGB(242, 245, 255);

    if (g_currentPage == PAGE_COMPLETE && primary) {
        fillColor = pressed ? RGB(73, 63, 159) : RGB(101, 91, 206);
        borderColor = RGB(137, 127, 235);
    }

    HBRUSH fill = CreateSolidBrush(fillColor);
    HPEN border = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldBrush = SelectObject(item->hDC, fill);
    HGDIOBJ oldPen = SelectObject(item->hDC, border);
    RoundRect(item->hDC, item->rcItem.left, item->rcItem.top,
        item->rcItem.right, item->rcItem.bottom, 12, 12);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(fill);
    DeleteObject(border);

    wchar_t label[80] = {};
    GetWindowTextW(item->hwndItem, label, ARRAYSIZE(label));
    RECT textRect = item->rcItem;
    HFONT font = (HFONT)SendMessageW(item->hwndItem, WM_GETFONT, 0, 0);
    HGDIOBJ oldFont = font ? SelectObject(item->hDC, font) : nullptr;
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, textColor);
    DrawTextW(item->hDC, label, -1, &textRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (oldFont) SelectObject(item->hDC, oldFont);
}

static HRESULT CreateDeviceResources() {
    if (g_renderTarget) return S_OK;
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);

    HRESULT hr = g_d2dFactory->CreateHwndRenderTarget(props,
        D2D1::HwndRenderTargetProperties(g_hWnd, size), &g_renderTarget);

    if (SUCCEEDED(hr)) {
        auto* rt = g_renderTarget;
        rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

        rt->CreateSolidColorBrush(Color::White, &g_brushWhite);
        rt->CreateSolidColorBrush(Color::Background, &g_brushBg);
        rt->CreateSolidColorBrush(Color::Surface, &g_brushSurface);
        rt->CreateSolidColorBrush(Color::SurfaceLight, &g_brushSurfaceLight);
        rt->CreateSolidColorBrush(Color::Accent, &g_brushAccent);
        rt->CreateSolidColorBrush(Color::TextPrimary, &g_brushTextPri);
        rt->CreateSolidColorBrush(Color::TextSecondary, &g_brushTextSec);
        rt->CreateSolidColorBrush(Color::TextMuted, &g_brushTextMuted);
        rt->CreateSolidColorBrush(Color::Separator, &g_brushSeparator);
        rt->CreateSolidColorBrush(Color::ProgressBg, &g_brushProgressBg);
        rt->CreateSolidColorBrush(Color::Success, &g_brushSuccess);
        rt->CreateSolidColorBrush(Color::Panel, &g_brushPanel);
        rt->CreateSolidColorBrush(Color::AccentBorder, &g_brushAccentBorder);
        rt->CreateSolidColorBrush(Color::Danger, &g_brushDanger);

        D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, Color::Indigo1 },
            { 0.5f, D2D1::ColorF(0x1A2A5A) },
            { 1.0f, Color::Panel }
        };
        ID2D1GradientStopCollection* col = nullptr;
        rt->CreateGradientStopCollection(stops, 3, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, (float)WINDOW_H)),
            col, &g_brushSidebarGrad);
        if (col) col->Release();

        D2D1_GRADIENT_STOP pStops[] = {
            { 0.0f, Color::Indigo2 },
            { 0.5f, Color::Violet2 },
            { 1.0f, Color::Blue1 }
        };
        rt->CreateGradientStopCollection(pStops, 3, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(D2D1::Point2F(SIDEBAR_W + 40.f, 0), D2D1::Point2F((float)WINDOW_W - 40.f, 0)),
            col, &g_brushProgressGrad);
        if (col) col->Release();
    }
    return hr;
}

static void DiscardDeviceResources() {
    auto rel = [](IUnknown** p) { if (*p) { (*p)->Release(); *p = nullptr; } };
    rel((IUnknown**)&g_brushWhite);
    rel((IUnknown**)&g_brushBg);
    rel((IUnknown**)&g_brushSurface);
    rel((IUnknown**)&g_brushSurfaceLight);
    rel((IUnknown**)&g_brushAccent);
    rel((IUnknown**)&g_brushTextPri);
    rel((IUnknown**)&g_brushTextSec);
    rel((IUnknown**)&g_brushTextMuted);
    rel((IUnknown**)&g_brushSeparator);
    rel((IUnknown**)&g_brushProgressBg);
    rel((IUnknown**)&g_brushSuccess);
    rel((IUnknown**)&g_brushPanel);
    rel((IUnknown**)&g_brushAccentBorder);
    rel((IUnknown**)&g_brushDanger);
    rel((IUnknown**)&g_brushSidebarGrad);
    rel((IUnknown**)&g_brushProgressGrad);
    if (g_renderTarget) { g_renderTarget->Release(); g_renderTarget = nullptr; }
}

static void DrawSidebar(ID2D1RenderTarget* rt) {
    rt->FillRectangle(RectF(0, 0, (float)SIDEBAR_W, (float)WINDOW_H), g_brushSidebarGrad);

    // Subtle right border
    ID2D1SolidColorBrush* borderBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0x3A4A70, 0.5f), &borderBrush);
    rt->DrawLine(
        D2D1::Point2F((float)SIDEBAR_W - 1.f, 0),
        D2D1::Point2F((float)SIDEBAR_W - 1.f, (float)WINDOW_H),
        borderBrush, 1.f);
    borderBrush->Release();

    // Brand text
    IDWriteTextFormat* brandFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 26.f, L"en-us", &brandFmt);
    if (brandFmt) {
        brandFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        rt->DrawTextW(L"NIRQOBA", 7, brandFmt,
            RectF(0, 110.f, (float)SIDEBAR_W, 145.f), g_brushTextPri);
        brandFmt->Release();
    }

    IDWriteTextFormat* subFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 13.f, L"en-us", &subFmt);
    if (subFmt) {
        subFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        rt->DrawTextW(L"Uninstall Maintenance", 21, subFmt,
            RectF(0, 150.f, (float)SIDEBAR_W, 175.f), g_brushTextMuted);
        subFmt->Release();
    }

    rt->DrawLine(D2D1::Point2F(24.f, 210.f), D2D1::Point2F(SIDEBAR_W - 24.f, 210.f), g_brushSeparator, 1.f);

    const wchar_t* steps[] = { L"Confirmation", L"Uninstalling", L"Finished" };
    float stepY = 240.f;
    for (int i = 0; i < 3; i++) {
        bool isActive = (i == (int)g_currentPage);
        bool isDone = (i < (int)g_currentPage);
        float cx = 42.f;
        float cy = stepY + i * 50.f + 10.f;

        rt->FillEllipse(Ellipse(cx, cy, 9.f, 9.f), isActive ? g_brushAccent : isDone ? g_brushSuccess : g_brushPanel);
        rt->DrawEllipse(Ellipse(cx, cy, 9.f, 9.f), isActive ? g_brushWhite : g_brushSeparator, 1.5f);

        IDWriteTextFormat* stFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            isActive ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            14.f, L"en-us", &stFmt);
        if (stFmt) {
            stFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            rt->DrawTextW(steps[i], (UINT32)wcslen(steps[i]), stFmt,
                RectF(64.f, cy - 12.f, SIDEBAR_W - 10.f, cy + 12.f),
                isActive ? g_brushTextPri : isDone ? g_brushSuccess : g_brushTextMuted);
            stFmt->Release();
        }
    }
}

// Draw the custom title bar
static void DrawTitleBar(ID2D1RenderTarget* rt) {
    float contentL = (float)SIDEBAR_W;
    float contentR = (float)WINDOW_W;

    // Title bar background
    rt->FillRectangle(RectF(contentL, 0, contentR, (float)TITLEBAR_H), g_brushPanel);

    // Bottom border of title bar
    rt->DrawLine(
        D2D1::Point2F(contentL, (float)TITLEBAR_H - 1.f),
        D2D1::Point2F(contentR, (float)TITLEBAR_H - 1.f),
        g_brushSeparator, 1.f);

    // Title text
    const wchar_t* pageTitles[] = {
        L"Uninstall Nirqoba",
        L"Uninstalling Nirqoba",
        L"Uninstallation Complete"
    };

    IDWriteTextFormat* fmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 15.f, L"en-us", &fmt);
    if (fmt) {
        fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        rt->DrawTextW(pageTitles[(int)g_currentPage],
            (UINT32)wcslen(pageTitles[(int)g_currentPage]),
            fmt,
            RectF(contentL + 24.f, 0, contentR - 120.f, (float)TITLEBAR_H),
            g_brushTextSec);
        fmt->Release();
    }

    // Modern window control buttons: Minimize [-] and Close [X]
    float btnW = 48.f;
    float btnH = (float)TITLEBAR_H;

    // Minimize button rect: [contentR - 96, contentR - 48]
    float minL = contentR - btnW * 2.f;
    float minR = minL + btnW;
    if (g_hoverMinBtn) {
        rt->FillRectangle(RectF(minL, 0, minR, btnH), g_brushSurfaceLight);
    }
    ID2D1SolidColorBrush* minGlyphBrush = g_hoverMinBtn ? g_brushWhite : g_brushTextSec;
    float minMidX = (minL + minR) / 2.f;
    float minMidY = btnH / 2.f;
    rt->DrawLine(D2D1::Point2F(minMidX - 7.f, minMidY), D2D1::Point2F(minMidX + 7.f, minMidY), minGlyphBrush, 2.0f);

    // Close button rect: [contentR - 48, contentR]
    float closeL = contentR - btnW;
    float closeR = contentR;
    if (g_hoverCloseBtn) {
        ID2D1SolidColorBrush* closeHoverBg = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(0xE81123), &closeHoverBg);
        rt->FillRectangle(RectF(closeL, 0, closeR, btnH), closeHoverBg);
        closeHoverBg->Release();
    }
    ID2D1SolidColorBrush* closeGlyphBrush = g_hoverCloseBtn ? g_brushWhite : g_brushTextSec;
    float closeMidX = (closeL + closeR) / 2.f;
    float closeMidY = btnH / 2.f;
    float xRadius = 6.f;
    rt->DrawLine(D2D1::Point2F(closeMidX - xRadius, closeMidY - xRadius),
                 D2D1::Point2F(closeMidX + xRadius, closeMidY + xRadius), closeGlyphBrush, 2.0f);
    rt->DrawLine(D2D1::Point2F(closeMidX + xRadius, closeMidY - xRadius),
                 D2D1::Point2F(closeMidX - xRadius, closeMidY + xRadius), closeGlyphBrush, 2.0f);
}

static void OnRender() {
    if (FAILED(CreateDeviceResources())) return;
    g_renderTarget->BeginDraw();
    g_renderTarget->Clear(Color::Background);

    DrawSidebar(g_renderTarget);
    DrawTitleBar(g_renderTarget);

    float cL = SIDEBAR_W + 48.f;
    float cR = (float)WINDOW_W - 48.f;
    float cT = (float)TITLEBAR_H + 32.f;

    IDWriteTextFormat* titleFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 30.f, L"en-us", &titleFmt);

    if (g_currentPage == PAGE_CONFIRM) {
        if (titleFmt) {
            g_renderTarget->DrawTextW(L"Uninstall Nirqoba", 17, titleFmt,
                RectF(cL, cT, cR, cT + 42.f), g_brushTextPri);
        }

        IDWriteTextFormat* subFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 14.f, L"en-us", &subFmt);
        if (subFmt) {
            g_renderTarget->DrawTextW(L"Are you sure you want to completely remove Nirqoba?",
                49, subFmt, RectF(cL, cT + 46.f, cR, cT + 70.f), g_brushTextSec);
            subFmt->Release();
        }

        g_renderTarget->DrawLine(D2D1::Point2F(cL, cT + 80.f), D2D1::Point2F(cR, cT + 80.f), g_brushSeparator, 1.f);

        float cardT = cT + 98.f;
        float cardB = cardT + 120.f;
        g_renderTarget->FillRoundedRectangle(RoundedRect(cL, cardT, cR, cardB, 8.f), g_brushSurface);
        g_renderTarget->DrawRoundedRectangle(RoundedRect(cL, cardT, cR, cardB, 8.f), g_brushAccentBorder, 1.f);

        IDWriteTextFormat* cardFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 13.5f, L"en-us", &cardFmt);
        if (cardFmt) {
            g_renderTarget->DrawTextW(L"Nirqoba will be uninstalled from the following folder:",
                52, cardFmt, RectF(cL + 20.f, cardT + 16.f, cR - 20.f, cardT + 38.f), g_brushTextMuted);

            g_renderTarget->DrawTextW(g_installPath.c_str(), (UINT32)g_installPath.size(), cardFmt,
                RectF(cL + 20.f, cardT + 42.f, cR - 20.f, cardT + 66.f), g_brushTextPri);

            g_renderTarget->DrawTextW(L"All binaries, standard libraries, and toolchain links will be removed.",
                68, cardFmt, RectF(cL + 20.f, cardT + 78.f, cR - 20.f, cardT + 102.f), g_brushDanger);
            cardFmt->Release();
        }
    }
    else if (g_currentPage == PAGE_UNINSTALLING) {
        if (titleFmt) {
            titleFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_renderTarget->DrawTextW(L"Uninstalling Nirqoba", 20, titleFmt,
                RectF(cL, cT + 50.f, cR, cT + 96.f), g_brushTextPri);
        }

        IDWriteTextFormat* statusFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 14.f, L"en-us", &statusFmt);
        if (statusFmt) {
            statusFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_renderTarget->DrawTextW(g_statusText.c_str(), (UINT32)g_statusText.size(), statusFmt,
                RectF(cL, cT + 106.f, cR, cT + 134.f), g_brushTextSec);
            statusFmt->Release();
        }

        float barL = cL + 40.f;
        float barR = cR - 40.f;
        float barT = cT + 154.f;
        float barB = barT + 10.f;
        g_renderTarget->FillRoundedRectangle(RoundedRect(barL, barT, barR, barB, 5.f), g_brushProgressBg);
        float fillW = (barR - barL) * (g_progress / 100.f);
        if (fillW > 10.f) {
            g_brushProgressGrad->SetStartPoint(D2D1::Point2F(barL, 0));
            g_brushProgressGrad->SetEndPoint(D2D1::Point2F(barR, 0));
            g_renderTarget->FillRoundedRectangle(RoundedRect(barL, barT, barL + fillW, barB, 5.f), g_brushProgressGrad);
        }
    }
    else if (g_currentPage == PAGE_COMPLETE) {
        float cx = (cL + cR) / 2.f;
        float cy = cT + 80.f;
        g_renderTarget->FillEllipse(Ellipse(cx, cy, 32.f, 32.f), g_brushPanel);
        g_renderTarget->DrawEllipse(Ellipse(cx, cy, 32.f, 32.f), g_brushSuccess, 2.f);

        ID2D1PathGeometry* checkGeo = nullptr;
        g_d2dFactory->CreatePathGeometry(&checkGeo);
        if (checkGeo) {
            ID2D1GeometrySink* sink = nullptr;
            checkGeo->Open(&sink);
            sink->BeginFigure(D2D1::Point2F(cx - 11.f, cy), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(D2D1::Point2F(cx - 3.f, cy + 8.f));
            sink->AddLine(D2D1::Point2F(cx + 13.f, cy - 8.f));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            sink->Release();
            g_renderTarget->DrawGeometry(checkGeo, g_brushSuccess, 2.5f);
            checkGeo->Release();
        }

        if (titleFmt) {
            titleFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_renderTarget->DrawTextW(L"Uninstallation Complete", 23, titleFmt,
                RectF(cL, cy + 46.f, cR, cy + 90.f), g_brushTextPri);
        }

        IDWriteTextFormat* subFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 14.f, L"en-us", &subFmt);
        if (subFmt) {
            subFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_renderTarget->DrawTextW(L"Nirqoba Compiler Suite has been removed from your computer.",
                58, subFmt, RectF(cL, cy + 96.f, cR, cy + 124.f), g_brushTextSec);
            subFmt->Release();
        }
    }
    if (titleFmt) titleFmt->Release();

    // Button bar background
    float barY = (float)WINDOW_H - 74.f;
    g_renderTarget->FillRectangle(RectF((float)SIDEBAR_W, barY, (float)WINDOW_W, (float)WINDOW_H), g_brushPanel);
    g_renderTarget->DrawLine(D2D1::Point2F((float)SIDEBAR_W, barY), D2D1::Point2F((float)WINDOW_W, barY), g_brushSeparator, 1.f);

    HRESULT hr = g_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) DiscardDeviceResources();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        g_hWnd = hWnd;
        MARGINS m = { 1, 1, 1, 1 };
        DwmExtendFrameIntoClientArea(hWnd, &m);
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hWnd, 20, &darkMode, sizeof(darkMode));
        DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
        DwmSetWindowAttribute(hWnd, 33, &corner, sizeof(corner));

        // Get directory of uninstall.exe as install path
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        PathRemoveFileSpecW(exePath);
        g_installPath = exePath;

        g_hBtnFont = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        float barY = (float)WINDOW_H - 74.f;
        g_hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            SIDEBAR_W + 32, (int)barY + 15, 108, 44,
            hWnd, (HMENU)(INT_PTR)ID_BTN_CANCEL, g_hInst, nullptr);

        g_hBtnAction = CreateWindowExW(0, L"BUTTON", L"Uninstall",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            WINDOW_W - 156, (int)barY + 15, 124, 44,
            hWnd, (HMENU)(INT_PTR)ID_BTN_UNINSTALL, g_hInst, nullptr);

        if (g_hBtnCancel) SendMessageW(g_hBtnCancel, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
        if (g_hBtnAction) SendMessageW(g_hBtnAction, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

        SetTimer(hWnd, ID_TIMER_ANIM, 16, nullptr);
        return 0;
    }

    case WM_NCHITTEST:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);

        // If mouse is over minimize or close buttons, return HTCLIENT so click & hover work
        if (pt.y >= 0 && pt.y < TITLEBAR_H && pt.x >= (float)WINDOW_W - 96.f) {
            return HTCLIENT;
        }

        // Drag along title bar area
        if (pt.y >= 0 && pt.y < TITLEBAR_H) {
            return HTCAPTION;
        }

        return HTCLIENT;
    }

    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        float btnW = 48.f;
        float closeL = (float)WINDOW_W - btnW;
        float minL   = (float)WINDOW_W - btnW * 2.f;

        if (pt.y >= 0 && pt.y <= TITLEBAR_H) {
            if (pt.x >= closeL && pt.x <= (float)WINDOW_W) {
                SendMessageW(hWnd, WM_CLOSE, 0, 0);
                return 0;
            }
            if (pt.x >= minL && pt.x < closeL) {
                ShowWindow(hWnd, SW_MINIMIZE);
                return 0;
            }
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        float btnW = 48.f;
        float closeL = (float)WINDOW_W - btnW;
        float minL   = (float)WINDOW_W - btnW * 2.f;

        bool newHoverClose = (pt.y >= 0 && pt.y <= TITLEBAR_H && pt.x >= closeL && pt.x <= (float)WINDOW_W);
        bool newHoverMin   = (pt.y >= 0 && pt.y <= TITLEBAR_H && pt.x >= minL && pt.x < closeL);

        if (newHoverClose != g_hoverCloseBtn || newHoverMin != g_hoverMinBtn) {
            g_hoverCloseBtn = newHoverClose;
            g_hoverMinBtn = newHoverMin;
            RECT rcTitle = { (int)minL, 0, (int)WINDOW_W, TITLEBAR_H };
            InvalidateRect(hWnd, &rcTitle, FALSE);
        }

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
    {
        if (g_hoverCloseBtn || g_hoverMinBtn) {
            g_hoverCloseBtn = false;
            g_hoverMinBtn = false;
            RECT rcTitle = { (int)(WINDOW_W - 96), 0, (int)WINDOW_W, TITLEBAR_H };
            InvalidateRect(hWnd, &rcTitle, FALSE);
        }
        break;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER_ANIM) {
            g_animT += 0.016f;
            if (g_currentPage == PAGE_UNINSTALLING) InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;

    case WM_UNINSTALL_PROGRESS:
        g_progress = (float)wParam;
        if (lParam) g_statusText = (const wchar_t*)lParam;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;

    case WM_UNINSTALL_COMPLETE:
        g_currentPage = PAGE_COMPLETE;
        ShowWindow(g_hBtnCancel, SW_HIDE);
        SetWindowTextW(g_hBtnAction, L"Close");
        EnableWindow(g_hBtnAction, TRUE);
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;

    case WM_DRAWITEM:
        DrawStyledButton(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == ID_BTN_CANCEL) {
            SendMessageW(hWnd, WM_CLOSE, 0, 0);
        } else if (id == ID_BTN_UNINSTALL) {
            if (g_currentPage == PAGE_CONFIRM) {
                g_currentPage = PAGE_UNINSTALLING;
                EnableWindow(g_hBtnAction, FALSE);
                ShowWindow(g_hBtnCancel, SW_HIDE);
                InvalidateRect(hWnd, nullptr, FALSE);
                CreateThread(nullptr, 0, UninstallWorkerThread, hWnd, 0, nullptr);
            } else if (g_currentPage == PAGE_COMPLETE) {
                SendMessageW(hWnd, WM_CLOSE, 0, 0);
            }
        }
        return 0;
    }

    case WM_CLOSE:
        if (g_currentPage == PAGE_UNINSTALLING) {
            // Cannot abort while uninstallation is actively modifying files
            return 0;
        }
        if (g_currentPage == PAGE_CONFIRM) {
            if (MessageBoxW(hWnd,
                L"Are you sure you want to cancel uninstallation?",
                L"Cancel Uninstallation",
                MB_YESNO | MB_ICONQUESTION) != IDYES)
                return 0;
        }
        AnimateWindow(hWnd, 140, AW_BLEND | AW_HIDE);
        DestroyWindow(hWnd);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        OnRender();
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        KillTimer(hWnd, ID_TIMER_ANIM);
        DiscardDeviceResources();
        if (g_hBtnFont) DeleteObject(g_hBtnFont);
        if (g_currentPage == PAGE_COMPLETE) {
            ScheduleSelfDelete();
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;

    // Check UAC elevation
    if (!IsElevated()) {
        if (RelaunchElevated()) return 0;
        MessageBoxW(nullptr,
            L"Nirqoba Uninstaller requires administrator privileges to clean toolchains and system PATH.\n\n"
            L"Please accept the User Account Control prompt to continue.",
            L"Nirqoba Uninstaller — Administrator Privileges Required",
            MB_OK | MB_ICONWARNING | MB_TASKMODAL);
        return 1;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_dwFactory);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"NirqobaUninstallerWnd";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);

    int sx = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_H) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"NirqobaUninstallerWnd",
        L"Nirqoba Uninstaller",
        WS_POPUP | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        sx, sy, WINDOW_W, WINDOW_H,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 1;

    UpdateWindow(hWnd);
    AnimateWindow(hWnd, 180, AW_BLEND | AW_ACTIVATE);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return (int)msg.wParam;
}
