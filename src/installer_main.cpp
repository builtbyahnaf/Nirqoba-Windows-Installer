// =============================================================================
// Nirqoba Installer - Modern Windows UI
// A sleek, minimalistic installer for the Nirqoba compiler.
//
// Design language:
//   - Dark navy background (#0D1B2A) with indigo/violet/blue brand accents
//   - Frameless window with custom title bar and drop shadow
//   - Multi-page wizard: Welcome → License → Install Path → Installing → Finish
//   - Custom-drawn progress bar, smooth gradient side panel
//   - Direct2D + DirectWrite for all rendering
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
#include <commdlg.h>
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
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

// ---------------------------------------------------------------------------
// Constants & Color palette (from brand assets)
// ---------------------------------------------------------------------------
namespace Color {
    // Backgrounds
    static const D2D1_COLOR_F Background    = D2D1::ColorF(0x0D1B2A);   // Deep navy
    static const D2D1_COLOR_F Panel         = D2D1::ColorF(0x0A1520);   // Slightly darker panel
    static const D2D1_COLOR_F Surface       = D2D1::ColorF(0x11243A);   // Card/surface
    static const D2D1_COLOR_F SurfaceLight  = D2D1::ColorF(0x162F4F);   // Hover surface

    // Brand gradients (from SVG)
    static const D2D1_COLOR_F Indigo1       = D2D1::ColorF(0x4D4A9F);   // Deep indigo
    static const D2D1_COLOR_F Indigo2       = D2D1::ColorF(0x6A62B8);   // Mid indigo
    static const D2D1_COLOR_F Blue1         = D2D1::ColorF(0x7EA3E2);   // Periwinkle blue
    static const D2D1_COLOR_F Violet1       = D2D1::ColorF(0xA98CE8);   // Soft violet
    static const D2D1_COLOR_F Violet2       = D2D1::ColorF(0x9A86E0);   // Mid violet
    static const D2D1_COLOR_F SteelBlue     = D2D1::ColorF(0x5A92D2);   // Steel blue
    static const D2D1_COLOR_F Lavender      = D2D1::ColorF(0xC0A8F4);   // Lavender

    // Text
    static const D2D1_COLOR_F White         = D2D1::ColorF(D2D1::ColorF::White);
    static const D2D1_COLOR_F TextPrimary   = D2D1::ColorF(0xF0F4FF);   // Near-white
    static const D2D1_COLOR_F TextSecondary = D2D1::ColorF(0x8A9FBF);   // Muted blue-grey
    static const D2D1_COLOR_F TextMuted     = D2D1::ColorF(0x4A6080);   // Dim text

    // Accents
    static const D2D1_COLOR_F Accent        = D2D1::ColorF(0x7A6FE0);   // Primary accent
    static const D2D1_COLOR_F AccentHover   = D2D1::ColorF(0x9085F0);   // Hover accent
    static const D2D1_COLOR_F AccentBorder  = D2D1::ColorF(0x4A4A9A);   // Subtle border
    static const D2D1_COLOR_F Separator     = D2D1::ColorF(0x1A2D45);   // Line separator
    static const D2D1_COLOR_F ProgressBg    = D2D1::ColorF(0x0F1E30);   // Progress track
    static const D2D1_COLOR_F Success       = D2D1::ColorF(0x5BC899);   // Green for finish
}

// Window dimensions
static const int WINDOW_W = 1550;
static const int WINDOW_H = 750;
static const int SIDEBAR_W = 300;
static const int TITLEBAR_H = 52;
static const int ID_TIMER_TRANSITION = 201;

// IDs
static const int ID_BTN_NEXT    = 101;
static const int ID_BTN_BACK    = 102;
static const int ID_BTN_CANCEL  = 103;
static const int ID_BTN_BROWSE  = 104;
static const int ID_EDIT_PATH   = 105;
static const int ID_CHK_DESKTOP = 106;
static const int ID_CHK_PATH    = 107;
static const int ID_TIMER_PROGRESS = 200;

// Resource IDs
static const int IDI_APPICON    = 1;

// Pages
enum InstallerPage {
    PAGE_WELCOME = 0,
    PAGE_LICENSE,
    PAGE_OPTIONS,
    PAGE_INSTALLING,
    PAGE_FINISH,
    PAGE_COUNT
};

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HWND g_hWnd             = nullptr;
static HINSTANCE g_hInst       = nullptr;

// D2D / DWrite
static ID2D1Factory*            g_d2dFactory    = nullptr;
static ID2D1HwndRenderTarget*   g_renderTarget  = nullptr;
static IDWriteFactory*          g_dwFactory     = nullptr;
static IWICImagingFactory*      g_wicFactory    = nullptr;

// Text formats
static IDWriteTextFormat* g_fmtTitle    = nullptr;  // 28pt, Light
static IDWriteTextFormat* g_fmtSubtitle = nullptr;  // 14pt, Regular
static IDWriteTextFormat* g_fmtBody     = nullptr;  // 11pt, Regular
static IDWriteTextFormat* g_fmtSmall    = nullptr;  // 9.5pt, Regular
static IDWriteTextFormat* g_fmtNav      = nullptr;  // 10pt, SemiBold
static IDWriteTextFormat* g_fmtSideNav  = nullptr;  // 10.5pt, Regular

// Brushes (created once)
static ID2D1SolidColorBrush* g_brushWhite       = nullptr;
static ID2D1SolidColorBrush* g_brushBg          = nullptr;
static ID2D1SolidColorBrush* g_brushSurface     = nullptr;
static ID2D1SolidColorBrush* g_brushAccent      = nullptr;
static ID2D1SolidColorBrush* g_brushTextPri     = nullptr;
static ID2D1SolidColorBrush* g_brushTextSec     = nullptr;
static ID2D1SolidColorBrush* g_brushTextMuted   = nullptr;
static ID2D1SolidColorBrush* g_brushSeparator   = nullptr;
static ID2D1SolidColorBrush* g_brushProgressBg  = nullptr;
static ID2D1SolidColorBrush* g_brushSuccess     = nullptr;
static ID2D1SolidColorBrush* g_brushPanel       = nullptr;
static ID2D1SolidColorBrush* g_brushSurfaceLight= nullptr;
static ID2D1SolidColorBrush* g_brushAccentBorder= nullptr;

// Gradient brushes
static ID2D1LinearGradientBrush* g_brushSidebarGrad  = nullptr;
static ID2D1LinearGradientBrush* g_brushProgressGrad = nullptr;
static ID2D1LinearGradientBrush* g_brushTitleGrad    = nullptr;
static ID2D1LinearGradientBrush* g_brushButtonGrad   = nullptr;
static ID2D1LinearGradientBrush* g_brushButtonHover  = nullptr;

// Logo bitmap
static ID2D1Bitmap* g_logoBitmap = nullptr;

