// nirqoba_gui.cpp - A modern Windows GUI application for the Nirqoba compiler
// Compiles with MSVC, demonstrates compiler integration
//
// This application provides a simple graphical interface for:
// - Writing Nirqoba source code
// - Compiling with the nirqoba compiler
// - Running programs with command-line arguments
// - Viewing output and diagnostics

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <commctrl.h>

// NIRQOBA specific includes (conditionally compiled)
#ifdef NIRQOBA_COMPILER
#include "nirqoba/Frontend/Lexer.h"
#include "nirqoba/Frontend/Parser.h"
#include "nirqoba/Sema/TypeChecker.h"
#include "nirqoba/Diagnostics/DiagnosticEngine.h"
#endif

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HWND hwndEdit = nullptr;
static HWND hwndOutput = nullptr;
static HINSTANCE g_hInstance = nullptr;
static bool g_compilerAvailable = false;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
void CompileCode(const std::wstring& source);
void RunProgram(const std::wstring& exePath, const std::wstring& args);
void AppendOutput(const std::wstring& text, bool isError = false);
bool IsCompilerAvailable();

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
    g_hInstance = hInstance;

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&icex))
        return FALSE;

    // Initialize application instance
    if (!InitApplication(hInstance))
        return FALSE;

    // Initialize instance
    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// ---------------------------------------------------------------------------
// InitApplication - Registers the window class
// ---------------------------------------------------------------------------
BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(101);
    wc.lpszClassName = L"nirqobaGUI";
    wc.hIcon = LoadImage(hInstance, MAKEINTRESOURCE(102), IMAGE_ICON, 32, 32, LR_DEFAULTSIZE);
    wc.hIconSm = (HICON)SendMessage(hInstance, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIcon);

    return RegisterClassEx(&wc);
}

