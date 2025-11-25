#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>

#define PIPE_NAME _T("\\\\.\\pipe\\LogServerPipe")
#define BUFFER_SIZE 1024

// Message structure (must match server)
typedef struct {
    DWORD clientId;
    TCHAR message[BUFFER_SIZE];
} LogMessage;

// Function declarations
BOOL SendLogMessage(HANDLE hPipe, DWORD clientId, const TCHAR* message);

int _tmain(int argc, TCHAR* argv[])
{
    HANDLE hPipe;
    DWORD clientId;
    int messageCount = 5;
    int delayMs = 1000;
    
    // Parse command line arguments
    if (argc > 1) {
        clientId = _ttoi(argv[1]);
    } else {
        clientId = GetCurrentProcessId() % 100;
    }
    
    if (argc > 2) {
        messageCount = _ttoi(argv[2]);
    }
    
    if (argc > 3) {
        delayMs = _ttoi(argv[3]);
    }
    
    _tprintf(_T("=== Log Client Started ===\n"));
    _tprintf(_T("Client ID: %d\n"), clientId);
    _tprintf(_T("Messages to send: %d\n"), messageCount);
    _tprintf(_T("Delay between messages: %d ms\n\n"), delayMs);
    
    // Connect to the named pipe
    _tprintf(_T("Connecting to log server...\n"));
    
    while (TRUE) {
        hPipe = CreateFile(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        
        if (hPipe != INVALID_HANDLE_VALUE) {
            break;
        }
        
        if (GetLastError() != ERROR_PIPE_BUSY) {
            _tprintf(_T("Could not open pipe (error %d)\n"), GetLastError());
            _tprintf(_T("Make sure the log server is running!\n"));
            return 1;
        }
        
        // Wait for pipe to become available
        _tprintf(_T("Waiting for pipe to become available...\n"));
        if (!WaitNamedPipe(PIPE_NAME, 5000)) {
            _tprintf(_T("Timeout waiting for pipe\n"));
            return 1;
        }
    }
    
    _tprintf(_T("Connected to server!\n\n"));
    
    // Set pipe to message mode
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);
    
    // Send messages
    for (int i = 1; i <= messageCount; i++) {
        TCHAR message[256];
        _sntprintf(message, 256, _T("Message #%d from client %d"), i, clientId);
        
        _tprintf(_T("Sending: %s\n"), message);
        
        if (!SendLogMessage(hPipe, clientId, message)) {
            _tprintf(_T("Failed to send message!\n"));
            break;
        }
        
        // Wait for acknowledgment
        TCHAR response[64];
        DWORD bytesRead;
        if (ReadFile(hPipe, response, sizeof(response), &bytesRead, NULL)) {
            _tprintf(_T("  Server response: %s\n"), response);
        }
        
        // Delay before next message
        if (i < messageCount) {
            Sleep(delayMs);
        }
    }
    
    _tprintf(_T("\nAll messages sent. Disconnecting...\n"));
    
    CloseHandle(hPipe);
    
    _tprintf(_T("Client finished.\n"));
    
    return 0;
}

BOOL SendLogMessage(HANDLE hPipe, DWORD clientId, const TCHAR* message)
{
    LogMessage msg;
    DWORD bytesWritten;
    
    msg.clientId = clientId;
    _tcsncpy(msg.message, message, BUFFER_SIZE - 1);
    msg.message[BUFFER_SIZE - 1] = _T('\0');
    
    BOOL result = WriteFile(
        hPipe,
        &msg,
        sizeof(LogMessage),
        &bytesWritten,
        NULL
    );
    
    if (!result) {
        _tprintf(_T("WriteFile failed (error %d)\n"), GetLastError());
        return FALSE;
    }
    
    FlushFileBuffers(hPipe);
    
    return TRUE;
}