// Installer state
static InstallerPage g_currentPage = PAGE_WELCOME;
static float         g_progress    = 0.0f;    // 0..1
static float         g_animT       = 0.0f;    // animation ticker
static bool          g_installing  = false;
static float         g_transitionProgress = 1.f;
static std::wstring  g_installPath;
static bool          g_addDesktopShortcut = true;
static bool          g_addToPath         = true;
static HWND          g_hEditPath  = nullptr;
static HWND          g_hBtnNext   = nullptr;
static HWND          g_hBtnBack   = nullptr;
static HWND          g_hBtnCancel = nullptr;
static HWND          g_hBtnBrowse = nullptr;
static HWND          g_hChkDesktop= nullptr;
static HWND          g_hChkPath   = nullptr;

// Drag state for frameless window
static bool  g_dragging   = false;
static POINT g_dragOffset = {0, 0};

// Button hover state tracking
static HWND g_hoveredBtn  = nullptr;

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------

static D2D1_RECT_F RectF(float l, float t, float r, float b) {
    return D2D1::RectF(l, t, r, b);
}
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

static void SafeRelease(IUnknown** pp) {
    if (pp && *pp) { (*pp)->Release(); *pp = nullptr; }
}

// ---------------------------------------------------------------------------
// WIC: Load PNG to ID2D1Bitmap
// ---------------------------------------------------------------------------
static HRESULT LoadBitmapFromFile(const wchar_t* path, ID2D1Bitmap** ppBitmap) {
    *ppBitmap = nullptr;
    if (!g_wicFactory || !g_renderTarget) return E_FAIL;

    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    HRESULT hr = g_wicFactory->CreateDecoderFromFilename(
        path, nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &decoder);

    if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr)) hr = g_wicFactory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) hr = converter->Initialize(
        frame, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.f,
        WICBitmapPaletteTypeMedianCut);
    if (SUCCEEDED(hr)) hr = g_renderTarget->CreateBitmapFromWicBitmap(converter, ppBitmap);

    if (converter) converter->Release();
    if (frame)     frame->Release();
    if (decoder)   decoder->Release();
    return hr;
}

// ---------------------------------------------------------------------------
// D2D resource creation / teardown
// ---------------------------------------------------------------------------
static void CreateBrushes() {
    auto* rt = g_renderTarget;
    auto mk = [&](D2D1_COLOR_F c, ID2D1SolidColorBrush** b) {
        rt->CreateSolidColorBrush(c, b);
    };
    mk(Color::White,        &g_brushWhite);
    mk(Color::Background,   &g_brushBg);
    mk(Color::Surface,      &g_brushSurface);
    mk(Color::Accent,       &g_brushAccent);
    mk(Color::TextPrimary,  &g_brushTextPri);
    mk(Color::TextSecondary,&g_brushTextSec);
    mk(Color::TextMuted,    &g_brushTextMuted);
    mk(Color::Separator,    &g_brushSeparator);
    mk(Color::ProgressBg,   &g_brushProgressBg);
    mk(Color::Success,      &g_brushSuccess);
    mk(Color::Panel,        &g_brushPanel);
    mk(Color::SurfaceLight, &g_brushSurfaceLight);
    mk(Color::AccentBorder, &g_brushAccentBorder);

    // Sidebar gradient (top indigo → bottom navy)
    {
        D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, Color::Indigo1 },
            { 0.5f, D2D1::ColorF(0x1A2A5A) },
            { 1.0f, Color::Panel }
        };
        ID2D1GradientStopCollection* col = nullptr;
        rt->CreateGradientStopCollection(stops, 3, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0, 0),
                D2D1::Point2F(0, (float)WINDOW_H)),
            col, &g_brushSidebarGrad);
        if (col) col->Release();
    }

    // Progress gradient
    {
        D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, Color::Indigo2 },
            { 0.5f, Color::Violet2 },
            { 1.0f, Color::Blue1 }
        };
        ID2D1GradientStopCollection* col = nullptr;
        rt->CreateGradientStopCollection(stops, 3, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(SIDEBAR_W + 40.f, 0),
                D2D1::Point2F((float)WINDOW_W - 40.f, 0)),
            col, &g_brushProgressGrad);
        if (col) col->Release();
    }

    // Title area gradient (left portion, subtle)
    {
        D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, D2D1::ColorF(0x1A2B50, 1.0f) },
            { 1.0f, Color::Background }
        };
        ID2D1GradientStopCollection* col = nullptr;
        rt->CreateGradientStopCollection(stops, 2, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(SIDEBAR_W + 0.f, 0),
                D2D1::Point2F(SIDEBAR_W + 200.f, 0)),
            col, &g_brushTitleGrad);
        if (col) col->Release();
    }

    // Button normal gradient
    {
        D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, D2D1::ColorF(0x6060C0) },
            { 1.0f, D2D1::ColorF(0x4848A0) }
        };
        ID2D1GradientStopCollection* col = nullptr;
        rt->CreateGradientStopCollection(stops, 2, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0, 450), D2D1::Point2F(0, 480)),
            col, &g_brushButtonGrad);
        if (col) col->Release();
    }

    // Button hover gradient
    {
        D2D1_GRADIENT_STOP stops[] = {
            { 0.0f, D2D1::ColorF(0x8080E0) },
            { 1.0f, D2D1::ColorF(0x6060C0) }
        };
        ID2D1GradientStopCollection* col = nullptr;
        rt->CreateGradientStopCollection(stops, 2, &col);
        rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0, 450), D2D1::Point2F(0, 480)),
            col, &g_brushButtonHover);
        if (col) col->Release();
    }
}

static void ReleaseBrushes() {
    auto rel = [](IUnknown** p){ if(*p){ (*p)->Release(); *p=nullptr; } };
    rel((IUnknown**)&g_brushWhite);
    rel((IUnknown**)&g_brushBg);
    rel((IUnknown**)&g_brushSurface);
    rel((IUnknown**)&g_brushAccent);
    rel((IUnknown**)&g_brushTextPri);
    rel((IUnknown**)&g_brushTextSec);
    rel((IUnknown**)&g_brushTextMuted);
    rel((IUnknown**)&g_brushSeparator);
    rel((IUnknown**)&g_brushProgressBg);
    rel((IUnknown**)&g_brushSuccess);
    rel((IUnknown**)&g_brushPanel);
    rel((IUnknown**)&g_brushSurfaceLight);
    rel((IUnknown**)&g_brushAccentBorder);
    rel((IUnknown**)&g_brushSidebarGrad);
    rel((IUnknown**)&g_brushProgressGrad);
    rel((IUnknown**)&g_brushTitleGrad);
    rel((IUnknown**)&g_brushButtonGrad);
    rel((IUnknown**)&g_brushButtonHover);
    rel((IUnknown**)&g_logoBitmap);
}

static HRESULT CreateDeviceResources() {
    if (g_renderTarget) return S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);

    HRESULT hr = g_d2dFactory->CreateHwndRenderTarget(
        props,
        D2D1::HwndRenderTargetProperties(g_hWnd, size),
        &g_renderTarget);

    if (SUCCEEDED(hr)) {
        g_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        g_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        CreateBrushes();

        // Load logo bitmap
        wchar_t logoPath[MAX_PATH];
        GetModuleFileNameW(g_hInst, logoPath, MAX_PATH);
        PathRemoveFileSpecW(logoPath);
        PathAppendW(logoPath, L"assets\\Nirqoba logo.png");
        LoadBitmapFromFile(logoPath, &g_logoBitmap);
        if (!g_logoBitmap) {
            // Try relative path from exe location
            PathRemoveFileSpecW(logoPath);
            PathAppendW(logoPath, L"..\\assets\\Nirqoba logo.png");
            LoadBitmapFromFile(logoPath, &g_logoBitmap);
        }
    }
    return hr;
}