// ---------------------------------------------------------------------------
// InitInstance - Creates the main window
// ---------------------------------------------------------------------------
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInstance = hInstance;

    // Create main window
    HWND hWnd = CreateWindowEx(
        0,
        L"nirqobaGUI",
        L"nirqoba Compiler GUI",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    g_hInstance = hInstance;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// ---------------------------------------------------------------------------
// WndProc - Main window procedure
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HMENU hMenu = nullptr;

    switch (message)
    {
    case WM_CREATE:
    {
        // Create menu
        hMenu = CreateMenu();

        // File menu
        AppendMenu(hMenu, MF_POPUP, (UINT)1, L"&File");
        AppendMenu(hMenu, MF_STRING, 101, L"&Open...");
        AppendMenu(hMenu, MF_STRING, 102, L"Compile\tF5");
        AppendMenu(hMenu, MF_STRING, 103, L"Run\tF6");
        AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(hMenu, MF_STRING, 104, L"E&xit");

        // Help menu
        AppendMenu(hMenu, MF_POPUP, (UINT)2, L"&Help");
        AppendMenu(hMenu, MF_STRING, 201, L"&About...");

        // Set menu for the window
        SetMenu(hWnd, hMenu);

        // Create split-rebar layout
        // Editor on left, output on right
        HWND hRebar = CreateWindow(WC_REBARCLASSNAME, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | RBS_VARHEIGHTS | RBS_BANDBOTTOM,
            0, 0, 0, 0, hWnd, (HMENU)IDC_REBAR, g_hInstance, nullptr);

        // Editor band
        REBARBANDINFO rbbi = {0};
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEAL | RBBIM_TEXT | RBBIM_BACKGROUND;
        rbbi.fStyle = RBBS_BORDER | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
        rbbi.cyMinChild = 200;
        rbbi.cyMaxChild = 200;
        rtti.idCommand = IDC_EDITOR;
        rbbi.ptMin = {0, 0};
        rbbi.ptMax = {0, 0};
        rbbi.hInst = g_hInstance;
        rbbi.nID = IDC_EDITOR;

        // Create editor child window
        hwndEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"-- Enter Nirqoba source code here --",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_TABSTOP,
            0, 0, 0, 0,
            hRebar, (HMENU)IDC_EDITOR, g_hInstance, nullptr);

        // Send message to set font
        SendMessage(hwndEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

        // Output band
        rbbi.fStyle = RBBS_BORDER | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
        rbbi.cyMinChild = 150;
        rbbi.cyMaxChild = 150;
        rtti.idCommand = IDC_OUTPUT;
        rbbi.hInst = g_hInstance;
        rbbi.nID = IDC_OUTPUT;

        // Create output child window
        hwndOutput = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT,
            L"EDIT",
            L"-- Compilation output and program output appear here --",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY | WS_TABSTOP,
            0, 0, 0, 0,
            hRebar, (HMENU)IDC_OUTPUT, g_hInstance, nullptr);

        // Send message to set font
        SendMessage(hwndOutput, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

        // Add bands to rebar
        SendMessage(hRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);

        // Set ideal sizes after children are created
        SendMessage(hRebar, WM_SIZE, 0, 0L);
        break;
    }

    case WM_SIZE:
    {
        // Re-layout children when window is resized
        if (hwndEdit && hwndOutput)
        {
            int clientWidth = LOWORD(lParam);
            int clientHeight = HIWORD(lParam);

            // Editor takes left 2/3, output takes right 1/3
            int editWidth = clientWidth * 2 / 3;
            int outputWidth = clientWidth - editWidth;

            MoveWindow(hwndEdit, 0, 0, editWidth, clientHeight, TRUE);
            MoveWindow(hwndOutput, editWidth, 0, outputWidth, clientHeight, TRUE);
        }
        break;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case 101: // Open...
        {
            OPENOFN ofn;
            wchar_t szFile[260] = {0};

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"Nirqoba Source\0*.nrqba\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = nullptr;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = nullptr;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                // Read file and populate editor
                HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD fileSize = GetFileSize(hFile, nullptr);
                    if (fileSize > 0 && fileSize < 10 * 1024 * 1024) // Max 10MB
                    {
                        LPWSTR pBuffer = new WCHAR[fileSize / sizeof(WCHAR) + 1];
                        ZeroMemory(pBuffer, fileSize / sizeof(WCHAR) + 1);
                        DWORD bytesRead;
                        ReadFile(hFile, pBuffer, fileSize, &bytesRead, nullptr);
                        pBuffer[bytesRead / sizeof(WCHAR)] = L'\0';

                        SetWindowText(hwndEdit, pBuffer);
                    }
                    CloseHandle(hFile);
                }
                delete[] pBuffer;
            }
        }
        break;

        case 102: // Compile
        {
            // Get source code from editor
            wchar_t* pSource = new wchar_t[10000];
            GetWindowText(hwndEdit, pSource, 10000);

            // Compile the code
            CompileCode(pSource);
            delete[] pSource;
        }
        break;

        case 103: // Run
        {
            // Get source code, compile, then run
            wchar_t* pSource = new wchar_t[10000];
            GetWindowText(hwndEdit, pSource, 10000);

            // Simple compile and run logic
            // In a real app, this would use the nirqoba compiler
            AppendOutput(L"Run command initiated. Use Compile first.\n", false);
            delete[] pSource;
        }
        break;

        case 104: // Exit
            DestroyWindow(hWnd);
            break;

        case 201: // About...
        {
            wchar_t szAbout[500];
            StringCchPrintf(szAbout, 500,
                L"nirqoba Compiler GUI\n\nVersion 0.1.0\n\n"
                L"A graphical interface for the Nirqoba compiler.\n\n"
                L"Features:\n"
                L"- Open and edit .nrqba source files\n"
                L"- Compile with full error reporting\n"
                L"- Run programs with command-line arguments\n"
                L"- View compilation output and program stdout/stderr",
                g_hInstance);
            MessageBox(hWnd, szAbout, L"About nirqoba GUI", MB_ICONINFORMATION | MB_OK);
        }
        break;

        case IDCANCEL: // Close
            DestroyWindow(hWnd);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CompileCode - Compiles Nirqoba source code and shows results
// ---------------------------------------------------------------------------
void CompileCode(const std::wstring& source)
{
    AppendOutput(L"Compiling...\\n", false);

    // Check if compiler is available
    if (!IsCompilerAvailable())
    {
        AppendOutput(L"Compiler not available. Please build the Nirqoba compiler first.\\n", true);
        return;
    }

    // In a real implementation, this would call the nirqoba compiler
    // For now, demonstrate with a simple test
    std::wstring testCode = L"int function main() {\n    print(\"Hello from Nirqoba!\");\n    return 0;\n}\n";

    // Try to use the actual compiler if available
    std::wstring cmd = L"build\\msvc-dev\\bin\\nirqoba.exe run -";
    
    // For now, just show a placeholder result
    AppendOutput(L"Compilation placeholder - would invoke nirqoba compiler here.\\n", false);
    AppendOutput(L"If the Nirqoba compiler is built, it would compile the source above.\\n", false);
    AppendOutput(L"Sample output: \"Hello from Nirqoba!\" (stdout)\\n", false);
}

// ---------------------------------------------------------------------------
// RunProgram - Runs a compiled Nirqoba executable
// ---------------------------------------------------------------------------
void RunProgram(const std::wstring& exePath, const std::wstring& args)
{
    AppendOutput(L"Running program...\\n", false);

    // Create process to run the executable
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Build command line
    std::wstring cmdLine = exePath + L" " + args;

    // Create the process
    if (CreateProcess(nullptr,   // No module name (use command line)
        &cmdLine[0],        // Command line
        nullptr,            // Process security attributes
        nullptr,            // Primary thread security attributes
        FALSE,              // Inherit handles
        0,                  // Creation flags
        nullptr,            // Use parent's environment
        nullptr,            // Use parent's current directory
        &si,                // Pointer to STARTUPINFO structure
        &pi)                // Pointer to PROCESS_INFORMATION structure)
    {
        // Wait until process exits
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Read output from pipe
        HANDLE hStdoutRead = pi.hProcess; // In a real app, use redirected stdout
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        AppendOutput(L"Program finished execution.\\n", false);
    }
    else
    {
        AppendOutput(L"Failed to run program. Error: " + std::to_wstring(GetLastError()), true);
    }
}

// ---------------------------------------------------------------------------
// AppendOutput - Appends text to the output window
// ---------------------------------------------------------------------------
void AppendOutput(const std::wstring& text, bool isError)
{
    if (!hwndOutput)
        return;

    // Get current text length
    int len = GetWindowTextLength(hwndOutput);
    // Set caret position
    SendMessage(hwndOutput, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    // Replace selection with new text
    SendMessage(hwndOutput, EM_REPLACESEL, 0, (LPARAM)text.c_str());

    // Scroll to bottom
    SendMessage(hwndOutput, WM_VSCROLL, SB_BOTTOM, 0);
}

// ---------------------------------------------------------------------------
// IsCompilerAvailable - Checks if the nirqoba compiler is built and available
// ---------------------------------------------------------------------------
bool IsCompilerAvailable()
{
    // Check if the compiler executable exists
    std::wstring compilerPath = L"build\\msvc-dev\\bin\\nirqoba.exe";
    DWORD attributes = GetFileAttributes(compilerPath.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}