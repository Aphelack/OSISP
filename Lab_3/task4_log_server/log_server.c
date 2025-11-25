#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>

#define PIPE_NAME _T("\\\\.\\pipe\\LogServerPipe")
#define BUFFER_SIZE 1024
#define LOG_FILE _T("server.log")
#define MAX_CLIENTS 10

// Message structure
typedef struct {
    DWORD clientId;
    TCHAR message[BUFFER_SIZE];
} LogMessage;

// Function declarations
void WriteLog(HANDLE hLogFile, DWORD clientId, const TCHAR* message);
void GetTimestamp(TCHAR* buffer, size_t size);
DWORD WINAPI ClientHandler(LPVOID lpParam);

// Global log file handle with mutex for thread-safe access
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
CRITICAL_SECTION g_csLogFile;
volatile BOOL g_bRunning = TRUE;

int _tmain(int argc, TCHAR* argv[])
{
    HANDLE hPipe;
    HANDLE hThreads[MAX_CLIENTS];
    DWORD threadCount = 0;
    
    _tprintf(_T("=== Log Server Starting ===\n"));
    _tprintf(_T("Pipe name: %s\n"), PIPE_NAME);
    _tprintf(_T("Log file: %s\n\n"), LOG_FILE);
    
    // Initialize critical section for log file access
    InitializeCriticalSection(&g_csLogFile);
    
    // Open log file
    g_hLogFile = CreateFile(
        LOG_FILE,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (g_hLogFile == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Failed to create log file (error %d)\n"), GetLastError());
        DeleteCriticalSection(&g_csLogFile);
        return 1;
    }
    
    _tprintf(_T("Log server is ready. Waiting for clients...\n"));
    _tprintf(_T("Press Ctrl+C to stop the server.\n\n"));
    
    // Main server loop - accept multiple clients
    while (g_bRunning && threadCount < MAX_CLIENTS) {
        // Create named pipe instance
        hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFFER_SIZE,
            BUFFER_SIZE,
            0,
            NULL
        );
        
        if (hPipe == INVALID_HANDLE_VALUE) {
            _tprintf(_T("CreateNamedPipe failed (error %d)\n"), GetLastError());
            break;
        }
        
        // Wait for client to connect
        _tprintf(_T("Waiting for client connection...\n"));
        
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? 
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        
        if (connected) {
            _tprintf(_T("Client connected! Creating handler thread...\n"));
            
            // Create thread to handle this client
            hThreads[threadCount] = CreateThread(
                NULL,
                0,
                ClientHandler,
                (LPVOID)hPipe,
                0,
                NULL
            );
            
            if (hThreads[threadCount] == NULL) {
                _tprintf(_T("CreateThread failed (error %d)\n"), GetLastError());
                CloseHandle(hPipe);
            } else {
                threadCount++;
            }
        } else {
            _tprintf(_T("ConnectNamedPipe failed (error %d)\n"), GetLastError());
            CloseHandle(hPipe);
        }
    }
    
    _tprintf(_T("\nShutting down server...\n"));
    g_bRunning = FALSE;
    
    // Wait for all client threads to finish
    if (threadCount > 0) {
        _tprintf(_T("Waiting for %d client threads to finish...\n"), threadCount);
        WaitForMultipleObjects(threadCount, hThreads, TRUE, 5000);
        
        for (DWORD i = 0; i < threadCount; i++) {
            CloseHandle(hThreads[i]);
        }
    }
    
    // Close log file
    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hLogFile);
    }
    
    DeleteCriticalSection(&g_csLogFile);
    
    _tprintf(_T("Server stopped. Log file: %s\n"), LOG_FILE);
    
    return 0;
}

DWORD WINAPI ClientHandler(LPVOID lpParam)
{
    HANDLE hPipe = (HANDLE)lpParam;
    LogMessage msg;
    DWORD bytesRead;
    BOOL result;
    DWORD clientNum = GetCurrentThreadId();
    
    _tprintf(_T("[Thread %d] Client handler started\n"), clientNum);
    
    // Read messages from client until disconnected
    while (g_bRunning) {
        result = ReadFile(
            hPipe,
            &msg,
            sizeof(LogMessage),
            &bytesRead,
            NULL
        );
        
        if (!result || bytesRead == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                _tprintf(_T("[Thread %d] Client disconnected\n"), clientNum);
            } else {
                _tprintf(_T("[Thread %d] ReadFile failed (error %d)\n"), 
                        clientNum, GetLastError());
            }
            break;
        }
        
        // Write to log file
        WriteLog(g_hLogFile, msg.clientId, msg.message);
        
        // Echo back to client (acknowledgment)
        TCHAR response[64];
        _sntprintf(response, 64, _T("ACK"));
        DWORD bytesWritten;
        WriteFile(hPipe, response, (_tcslen(response) + 1) * sizeof(TCHAR), 
                 &bytesWritten, NULL);
    }
    
    // Cleanup
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    
    _tprintf(_T("[Thread %d] Client handler stopped\n"), clientNum);
    
    return 0;
}

void WriteLog(HANDLE hLogFile, DWORD clientId, const TCHAR* message)
{
    TCHAR timestamp[64];
    TCHAR logEntry[BUFFER_SIZE + 128];
    DWORD bytesWritten;
    
    GetTimestamp(timestamp, 64);
    
    // Format: [timestamp] [ClientID] message
    _sntprintf(logEntry, BUFFER_SIZE + 128, 
              _T("[%s] [Client-%d] %s\r\n"), 
              timestamp, clientId, message);
    
    // Thread-safe write to log file
    EnterCriticalSection(&g_csLogFile);
    
    WriteFile(
        hLogFile,
        logEntry,
        _tcslen(logEntry) * sizeof(TCHAR),
        &bytesWritten,
        NULL
    );
    
    FlushFileBuffers(hLogFile);
    
    LeaveCriticalSection(&g_csLogFile);
    
    // Also print to console
    _tprintf(_T("%s"), logEntry);
}

void GetTimestamp(TCHAR* buffer, size_t size)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    _sntprintf(buffer, size, _T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}