static void DiscardDeviceResources() {
    ReleaseBrushes();
    if (g_renderTarget) { g_renderTarget->Release(); g_renderTarget = nullptr; }
}

// ---------------------------------------------------------------------------
// Draw helpers
// ---------------------------------------------------------------------------

// Draw the supplied Nirqoba mark at its original aspect ratio.
static void DrawNirqobaLogo(ID2D1RenderTarget* rt, float cx, float cy, float scale) {
    if (!g_logoBitmap) return;
    // Fit the supplied portrait mark into a square without stretching or cropping it.
    D2D1_SIZE_F source = g_logoBitmap->GetSize();
    float box = 292.f * scale;
    float factor = (source.width > 0.f && source.height > 0.f)
        ? std::min(box / source.width, box / source.height) : 0.f;
    float width = source.width * factor;
    float height = source.height * factor;
    rt->DrawBitmap(g_logoBitmap,
        RectF(cx - width / 2.f, cy - height / 2.f,
              cx + width / 2.f, cy + height / 2.f),
        1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}
// Draw the sidebar (always visible)
static void DrawSidebar(ID2D1RenderTarget* rt) {
    // Sidebar background gradient
    g_brushSidebarGrad->SetStartPoint(D2D1::Point2F(0, 0));
    g_brushSidebarGrad->SetEndPoint(D2D1::Point2F(0, (float)WINDOW_H));
    rt->FillRectangle(RectF(0, 0, SIDEBAR_W, (float)WINDOW_H), g_brushSidebarGrad);

    // Subtle right border
    ID2D1SolidColorBrush* borderBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0x3A4A70, 0.5f), &borderBrush);
    rt->DrawLine(
        D2D1::Point2F(SIDEBAR_W - 1.f, 0),
        D2D1::Point2F(SIDEBAR_W - 1.f, (float)WINDOW_H),
        borderBrush, 1.f);
    borderBrush->Release();

    // Logo (large, centered in top portion of sidebar)
    float logoSize = 120.f;
    float logoCx = SIDEBAR_W / 2.f;
    float logoCy = 140.f;
    DrawNirqobaLogo(rt, logoCx, logoCy, logoSize / 292.f);

    // "NIRQOBA" text below logo
    {
        IDWriteTextFormat* fmt = nullptr;
        g_dwFactory->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            24.f, L"en-us", &fmt);
        if (fmt) {
            fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            // Letter spacing
            fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            rt->DrawTextW(L"NIRQOBA",
                7, fmt,
                RectF(0, logoCy + logoSize * 0.65f, SIDEBAR_W, logoCy + logoSize * 0.65f + 28.f),
                g_brushTextPri);
            fmt->Release();
        }
    }

    // Horizontal separator
    float sepY = 278.f;
    rt->DrawLine(
        D2D1::Point2F(24.f, sepY),
        D2D1::Point2F(SIDEBAR_W - 24.f, sepY),
        g_brushSeparator, 1.f);

    // Navigation steps
    const wchar_t* stepNames[] = {
        L"Welcome", L"License", L"Options", L"Installing", L"Finish"
    };
    float stepStartY = 310.f;
    float stepSpacing = 56.f;

    for (int i = 0; i < PAGE_COUNT; i++) {
        float stepY = stepStartY + i * stepSpacing;
        bool isActive  = (i == (int)g_currentPage);
        bool isDone    = (i < (int)g_currentPage);
        bool isFuture  = (i > (int)g_currentPage);

        // Step circle
        float cx = 42.f;
        float cy2 = stepY + 8.f;
        float r   = 11.f;

        ID2D1SolidColorBrush* circleFill   = nullptr;
        ID2D1SolidColorBrush* circleStroke = nullptr;

        if (isActive) {
            rt->CreateSolidColorBrush(Color::Accent, &circleFill);
            rt->CreateSolidColorBrush(Color::Lavender, &circleStroke);
        } else if (isDone) {
            rt->CreateSolidColorBrush(D2D1::ColorF(0x3A7A50, 0.9f), &circleFill);
            rt->CreateSolidColorBrush(Color::Success, &circleStroke);
        } else {
            rt->CreateSolidColorBrush(D2D1::ColorF(0x1A2B45, 0.8f), &circleFill);
            rt->CreateSolidColorBrush(D2D1::ColorF(0x2A3B55, 1.0f), &circleStroke);
        }

        rt->FillEllipse(Ellipse(cx, cy2, r, r), circleFill);
        rt->DrawEllipse(Ellipse(cx, cy2, r, r), circleStroke, 1.5f);

        // Checkmark or number in circle
        if (isDone) {
            // Draw checkmark
            ID2D1SolidColorBrush* chkBrush = nullptr;
            rt->CreateSolidColorBrush(Color::White, &chkBrush);
            rt->DrawLine(D2D1::Point2F(cx-3.f, cy2),   D2D1::Point2F(cx-0.5f, cy2+3.f), chkBrush, 1.5f);
            rt->DrawLine(D2D1::Point2F(cx-0.5f, cy2+3.f), D2D1::Point2F(cx+4.f, cy2-3.f), chkBrush, 1.5f);
            chkBrush->Release();
        } else {
            // Draw step number
            wchar_t numStr[4];
            swprintf_s(numStr, L"%d", i + 1);
            IDWriteTextFormat* numFmt = nullptr;
            g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
                DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 11.f, L"en-us", &numFmt);
            if (numFmt) {
                numFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                numFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                auto numColor = isActive ? g_brushWhite : g_brushTextMuted;
                rt->DrawTextW(numStr, (UINT32)wcslen(numStr), numFmt,
                    RectF(cx-r, cy2-r, cx+r, cy2+r), numColor);
                numFmt->Release();
            }
        }

        // Step label
        IDWriteTextFormat* lFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            isActive ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            16.f, L"en-us", &lFmt);
        if (lFmt) {
            lFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            auto lColor = isActive ? g_brushTextPri : (isDone ? g_brushSuccess : g_brushTextMuted);
            rt->DrawTextW(stepNames[i], (UINT32)wcslen(stepNames[i]), lFmt,
                RectF(cx + r + 12.f, stepY, SIDEBAR_W - 18.f, stepY + 28.f),
                lColor);
            lFmt->Release();
        }

        // Connector line between steps
        if (i < PAGE_COUNT - 1) {
            float lineX = cx;
            float lineTop = stepY + 8.f + r + 2.f;
            float lineBot = stepY + stepSpacing - r - 2.f + 8.f;
            ID2D1SolidColorBrush* lineBrush = nullptr;
            rt->CreateSolidColorBrush(isDone ? D2D1::ColorF(0x2A5A38, 0.8f) : D2D1::ColorF(0x1A2B40, 1.f), &lineBrush);
            rt->DrawLine(D2D1::Point2F(lineX, lineTop), D2D1::Point2F(lineX, lineBot), lineBrush, 1.f);
            lineBrush->Release();
        }

        if (circleFill)   circleFill->Release();
        if (circleStroke) circleStroke->Release();
    }

    // Bottom branding
    {
        IDWriteTextFormat* fmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.f, L"en-us", &fmt);
        if (fmt) {
            fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            rt->DrawTextW(L"Copyright (2026-) Muhammad Tahmid Ahnaf",
                39, fmt,
                RectF(0, WINDOW_H - 34.f, SIDEBAR_W, (float)WINDOW_H - 12.f),
                g_brushTextMuted);
            fmt->Release();
        }
    }
}

