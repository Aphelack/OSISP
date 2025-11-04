#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define SHARED_MEMORY_NAME _T("Global\\SelfRestoringProcessData")
#define BUFFER_SIZE 1024

// Application context structure
typedef struct {
    int counter;
    TCHAR message[256];
    BOOL isRestarting;
} AppContext;

// Global variables
HWND g_hwndMain = NULL;
HWND g_hwndEdit = NULL;
HWND g_hwndCounter = NULL;
HWND g_hwndButton = NULL;
AppContext g_context = {0};
HANDLE g_hMapFile = NULL;

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SaveContext();
void LoadContext();
BOOL CreateChildProcess();
void UpdateDisplay();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    LPWSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc = {0};
    MSG msg;
    
    // Load context from shared memory if exists
    LoadContext();
    
    // Register window class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = _T("SelfRestoringProcess");
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, _T("Window Registration Failed!"), _T("Error"),
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create main window
    g_hwndMain = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        _T("SelfRestoringProcess"),
        _T("Self-Restoring Process Demo"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 350,
        NULL, NULL, hInstance, NULL
    );
    
    if (g_hwndMain == NULL) {
        MessageBox(NULL, _T("Window Creation Failed!"), _T("Error"),
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);
    
    // Message loop
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
        {
            // Create controls
            CreateWindow(_T("STATIC"), _T("Message:"),
                        WS_CHILD | WS_VISIBLE,
                        10, 10, 80, 20,
                        hwnd, NULL, NULL, NULL);
            
            g_hwndEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                _T("EDIT"),
                g_context.message,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                100, 10, 350, 20,
                hwnd, (HMENU)1, NULL, NULL
            );
            
            CreateWindow(_T("STATIC"), _T("Counter:"),
                        WS_CHILD | WS_VISIBLE,
                        10, 50, 80, 20,
                        hwnd, NULL, NULL, NULL);
            
            g_hwndCounter = CreateWindow(
                _T("STATIC"),
                _T("0"),
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                100, 50, 100, 20,
                hwnd, NULL, NULL, NULL
            );
            
            g_hwndButton = CreateWindow(
                _T("BUTTON"),
                _T("Increment Counter"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                100, 90, 150, 30,
                hwnd, (HMENU)2, NULL, NULL
            );
            
            CreateWindow(
                _T("BUTTON"),
                _T("Close (Self-Restore)"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                100, 140, 200, 30,
                hwnd, (HMENU)3, NULL, NULL
            );
            
            // Create info text
            CreateWindow(_T("STATIC"), 
                        _T("Click 'Close (Self-Restore)' to restart the process.\n")
                        _T("The counter and message will be preserved."),
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        10, 190, 460, 80,
                        hwnd, NULL, NULL, NULL);
            
            UpdateDisplay();
            break;
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 2: // Increment button
                    g_context.counter++;
                    GetWindowText(g_hwndEdit, g_context.message, 256);
                    UpdateDisplay();
                    break;
                    
                case 3: // Close and restart
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
        
        case WM_CLOSE:
            // Get current message text
            GetWindowText(g_hwndEdit, g_context.message, 256);
            
            // Mark as restarting
            g_context.isRestarting = TRUE;
            
            // Save context to shared memory
            SaveContext();
            
            // Create child process before terminating
            if (!CreateChildProcess()) {
                MessageBox(hwnd, 
                          _T("Failed to create child process!"), 
                          _T("Error"),
                          MB_ICONERROR | MB_OK);
            }
            
            // Destroy window
            DestroyWindow(hwnd);
            break;
        
        case WM_DESTROY:
            // Clean up shared memory if not restarting
            if (g_hMapFile != NULL && !g_context.isRestarting) {
                CloseHandle(g_hMapFile);
                g_hMapFile = NULL;
            }
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void SaveContext()
{
    LPVOID pBuf = NULL;
    
    // Create or open shared memory
    g_hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(AppContext),
        SHARED_MEMORY_NAME
    );
    
    if (g_hMapFile == NULL) {
        return;
    }
    
    // Map view of file
    pBuf = MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(AppContext)
    );
    
    if (pBuf == NULL) {
        CloseHandle(g_hMapFile);
        return;
    }
    
    // Copy context to shared memory
    CopyMemory(pBuf, &g_context, sizeof(AppContext));
    
    UnmapViewOfFile(pBuf);
}

void LoadContext()
{
    LPVOID pBuf = NULL;
    
    // Try to open existing shared memory
    g_hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_NAME
    );
    
    if (g_hMapFile == NULL) {
        // No shared memory exists, initialize with defaults
        g_context.counter = 0;
        _tcscpy(g_context.message, _T("Hello, World!"));
        g_context.isRestarting = FALSE;
        return;
    }
    
    // Map view of file
    pBuf = MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(AppContext)
    );
    
    if (pBuf == NULL) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return;
    }
    
    // Copy context from shared memory
    CopyMemory(&g_context, pBuf, sizeof(AppContext));
    
    // Reset restarting flag
    g_context.isRestarting = FALSE;
    
    UnmapViewOfFile(pBuf);
}

BOOL CreateChildProcess()
{
    TCHAR szPath[MAX_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    // Get path to current executable
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0) {
        return FALSE;
    }
    
    // Initialize structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    // Create child process
    if (!CreateProcess(
            szPath,         // Application name
            NULL,           // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory
            &si,            // Pointer to STARTUPINFO structure
            &pi)            // Pointer to PROCESS_INFORMATION structure
       ) {
        return FALSE;
    }
    
    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return TRUE;
}

void UpdateDisplay()
{
    TCHAR buffer[32];
    _sntprintf(buffer, 32, _T("%d"), g_context.counter);
    SetWindowText(g_hwndCounter, buffer);
}
