#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>

#define BUFFER_SIZE 16384
#define NUM_ASYNC_OPS 8
#define MAX_ASYNC_OPS 16
#define TEST_FILE_SIZE (1024 * 1024 * 200)  // 200 MB

// Structure for async operation context
typedef struct {
    OVERLAPPED readOverlapped;
    OVERLAPPED writeOverlapped;
    BYTE buffer[BUFFER_SIZE];
    DWORD bytesRead;
    BOOL readComplete;
    BOOL writeComplete;
    int operationId;
    enum { STATE_IDLE, STATE_READING, STATE_PROCESSING, STATE_WRITING } state;
} AsyncOperation;

// Function declarations
BOOL CreateTestFile(const TCHAR* filename, DWORD size);
BOOL ProcessFileAsync(const TCHAR* inputFile, const TCHAR* outputFile, int numOps);
BOOL ProcessFileSync(const TCHAR* inputFile, const TCHAR* outputFile);
void ProcessBuffer(BYTE* buffer, DWORD size);
double GetElapsedTime(clock_t start, clock_t end);
void PrintProgress(int current, int total);

int _tmain(int argc, TCHAR* argv[])
{
    const TCHAR* testFile = _T("test_input.dat");
    const TCHAR* asyncOutput = _T("async_output.dat");
    const TCHAR* syncOutput = _T("sync_output.dat");
    clock_t start, end;
    double asyncTime, syncTime;
    int numAsyncOps = NUM_ASYNC_OPS;
    
    // Parse command line arguments
    if (argc > 1) {
        numAsyncOps = _ttoi(argv[1]);
        if (numAsyncOps < 1 || numAsyncOps > MAX_ASYNC_OPS) {
            _tprintf(_T("Invalid number of async operations. Using default: %d\n"), NUM_ASYNC_OPS);
            numAsyncOps = NUM_ASYNC_OPS;
        }
    }
    
    _tprintf(_T("=== Asynchronous File I/O Demo ===\n"));
    _tprintf(_T("Number of parallel async operations: %d\n\n"), numAsyncOps);
    
    // Create test file
    _tprintf(_T("Creating test file (%d MB)...\n"), TEST_FILE_SIZE / (1024 * 1024));
    if (!CreateTestFile(testFile, TEST_FILE_SIZE)) {
        _tprintf(_T("Failed to create test file!\n"));
        return 1;
    }
    _tprintf(_T("Test file created successfully.\n\n"));
    
    // Process file asynchronously
    _tprintf(_T("Processing file with ASYNC I/O (%d parallel ops)...\n"), numAsyncOps);
    start = clock();
    if (!ProcessFileAsync(testFile, asyncOutput, numAsyncOps)) {
        _tprintf(_T("Async processing failed!\n"));
        return 1;
    }
    end = clock();
    asyncTime = GetElapsedTime(start, end);
    _tprintf(_T("\nAsync processing completed in %.3f seconds\n\n"), asyncTime);
    
    // Process file synchronously
    _tprintf(_T("Processing file with SYNC I/O...\n"));
    start = clock();
    if (!ProcessFileSync(testFile, syncOutput)) {
        _tprintf(_T("Sync processing failed!\n"));
        return 1;
    }
    end = clock();
    syncTime = GetElapsedTime(start, end);
    _tprintf(_T("\nSync processing completed in %.3f seconds\n\n"), syncTime);
    
    // Show comparison
    _tprintf(_T("=== Performance Comparison ===\n"));
    _tprintf(_T("Async time: %.3f seconds (%d parallel ops)\n"), asyncTime, numAsyncOps);
    _tprintf(_T("Sync time:  %.3f seconds\n"), syncTime);
    if (asyncTime < syncTime) {
        _tprintf(_T("Speedup:    %.2fx faster with async I/O!\n"), syncTime / asyncTime);
    } else {
        _tprintf(_T("Note: Async was %.2fx slower (try running on slower storage or increase parallel ops)\n"), 
                asyncTime / syncTime);
    }
    
    _tprintf(_T("\nFiles created:\n"));
    _tprintf(_T("  - %s (test input)\n"), testFile);
    _tprintf(_T("  - %s (async output)\n"), asyncOutput);
    _tprintf(_T("  - %s (sync output)\n"), syncOutput);
    _tprintf(_T("\nTip: Run with argument to change parallel ops count (1-%d)\n"), MAX_ASYNC_OPS);
    _tprintf(_T("Example: async_io.exe 16\n"));
    
    return 0;
}