// Draw the custom title bar
static void DrawTitleBar(ID2D1RenderTarget* rt) {
    float contentL = SIDEBAR_W + 0.f;
    float contentR = (float)WINDOW_W;

    // Title bar background
    rt->FillRectangle(RectF(contentL, 0, contentR, (float)TITLEBAR_H), g_brushPanel);

    // Bottom border of title bar
    rt->DrawLine(
        D2D1::Point2F(contentL, TITLEBAR_H - 1.f),
        D2D1::Point2F(contentR, TITLEBAR_H - 1.f),
        g_brushSeparator, 1.f);

    // Title text
    const wchar_t* pageTitle[] = {
        L"Welcome to Nirqoba Setup",
        L"License Agreement",
        L"Installation Options",
        L"Installing...",
        L"Setup Complete"
    };

    IDWriteTextFormat* fmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 17.f, L"en-us", &fmt);
    if (fmt) {
        fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        rt->DrawTextW(pageTitle[(int)g_currentPage],
            (UINT32)wcslen(pageTitle[(int)g_currentPage]),
            fmt,
            RectF(contentL + 20.f, 0, contentR - 120.f, (float)TITLEBAR_H),
            g_brushTextSec);
        fmt->Release();
    }

    // Window control buttons area (close / minimize) — drawn, but functional via WM_NCHITTEST
    float btnSize = TITLEBAR_H * 0.6f;
    float btnY = (TITLEBAR_H - btnSize) / 2.f;

    // Close button area
    float closeX = contentR - btnSize - 8.f;
    // Minimize button
    float minX   = closeX - btnSize - 4.f;

    // Minimal icon hints
    ID2D1SolidColorBrush* iconBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0x5A6A80, 0.7f), &iconBrush);

    // Minimize: dash
    rt->DrawLine(
        D2D1::Point2F(minX + btnSize*0.25f, TITLEBAR_H/2.f),
        D2D1::Point2F(minX + btnSize*0.75f, TITLEBAR_H/2.f),
        iconBrush, 1.5f);

    // Close: X
    rt->DrawLine(
        D2D1::Point2F(closeX + btnSize*0.25f, btnY + btnSize*0.25f),
        D2D1::Point2F(closeX + btnSize*0.75f, btnY + btnSize*0.75f),
        iconBrush, 1.5f);
    rt->DrawLine(
        D2D1::Point2F(closeX + btnSize*0.75f, btnY + btnSize*0.25f),
        D2D1::Point2F(closeX + btnSize*0.25f, btnY + btnSize*0.75f),
        iconBrush, 1.5f);

    iconBrush->Release();
}

// ---------------------------------------------------------------------------
// Page renderers
// ---------------------------------------------------------------------------

static void DrawPageWelcome(ID2D1RenderTarget* rt) {
    const float cL = SIDEBAR_W + 64.f;
    const float cR = (float)WINDOW_W - 64.f;
    const float cT = 118.f;

    IDWriteTextFormat* titleFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 44.f, L"en-us", &titleFmt);
    if (titleFmt) {
        rt->DrawTextW(L"Welcome to Nirqoba", 18, titleFmt,
            RectF(cL, cT, cR, cT + 60.f), g_brushTextPri);
        titleFmt->Release();
    }

    IDWriteTextFormat* subFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 23.f, L"en-us", &subFmt);
    if (subFmt) {
        rt->DrawTextW(L"Compiler setup for Windows", 26, subFmt,
            RectF(cL, cT + 72.f, cR, cT + 108.f), g_brushAccent);
        subFmt->Release();
    }

    const float cardT = cT + 146.f;
    const float cardB = cardT + 150.f;
    rt->FillRoundedRectangle(RoundedRect(cL, cardT, cR, cardB, 12.f), g_brushSurface);
    rt->DrawRoundedRectangle(RoundedRect(cL, cardT, cR, cardB, 12.f), g_brushAccentBorder, 1.f);

    IDWriteTextFormat* bodyFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 22.f, L"en-us", &bodyFmt);
    if (bodyFmt) {
        bodyFmt->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        bodyFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        const wchar_t* welcomeText =
            L"Choose an install location, review the MIT open-source license, "
            L"and follow the steps to install Nirqoba.";
        rt->DrawTextW(welcomeText, (UINT32)wcslen(welcomeText), bodyFmt,
            RectF(cL + 36.f, cardT + 24.f, cR - 36.f, cardB - 24.f),
            g_brushTextSec);
        bodyFmt->Release();
    }
}
static void DrawPageLicense(ID2D1RenderTarget* rt) {
    float cL = SIDEBAR_W + 40.f;
    float cR = (float)WINDOW_W - 40.f;
    float cT = TITLEBAR_H + 32.f;

    IDWriteTextFormat* titleFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 36.f, L"en-us", &titleFmt);
    if (titleFmt) {
        rt->DrawTextW(L"License Agreement", 17, titleFmt,
            RectF(cL, cT, cR, cT + 52.f), g_brushTextPri);
        titleFmt->Release();
    }

    IDWriteTextFormat* subFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.f, L"en-us", &subFmt);
    if (subFmt) {
        rt->DrawTextW(L"MIT License", 11, subFmt,
            RectF(cL, cT + 58.f, cR, cT + 86.f), g_brushTextSec);
        subFmt->Release();
    }

    rt->DrawLine(D2D1::Point2F(cL, cT + 96.f), D2D1::Point2F(cR, cT + 96.f),
        g_brushSeparator, 1.f);

    // License text box
    float boxT = cT + 108.f;
    float boxB = (float)WINDOW_H - 100.f;
    rt->FillRoundedRectangle(RoundedRect(cL, boxT, cR, boxB, 6.f), g_brushSurface);

    // Accent border
    rt->DrawRoundedRectangle(RoundedRect(cL, boxT, cR, boxB, 6.f), g_brushAccentBorder, 1.f);

    IDWriteTextFormat* licenseFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Cascadia Mono", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 15.f, L"en-us", &licenseFmt);
    if (!licenseFmt) {
        // Fallback to Consolas
        g_dwFactory->CreateTextFormat(L"Consolas", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 15.f, L"en-us", &licenseFmt);
    }
    if (licenseFmt) {
        licenseFmt->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        const wchar_t* licenseText =
            L"MIT License\n\n"
            L"Copyright (2026-) Muhammad Tahmid Ahnaf\n\n"
            L"Permission is hereby granted, free of charge, to any person obtaining a copy "
            L"of this software and associated documentation files (the \"Software\"), to deal "
            L"in the Software without restriction, including without limitation the rights "
            L"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
            L"copies of the Software, and to permit persons to whom the Software is "
            L"furnished to do so, subject to the following conditions:\n\n"
            L"The above copyright notice and this permission notice shall be included in all "
            L"copies or substantial portions of the Software.\n\n"
            L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
            L"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
            L"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
            L"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
            L"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
            L"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
            L"SOFTWARE.";

        // Clip to box
        D2D1_RECT_F clipRect = RectF(cL + 14.f, boxT + 12.f, cR - 14.f, boxB - 12.f);
        rt->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
        rt->DrawTextW(licenseText, (UINT32)wcslen(licenseText),
            licenseFmt, clipRect, g_brushTextSec);
        rt->PopAxisAlignedClip();
        licenseFmt->Release();
    }

}