BOOL CreateTestFile(const TCHAR* filename, DWORD size)
{
    HANDLE hFile;
    BYTE buffer[BUFFER_SIZE];
    DWORD written, totalWritten = 0;
    DWORD i;
    
    hFile = CreateFile(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    // Fill buffer with pseudo-random data
    for (i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = (BYTE)(i % 256);
    }
    
    while (totalWritten < size) {
        DWORD toWrite = min(BUFFER_SIZE, size - totalWritten);
        
        if (!WriteFile(hFile, buffer, toWrite, &written, NULL)) {
            CloseHandle(hFile);
            return FALSE;
        }
        
        totalWritten += written;
        
        // Show progress every 10%
        if (totalWritten % (size / 10) < BUFFER_SIZE) {
            PrintProgress(totalWritten, size);
        }
    }
    
    CloseHandle(hFile);
    return TRUE;
}

BOOL ProcessFileAsync(const TCHAR* inputFile, const TCHAR* outputFile, int numOps)
{
    HANDLE hInput, hOutput;
    AsyncOperation* ops;
    DWORD i;
    LARGE_INTEGER fileSize;
    LARGE_INTEGER readPos = {0};
    DWORD totalProcessed = 0;
    DWORD activeOps = 0;
    
    // Allocate operations array dynamically
    ops = (AsyncOperation*)malloc(sizeof(AsyncOperation) * numOps);
    if (!ops) {
        _tprintf(_T("Failed to allocate memory for operations\n"));
        return FALSE;
    }
    
    // Open input file with FILE_FLAG_OVERLAPPED and NO_BUFFERING for better async performance
    hInput = CreateFile(
        inputFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    
    if (hInput == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Failed to open input file (error %d)\n"), GetLastError());
        return FALSE;
    }
    
    // Get file size
    if (!GetFileSizeEx(hInput, &fileSize)) {
        CloseHandle(hInput);
        return FALSE;
    }
    
    // Open output file with FILE_FLAG_OVERLAPPED
    hOutput = CreateFile(
        outputFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    
    if (hOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(hInput);
        return FALSE;
    }
    
    // Initialize async operations
    for (i = 0; i < numOps; i++) {
        ZeroMemory(&ops[i], sizeof(AsyncOperation));
        ops[i].readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ops[i].writeOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ops[i].operationId = i;
        ops[i].state = STATE_IDLE;
    }
    
    // Main async I/O pipeline loop
    BOOL allDataRead = FALSE;
    while (totalProcessed < fileSize.QuadPart) {
        // Phase 1: Issue new read operations for idle slots
        for (i = 0; i < numOps && !allDataRead; i++) {
            if (ops[i].state == STATE_IDLE && readPos.QuadPart < fileSize.QuadPart) {
                // Set file position for this read operation
                ops[i].readOverlapped.Offset = readPos.LowPart;
                ops[i].readOverlapped.OffsetHigh = readPos.HighPart;
                ResetEvent(ops[i].readOverlapped.hEvent);
                ops[i].readComplete = FALSE;
                ops[i].state = STATE_READING;
                
                // Start async read
                if (!ReadFile(hInput, ops[i].buffer, BUFFER_SIZE, 
                             NULL, &ops[i].readOverlapped)) {
                    if (GetLastError() != ERROR_IO_PENDING) {
                        _tprintf(_T("ReadFile failed (error %d)\n"), GetLastError());
                        goto cleanup;
                    }
                }
                
                readPos.QuadPart += BUFFER_SIZE;
                activeOps++;
                
                if (readPos.QuadPart >= fileSize.QuadPart) {
                    allDataRead = TRUE;
                }
            }
        }
        
        // Phase 2: Check for completed reads and process data
        for (i = 0; i < numOps; i++) {
            if (ops[i].state == STATE_READING) {
                DWORD bytesRead = 0;
                if (GetOverlappedResult(hInput, &ops[i].readOverlapped, &bytesRead, FALSE)) {
                    // Read completed - process the data immediately
                    ops[i].bytesRead = bytesRead;
                    ProcessBuffer(ops[i].buffer, bytesRead);
                    ops[i].state = STATE_PROCESSING;
                }
            }
        }
        
        // Phase 3: Issue async writes for processed data
        for (i = 0; i < numOps; i++) {
            if (ops[i].state == STATE_PROCESSING) {
                // Set write position (same as read position)
                ops[i].writeOverlapped.Offset = ops[i].readOverlapped.Offset;
                ops[i].writeOverlapped.OffsetHigh = ops[i].readOverlapped.OffsetHigh;
                ResetEvent(ops[i].writeOverlapped.hEvent);
                ops[i].writeComplete = FALSE;
                ops[i].state = STATE_WRITING;
                
                // Start async write (don't wait!)
                if (!WriteFile(hOutput, ops[i].buffer, ops[i].bytesRead, 
                              NULL, &ops[i].writeOverlapped)) {
                    if (GetLastError() != ERROR_IO_PENDING) {
                        _tprintf(_T("WriteFile failed (error %d)\n"), GetLastError());
                        goto cleanup;
                    }
                }
            }
        }
        
        // Phase 4: Check for completed writes and free slots
        for (i = 0; i < numOps; i++) {
            if (ops[i].state == STATE_WRITING) {
                DWORD bytesWritten = 0;
                if (GetOverlappedResult(hOutput, &ops[i].writeOverlapped, &bytesWritten, FALSE)) {
                    // Write completed - slot is now free
                    totalProcessed += bytesWritten;
                    ops[i].state = STATE_IDLE;
                    activeOps--;
                    
                    // Show progress
                    if (totalProcessed % (fileSize.QuadPart / 20) < BUFFER_SIZE) {
                        PrintProgress(totalProcessed, fileSize.QuadPart);
                    }
                }
            }
        }
        
        // Small sleep to prevent busy-waiting
        Sleep(0);
    }
    
    // Wait for all pending operations to complete
    for (i = 0; i < numOps; i++) {
        if (ops[i].state == STATE_READING) {
            DWORD bytesRead;
            GetOverlappedResult(hInput, &ops[i].readOverlapped, &bytesRead, TRUE);
        }
        if (ops[i].state == STATE_WRITING) {
            DWORD bytesWritten;
            GetOverlappedResult(hOutput, &ops[i].writeOverlapped, &bytesWritten, TRUE);
        }
    }
    
cleanup:
    // Clean up
    for (i = 0; i < numOps; i++) {
        if (ops[i].readOverlapped.hEvent) {
            CloseHandle(ops[i].readOverlapped.hEvent);
        }
        if (ops[i].writeOverlapped.hEvent) {
            CloseHandle(ops[i].writeOverlapped.hEvent);
        }
    }
    
    free(ops);
    CloseHandle(hInput);
    CloseHandle(hOutput);
    
    return TRUE;
}

BOOL ProcessFileSync(const TCHAR* inputFile, const TCHAR* outputFile)
{
    HANDLE hInput, hOutput;
    BYTE buffer[BUFFER_SIZE];
    DWORD bytesRead, bytesWritten;
    LARGE_INTEGER fileSize;
    DWORD totalProcessed = 0;
    
    // Open files synchronously
    hInput = CreateFile(
        inputFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hInput == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    GetFileSizeEx(hInput, &fileSize);
    
    hOutput = CreateFile(
        outputFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(hInput);
        return FALSE;
    }
    
    // Simple read-process-write loop
    while (ReadFile(hInput, buffer, BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0) {
        // Process the data
        ProcessBuffer(buffer, bytesRead);
        
        // Write processed data
        if (!WriteFile(hOutput, buffer, bytesRead, &bytesWritten, NULL)) {
            CloseHandle(hInput);
            CloseHandle(hOutput);
            return FALSE;
        }
        
        totalProcessed += bytesRead;
        
        // Show progress
        if (totalProcessed % (fileSize.QuadPart / 20) < BUFFER_SIZE) {
            PrintProgress(totalProcessed, fileSize.QuadPart);
        }
    }
    
    CloseHandle(hInput);
    CloseHandle(hOutput);
    
    return TRUE;
}

void ProcessBuffer(BYTE* buffer, DWORD size)
{
    DWORD i;
    // Lighter processing - single pass transformation
    // This allows I/O to dominate rather than CPU
    for (i = 0; i < size; i++) {
        // XOR with rotating pattern
        buffer[i] ^= (BYTE)(i & 0xFF);
        // Simple bit rotation
        buffer[i] = (buffer[i] << 1) | (buffer[i] >> 7);
    }
}

double GetElapsedTime(clock_t start, clock_t end)
{
    return (double)(end - start) / CLOCKS_PER_SEC;
}

void PrintProgress(int current, int total)
{
    int percent = (int)((current * 100LL) / total);
    _tprintf(_T("\rProgress: %d%%"), percent);
}