static void DrawPageOptions(ID2D1RenderTarget* rt) {
    float cL = SIDEBAR_W + 40.f;
    float cR = (float)WINDOW_W - 40.f;
    float cT = TITLEBAR_H + 32.f;

    IDWriteTextFormat* titleFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 36.f, L"en-us", &titleFmt);
    if (titleFmt) {
        rt->DrawTextW(L"Installation Options", 20, titleFmt,
            RectF(cL, cT, cR, cT + 52.f), g_brushTextPri);
        titleFmt->Release();
    }

    IDWriteTextFormat* subFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.f, L"en-us", &subFmt);
    if (subFmt) {
        rt->DrawTextW(L"Choose where to install Nirqoba and configure installation options.",
            64, subFmt, RectF(cL, cT + 58.f, cR, cT + 86.f), g_brushTextSec);
        subFmt->Release();
    }

    rt->DrawLine(D2D1::Point2F(cL, cT + 96.f), D2D1::Point2F(cR, cT + 96.f),
        g_brushSeparator, 1.f);

    // Install Path section
    float sectionT = cT + 120.f;
    IDWriteTextFormat* sectionFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 22.f, L"en-us", &sectionFmt);
    if (sectionFmt) {
        rt->DrawTextW(L"Installation Directory", 22, sectionFmt,
            RectF(cL, sectionT, cR, sectionT + 32.f), g_brushTextPri);
        sectionFmt->Release();
    }

    // Path edit box visualization (behind the actual HWND edit control)
    float editT = sectionT + 42.f;
    float editB = editT + 52.f;
    float editR = cR - 154.f;
    rt->FillRoundedRectangle(RoundedRect(cL, editT, editR, editB, 5.f), g_brushSurface);
    rt->DrawRoundedRectangle(RoundedRect(cL, editT, editR, editB, 5.f), g_brushAccentBorder, 1.f);

    // Additional options section
    float optT = editB + 44.f;
    IDWriteTextFormat* optSectionFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 22.f, L"en-us", &optSectionFmt);
    if (optSectionFmt) {
        rt->DrawTextW(L"Additional Options", 18, optSectionFmt,
            RectF(cL, optT, cR, optT + 32.f), g_brushTextPri);
        optSectionFmt->Release();
    }

}

static void DrawPageInstalling(ID2D1RenderTarget* rt) {
    float cL = SIDEBAR_W + 40.f;
    float cR = (float)WINDOW_W - 40.f;
    float cT = TITLEBAR_H + 40.f;
    float cMidX = (cL + cR) / 2.f;

    // Animated logo during install
    float logoScale = 0.55f + 0.025f * sinf(g_animT * 1.2f);
    DrawNirqobaLogo(rt, cMidX, cT + 128.f, logoScale);

    IDWriteTextFormat* titleFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 34.f, L"en-us", &titleFmt);
    if (titleFmt) {
        titleFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        rt->DrawTextW(L"Installing Nirqoba", 18, titleFmt,
            RectF(cL, cT + 300.f, cR, cT + 348.f), g_brushTextPri);
        titleFmt->Release();
    }

    // Status text (animated dots)
    int dotCount = ((int)(g_animT * 1.5f)) % 4;
    wchar_t statusText[128];
    const wchar_t* steps[] = {
        L"Copying compiler binaries",
        L"Installing runtime library",
        L"Registering toolchain",
        L"Configuring environment",
        L"Finalizing installation"
    };
    int stepIdx = (int)(g_progress * 5.f);
    if (stepIdx >= 5) stepIdx = 4;
    swprintf_s(statusText, L"%ls%ls", steps[stepIdx],
        dotCount == 0 ? L"." : dotCount == 1 ? L".." : dotCount == 2 ? L"..." : L"");

    IDWriteTextFormat* statusFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.f, L"en-us", &statusFmt);
    if (statusFmt) {
        statusFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        rt->DrawTextW(statusText, (UINT32)wcslen(statusText), statusFmt,
            RectF(cL, cT + 354.f, cR, cT + 384.f), g_brushTextSec);
        statusFmt->Release();
    }

    // Progress bar track
    float barL = cL;
    float barR = cR;
    float barT = cT + 408.f;
    float barH = 14.f;
    float barB = barT + barH;
    float barRadius = barH / 2.f;

    rt->FillRoundedRectangle(RoundedRect(barL, barT, barR, barB, barRadius), g_brushProgressBg);

    // Progress fill
    float fillW = (barR - barL) * g_progress;
    if (fillW > barRadius * 2.f) {
        g_brushProgressGrad->SetStartPoint(D2D1::Point2F(barL, 0));
        g_brushProgressGrad->SetEndPoint(D2D1::Point2F(barR, 0));
        rt->FillRoundedRectangle(RoundedRect(barL, barT, barL + fillW, barB, barRadius), g_brushProgressGrad);

        // Shimmer effect
        ID2D1SolidColorBrush* shimmerBrush = nullptr;
        float shimmerPos = barL + fillW - 30.f + 60.f * fmodf(g_animT * 0.8f, 1.f);
        shimmerPos = std::min(shimmerPos, barL + fillW - 5.f);
        if (shimmerPos > barL) {
            rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.2f), &shimmerBrush);
            rt->FillRoundedRectangle(
                RoundedRect(shimmerPos - 15.f, barT + 1.f, shimmerPos + 15.f, barB - 1.f, 3.f),
                shimmerBrush);
            shimmerBrush->Release();
        }
    }

    // Progress percentage text
    wchar_t pctText[16];
    swprintf_s(pctText, L"%.0f%%", g_progress * 100.f);
    IDWriteTextFormat* pctFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 16.f, L"en-us", &pctFmt);
    if (pctFmt) {
        pctFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        rt->DrawTextW(pctText, (UINT32)wcslen(pctText), pctFmt,
            RectF(cL, barB + 12.f, cR, barB + 38.f), g_brushTextSec);
        pctFmt->Release();
    }
}

static void DrawPageFinish(ID2D1RenderTarget* rt) {
    float cL = SIDEBAR_W + 40.f;
    float cR = (float)WINDOW_W - 40.f;
    float cT = TITLEBAR_H + 40.f;
    float cMidX = (cL + cR) / 2.f;

    // Success checkmark circle
    float checkCx = cMidX;
    float checkCy = cT + 70.f;
    float checkR  = 52.f;

    ID2D1SolidColorBrush* greenFill = nullptr;
    ID2D1SolidColorBrush* greenStroke = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0x1A4A32, 0.8f), &greenFill);
    rt->CreateSolidColorBrush(Color::Success, &greenStroke);

    rt->FillEllipse(Ellipse(checkCx, checkCy, checkR, checkR), greenFill);
    rt->DrawEllipse(Ellipse(checkCx, checkCy, checkR, checkR), greenStroke, 2.f);

    // Checkmark
    ID2D1PathGeometry* checkGeo = nullptr;
    g_d2dFactory->CreatePathGeometry(&checkGeo);
    if (checkGeo) {
        ID2D1GeometrySink* sink = nullptr;
        checkGeo->Open(&sink);
        sink->BeginFigure(D2D1::Point2F(checkCx - 16.f, checkCy), D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLine(D2D1::Point2F(checkCx - 4.f, checkCy + 14.f));
        sink->AddLine(D2D1::Point2F(checkCx + 18.f, checkCy - 14.f));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();
        if (sink) sink->Release();
        rt->DrawGeometry(checkGeo, greenStroke, 3.5f);
        checkGeo->Release();
    }
    greenFill->Release();
    greenStroke->Release();

    // Success title
    IDWriteTextFormat* titleFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 40.f, L"en-us", &titleFmt);
    if (titleFmt) {
        titleFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        rt->DrawTextW(L"Installation Complete!", 21, titleFmt,
            RectF(cL, checkCy + checkR + 12.f, cR, checkCy + checkR + 64.f),
            g_brushTextPri);
        titleFmt->Release();
    }

    IDWriteTextFormat* subFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 22.f, L"en-us", &subFmt);
    if (subFmt) {
        subFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        subFmt->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        rt->DrawTextW(
            L"Nirqoba is ready to use.",
            23, subFmt,
            RectF(cL + 20.f, checkCy + checkR + 68.f, cR - 20.f, checkCy + checkR + 104.f),
            g_brushTextSec);
        subFmt->Release();
    }

    // Info card
    float infoT = checkCy + checkR + 116.f;
    rt->FillRoundedRectangle(RoundedRect(cL, infoT, cR, infoT + 102.f, 10.f), g_brushSurface);
    ID2D1SolidColorBrush* infoAccent = nullptr;
    rt->CreateSolidColorBrush(Color::Success, &infoAccent);
    rt->FillRoundedRectangle(RoundedRect(cL, infoT, cL + 5.f, infoT + 102.f, 3.f), infoAccent);
    infoAccent->Release();

    IDWriteTextFormat* infoFmt = nullptr;
    g_dwFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.f, L"en-us", &infoFmt);
    if (infoFmt) {
        rt->DrawTextW(L"Installed to:", 13, infoFmt,
            RectF(cL + 24.f, infoT + 12.f, cR - 16.f, infoT + 38.f), g_brushTextMuted);

        IDWriteTextFormat* pathFmt = nullptr;
        g_dwFactory->CreateTextFormat(L"Cascadia Mono", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 17.f, L"en-us", &pathFmt);
        if (!pathFmt) {
            g_dwFactory->CreateTextFormat(L"Consolas", nullptr,
                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 17.f, L"en-us", &pathFmt);
        }
        if (pathFmt) {
            rt->DrawTextW(g_installPath.c_str(), (UINT32)g_installPath.size(), pathFmt,
                RectF(cL + 24.f, infoT + 40.f, cR - 16.f, infoT + 66.f), g_brushAccent);
            pathFmt->Release();
        }

        rt->DrawTextW(L"Run  nirqoba --help  to get started.",
            36, infoFmt,
            RectF(cL + 24.f, infoT + 70.f, cR - 16.f, infoT + 96.f), g_brushTextSec);
        infoFmt->Release();
    }
}

// ---------------------------------------------------------------------------
// Button bar drawing
// ---------------------------------------------------------------------------
static void DrawButtonBar(ID2D1RenderTarget* rt) {
    float barT = (float)WINDOW_H - 84.f;
    float barB = (float)WINDOW_H;
    float cL   = (float)SIDEBAR_W;
    float cR   = (float)WINDOW_W;

    // Bar background
    rt->FillRectangle(RectF(cL, barT, cR, barB), g_brushPanel);
    rt->DrawLine(D2D1::Point2F(cL, barT), D2D1::Point2F(cR, barT), g_brushSeparator, 1.f);
}

static void StartPageTransition() {
    g_transitionProgress = 0.f;
    SetTimer(g_hWnd, ID_TIMER_TRANSITION, 16, nullptr);
}

// ---------------------------------------------------------------------------
// Main render function
// ---------------------------------------------------------------------------
static void OnRender() {
    HRESULT hr = CreateDeviceResources();
    if (FAILED(hr)) return;

    g_renderTarget->BeginDraw();

    // Full background
    g_renderTarget->Clear(Color::Background);

    // Sidebar
    DrawSidebar(g_renderTarget);

    // Title bar in content area
    DrawTitleBar(g_renderTarget);

    // Slide each new page into place with a short ease-out transition.
    float t = std::max(0.f, std::min(1.f, g_transitionProgress));
    float eased = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
    g_renderTarget->SetTransform(D2D1::Matrix3x2F::Translation((1.f - eased) * 44.f, 0.f));

    // Page content
    switch (g_currentPage) {
        case PAGE_WELCOME:   DrawPageWelcome(g_renderTarget);   break;
        case PAGE_LICENSE:   DrawPageLicense(g_renderTarget);   break;
        case PAGE_OPTIONS:   DrawPageOptions(g_renderTarget);   break;
        case PAGE_INSTALLING:DrawPageInstalling(g_renderTarget);break;
        case PAGE_FINISH:    DrawPageFinish(g_renderTarget);    break;
    }
    g_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // Button bar
    DrawButtonBar(g_renderTarget);

    hr = g_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
}

// ---------------------------------------------------------------------------
// Native control management (position/show/hide based on page)
// ---------------------------------------------------------------------------
static void UpdateNativeControls() {
    bool showEditPath  = (g_currentPage == PAGE_OPTIONS);
    bool showBrowse    = (g_currentPage == PAGE_OPTIONS);
    bool showChkDsk    = (g_currentPage == PAGE_OPTIONS);
    bool showChkPath   = (g_currentPage == PAGE_OPTIONS);

    if (g_hEditPath)   ShowWindow(g_hEditPath,   showEditPath ? SW_SHOW : SW_HIDE);
    if (g_hBtnBrowse)  ShowWindow(g_hBtnBrowse,  showBrowse   ? SW_SHOW : SW_HIDE);
    if (g_hChkDesktop) ShowWindow(g_hChkDesktop, showChkDsk   ? SW_SHOW : SW_HIDE);
    if (g_hChkPath)    ShowWindow(g_hChkPath,    showChkPath  ? SW_SHOW : SW_HIDE);

    // Update Next button text
    if (g_hBtnNext) {
        if (g_currentPage == PAGE_FINISH)
            SetWindowTextW(g_hBtnNext, L"Finish");
        else if (g_currentPage == PAGE_OPTIONS)
            SetWindowTextW(g_hBtnNext, L"Install");
        else
            SetWindowTextW(g_hBtnNext, L"Next");
        EnableWindow(g_hBtnNext, g_currentPage != PAGE_INSTALLING);
    }

    // Back button
    if (g_hBtnBack) {
        EnableWindow(g_hBtnBack,
            g_currentPage != PAGE_WELCOME &&
            g_currentPage != PAGE_INSTALLING &&
            g_currentPage != PAGE_FINISH);
    }

    // Cancel button
    if (g_hBtnCancel) {
        SetWindowTextW(g_hBtnCancel,
            g_currentPage == PAGE_FINISH ? L"Close" : L"Cancel");
    }
}

// ---------------------------------------------------------------------------
// Page transitions
// ---------------------------------------------------------------------------
static void GoNextPage() {
    if (g_currentPage == PAGE_OPTIONS) {
        // Get install path from edit box
        if (g_hEditPath) {
            wchar_t buf[MAX_PATH] = {0};
            GetWindowTextW(g_hEditPath, buf, MAX_PATH);
            g_installPath = buf;
        }
        // Start installation
        g_currentPage = PAGE_INSTALLING;
        g_progress    = 0.f;
        g_installing  = true;
        SetTimer(g_hWnd, ID_TIMER_PROGRESS, 60, nullptr);
    } else if (g_currentPage == PAGE_FINISH) {
        PostQuitMessage(0);
    } else if (g_currentPage < PAGE_OPTIONS) {
        g_currentPage = (InstallerPage)((int)g_currentPage + 1);
    }
    if (g_currentPage != PAGE_FINISH)
        StartPageTransition();
    UpdateNativeControls();
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

static void GoPrevPage() {
    if (g_currentPage > PAGE_WELCOME &&
        g_currentPage != PAGE_INSTALLING &&
        g_currentPage != PAGE_FINISH)
    {
        g_currentPage = (InstallerPage)((int)g_currentPage - 1);
        StartPageTransition();
        UpdateNativeControls();
        InvalidateRect(g_hWnd, nullptr, FALSE);
    }
}

// ---------------------------------------------------------------------------
// Native control creation
// ---------------------------------------------------------------------------
static void CreateNativeControls(HWND hWnd) {
    // Calculate positions
    float cL  = SIDEBAR_W + 40.f;
    float cR  = (float)WINDOW_W - 40.f;
    float cT  = TITLEBAR_H + 32.f;

    // Options page: path edit box
    float editT = cT + 120.f + 42.f;
    float editR = cR - 154.f;

    g_hEditPath = CreateWindowExW(
        0, L"EDIT", g_installPath.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        (int)cL + 1, (int)editT + 1, (int)(editR - cL - 2), 52,
        hWnd, (HMENU)ID_EDIT_PATH, g_hInst, nullptr);

    // Style the edit box to match dark theme
    if (g_hEditPath) {
        HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        SendMessageW(g_hEditPath, WM_SETFONT, (WPARAM)hFont, TRUE);
        // Colors handled by WM_CTLCOLOREDIT
    }

    // Browse button
    g_hBtnBrowse = CreateWindowExW(
        0, L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        (int)editR + 12, (int)editT, 132, 54,
        hWnd, (HMENU)ID_BTN_BROWSE, g_hInst, nullptr);

    // Checkboxes
    float optT = editT + 52.f + 44.f;
    g_hChkDesktop = CreateWindowExW(
        0, L"BUTTON", L"Create desktop shortcut",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        (int)cL, (int)optT, (int)(cR - cL), 36,
        hWnd, (HMENU)ID_CHK_DESKTOP, g_hInst, nullptr);
    SendMessageW(g_hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);

    g_hChkPath = CreateWindowExW(
        0, L"BUTTON", L"Add Nirqoba to system PATH",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        (int)cL, (int)optT + 44.f, (int)(cR - cL), 36,
        hWnd, (HMENU)ID_CHK_PATH, g_hInst, nullptr);
    SendMessageW(g_hChkPath, BM_SETCHECK, BST_CHECKED, 0);

    // Style native controls with Segoe UI
    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    if (g_hBtnBrowse)  SendMessageW(g_hBtnBrowse,  WM_SETFONT, (WPARAM)hFont, TRUE);
    if (g_hChkDesktop) SendMessageW(g_hChkDesktop, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (g_hChkPath)    SendMessageW(g_hChkPath,    WM_SETFONT, (WPARAM)hFont, TRUE);

    // Navigation buttons (always visible in button bar)
    float btnBarT = WINDOW_H - 84.f;
    float btnH    = 54.f;
    float btnW    = 144.f;
    float btnMarR = WINDOW_W - 32.f;

    HFONT hBtnFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Cancel button (leftmost in button bar)
    g_hBtnCancel = CreateWindowExW(
        0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        SIDEBAR_W + 32, (int)btnBarT + 15, 112, (int)btnH,
        hWnd, (HMENU)ID_BTN_CANCEL, g_hInst, nullptr);

    // Back button
    g_hBtnBack = CreateWindowExW(
        0, L"BUTTON", L"Back",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        (int)btnMarR - 2 * (int)btnW - 16, (int)btnBarT + 15, (int)btnW, (int)btnH,
        hWnd, (HMENU)ID_BTN_BACK, g_hInst, nullptr);

    // Next button (primary)
    g_hBtnNext = CreateWindowExW(
        0, L"BUTTON", L"Next",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        (int)btnMarR - (int)btnW, (int)btnBarT + 15, (int)btnW, (int)btnH,
        hWnd, (HMENU)ID_BTN_NEXT, g_hInst, nullptr);

    if (g_hBtnNext)   SendMessageW(g_hBtnNext,   WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    if (g_hBtnBack)   SendMessageW(g_hBtnBack,   WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    if (g_hBtnCancel) SendMessageW(g_hBtnCancel, WM_SETFONT, (WPARAM)hBtnFont, TRUE);

    UpdateNativeControls();
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
static HBRUSH g_hDarkBrush   = nullptr;
static HBRUSH g_hSurfBrush   = nullptr;
static HBRUSH g_hEditBrush   = nullptr;

static void DrawStyledButton(const DRAWITEMSTRUCT* item) {
    if (!item || !item->hwndItem) return;
    const bool primary = item->CtlID == ID_BTN_NEXT;
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    COLORREF fillColor = disabled ? RGB(45, 54, 70)
        : primary ? (pressed ? RGB(73, 63, 159) : RGB(101, 91, 206))
        : (pressed ? RGB(30, 48, 70) : RGB(20, 37, 56));
    COLORREF borderColor = disabled ? RGB(57, 67, 82)
        : primary ? RGB(137, 127, 235) : RGB(58, 78, 103);
    COLORREF textColor = disabled ? RGB(132, 145, 162) : RGB(242, 245, 255);

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
    if (item->itemState & ODS_FOCUS) {
        InflateRect(&textRect, -8, -7);
        DrawFocusRect(item->hDC, &textRect);
    }
    if (oldFont) SelectObject(item->hDC, oldFont);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        g_hWnd = hWnd;

        // Default install path
        wchar_t pfDir[MAX_PATH];
        SHGetSpecialFolderPathW(hWnd, pfDir, CSIDL_PROGRAM_FILES, FALSE);
        g_installPath = std::wstring(pfDir) + L"\\Nirqoba";

        // DWM drop shadow for borderless window
        MARGINS margins = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea(hWnd, &margins);

        // Enable DWM dark mode
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hWnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &darkMode, sizeof(darkMode));

        // Corner rounding
        DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
        DwmSetWindowAttribute(hWnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &corner, sizeof(corner));

        // GDI brushes for native control backgrounds
        g_hDarkBrush = CreateSolidBrush(RGB(0x0D, 0x1B, 0x2A));
        g_hSurfBrush = CreateSolidBrush(RGB(0x11, 0x24, 0x3A));
        g_hEditBrush = CreateSolidBrush(RGB(0x11, 0x24, 0x3A));

        CreateNativeControls(hWnd);
        SetTimer(hWnd, 1, 16, nullptr); // ~60fps render timer for animations
        return 0;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER_TRANSITION) {
            g_transitionProgress = std::min(1.f, g_transitionProgress + 0.10f);
            if (g_transitionProgress >= 1.f)
                KillTimer(hWnd, ID_TIMER_TRANSITION);
            InvalidateRect(hWnd, nullptr, FALSE);
        } else if (wParam == 1) {
            g_animT += 0.016f;
            if (g_currentPage == PAGE_INSTALLING) {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        } else if (wParam == ID_TIMER_PROGRESS) {
            g_progress += 0.008f;
            if (g_progress >= 1.0f) {
                g_progress = 1.0f;
                g_installing = false;
                KillTimer(hWnd, ID_TIMER_PROGRESS);
                // Auto-advance to finish after short delay
                SetTimer(hWnd, 300, 600, nullptr);
            }
            InvalidateRect(hWnd, nullptr, FALSE);
        } else if (wParam == 300) {
            KillTimer(hWnd, 300);
            g_currentPage = PAGE_FINISH;
            StartPageTransition();
            UpdateNativeControls();
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        OnRender();
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SIZE:
    {
        if (g_renderTarget) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            g_renderTarget->Resize(D2D1::SizeU(rc.right, rc.bottom));
        }
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_MOVE:
        RedrawWindow(hWnd, nullptr, nullptr,
            RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        return 0;

    case WM_DRAWITEM:
        DrawStyledButton(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    // Dark theme for native controls
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0xF0, 0xF4, 0xFF));
        SetBkColor(hdc, RGB(0x11, 0x24, 0x3A));
        return (LRESULT)g_hEditBrush;
    }
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0xA0, 0xB8, 0xD8));
        SetBkColor(hdc, RGB(0x0D, 0x1B, 0x2A));
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)g_hDarkBrush;
    }

    // Frameless window: allow dragging from title bar area
    case WM_NCHITTEST:
    {
        LRESULT result = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (result == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            // Drag from title bar area (top portion of content area)
            if (pt.x > SIDEBAR_W && pt.y < TITLEBAR_H) {
                // Check if not on close/minimize buttons
                float btnSize = TITLEBAR_H * 0.6f;
                if (pt.x < WINDOW_W - btnSize * 2 - 16)
                    return HTCAPTION;
            }
            // Resize edges
            RECT rc;
            GetClientRect(hWnd, &rc);
            int edge = 4;
            if (pt.x < edge) return (pt.y < edge) ? HTTOPLEFT : (pt.y > rc.bottom - edge) ? HTBOTTOMLEFT : HTLEFT;
            if (pt.x > rc.right - edge) return (pt.y < edge) ? HTTOPRIGHT : (pt.y > rc.bottom - edge) ? HTBOTTOMRIGHT : HTRIGHT;
            if (pt.y < edge) return HTTOP;
            if (pt.y > rc.bottom - edge) return HTBOTTOM;
        }
        return result;
    }

    // Window control buttons (hit on the X and minimize icons we drew)
    case WM_NCLBUTTONDOWN:
    {
        if (wParam == HTCAPTION) break; // handled by system
        break;
    }

    // Simulate close/minimize button clicks
    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        float btnSize = TITLEBAR_H * 0.6f;
        float closeX  = WINDOW_W - btnSize - 8.f;
        float minX    = closeX - btnSize - 4.f;
        float btnY    = (TITLEBAR_H - btnSize) / 2.f;

        if (pt.y >= btnY && pt.y <= btnY + btnSize && pt.x > SIDEBAR_W) {
            if (pt.x >= closeX && pt.x <= closeX + btnSize) {
                PostQuitMessage(0);
                return 0;
            }
            if (pt.x >= minX && pt.x <= minX + btnSize) {
                ShowWindow(hWnd, SW_MINIMIZE);
                return 0;
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        switch (id) {
        case ID_BTN_NEXT:   GoNextPage();  break;
        case ID_BTN_BACK:   GoPrevPage();  break;
        case ID_BTN_CANCEL:
            if (g_currentPage == PAGE_FINISH)
                PostQuitMessage(0);
            else {
                if (MessageBoxW(hWnd,
                    L"Are you sure you want to cancel the installation?",
                    L"Cancel Installation",
                    MB_YESNO | MB_ICONQUESTION) == IDYES)
                    PostQuitMessage(1);
            }
            break;
        case ID_BTN_BROWSE:
        {
            // Folder browser
            BROWSEINFOW bi = {};
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"Select installation directory:";
            bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t path[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, path)) {
                    std::wstring fullPath = std::wstring(path) + L"\\Nirqoba";
                    if (g_hEditPath) SetWindowTextW(g_hEditPath, fullPath.c_str());
                }
                CoTaskMemFree(pidl);
            }
            break;
        }
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // prevent flicker

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        KillTimer(hWnd, ID_TIMER_PROGRESS);
        KillTimer(hWnd, ID_TIMER_TRANSITION);
        DiscardDeviceResources();
        if (g_hDarkBrush) { DeleteObject(g_hDarkBrush); g_hDarkBrush = nullptr; }
        if (g_hSurfBrush) { DeleteObject(g_hSurfBrush); g_hSurfBrush = nullptr; }
        if (g_hEditBrush) { DeleteObject(g_hEditBrush); g_hEditBrush = nullptr; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;

    // Initialize COM
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Create D2D factory
    D2D1_FACTORY_OPTIONS opts = {};
    opts.debugLevel = D2D1_DEBUG_LEVEL_NONE;
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, opts, &g_d2dFactory)))
        return 1;

    // Create DirectWrite factory
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&g_dwFactory))))
        return 1;

    // Create WIC factory
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&g_wicFactory));

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"NirqobaInstallerWnd";
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm       = wc.hIcon;
    RegisterClassExW(&wc);

    // Center on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winX    = (screenW - WINDOW_W) / 2;
    int winY    = (screenH - WINDOW_H) / 2;

    // Create borderless window with drop shadow
    HWND hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"NirqobaInstallerWnd",
        L"Nirqoba Setup",
        WS_POPUP | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        winX, winY, WINDOW_W, WINDOW_H,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    if (g_d2dFactory) { g_d2dFactory->Release(); g_d2dFactory = nullptr; }
    if (g_dwFactory)  { g_dwFactory->Release();  g_dwFactory  = nullptr; }
    if (g_wicFactory) { g_wicFactory->Release(); g_wicFactory = nullptr; }
    CoUninitialize();

    return (int)msg.wParam;
}
