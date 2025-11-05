#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>

#define BUFFER_SIZE (64 * 1024)  // 64KB - оптимальный размер для SSD
#define NUM_ASYNC_OPS 8         // Увеличено для лучшего параллелизма
#define MAX_ASYNC_OPS 32
#define TEST_FILE_SIZE (1024 * 1024 * 1024)  // 1 GB

// Структура для контекста асинхронной операции
typedef struct {
    OVERLAPPED overlapped;
    BYTE* buffer;
    DWORD bytesTransferred;
    BOOL operationPending;
    int operationId;
    enum { OP_READ, OP_WRITE, OP_IDLE } type;
} AsyncOperation;

// Прототипы функций
BOOL CreateTestFile(const TCHAR* filename, DWORD size);
BOOL ProcessFileAsyncOptimized(const TCHAR* inputFile, const TCHAR* outputFile, int numOps);
BOOL ProcessFileSync(const TCHAR* inputFile, const TCHAR* outputFile);
void ProcessBuffer(BYTE* buffer, DWORD size);
double GetElapsedTime(clock_t start, clock_t end);
void PrintProgress(DWORD current, DWORD total);
DWORD GetOptimalBufferSize();

// Глобальные переменные для производительности
static volatile LONG g_activeOperations = 0;
static volatile LONG g_totalProcessed = 0;

int _tmain(int argc, TCHAR* argv[])
{
    const TCHAR* testFile = _T("test_input.dat");
    const TCHAR* asyncOutput = _T("async_output.dat");
    const TCHAR* syncOutput = _T("sync_output.dat");
    clock_t start, end;
    double asyncTime, syncTime;
    int numAsyncOps = NUM_ASYNC_OPS;
    
    // Парсинг аргументов командной строки
    if (argc > 1) {
        numAsyncOps = _ttoi(argv[1]);
        if (numAsyncOps < 1 || numAsyncOps > MAX_ASYNC_OPS) {
            _tprintf(_T("Invalid number of async operations. Using default: %d\n"), NUM_ASYNC_OPS);
            numAsyncOps = NUM_ASYNC_OPS;
        }
    }
    
    _tprintf(_T("=== Optimized Asynchronous File I/O Demo ===\n"));
    _tprintf(_T("Optimal buffer size: %d KB\n"), BUFFER_SIZE / 1024);
    _tprintf(_T("Number of parallel async operations: %d\n\n"), numAsyncOps);
    
    // Создание тестового файла
    _tprintf(_T("Creating test file (%d MB)...\n"), TEST_FILE_SIZE / (1024 * 1024));
    if (!CreateTestFile(testFile, TEST_FILE_SIZE)) {
        _tprintf(_T("Failed to create test file!\n"));
        return 1;
    }
    _tprintf(_T("Test file created successfully.\n\n"));
    
    // Асинхронная обработка с оптимизациями
    _tprintf(_T("Processing file with OPTIMIZED ASYNC I/O (%d parallel ops)...\n"), numAsyncOps);
    start = clock();
    if (!ProcessFileAsyncOptimized(testFile, asyncOutput, numAsyncOps)) {
        _tprintf(_T("Async processing failed!\n"));
        return 1;
    }
    end = clock();
    asyncTime = GetElapsedTime(start, end);
    _tprintf(_T("\nOptimized async processing completed in %.3f seconds\n\n"), asyncTime);
    
    // Синхронная обработка для сравнения
    _tprintf(_T("Processing file with SYNC I/O...\n"));
    start = clock();
    if (!ProcessFileSync(testFile, syncOutput)) {
        _tprintf(_T("Sync processing failed!\n"));
        return 1;
    }
    end = clock();
    syncTime = GetElapsedTime(start, end);
    _tprintf(_T("\nSync processing completed in %.3f seconds\n\n"), syncTime);
    
    // Сравнение производительности
    _tprintf(_T("=== Performance Comparison ===\n"));
    _tprintf(_T("Optimized Async time: %.3f seconds (%d parallel ops)\n"), asyncTime, numAsyncOps);
    _tprintf(_T("Sync time:           %.3f seconds\n"), syncTime);
    if (asyncTime < syncTime) {
        _tprintf(_T("Speedup:             %.2fx faster with async I/O!\n"), syncTime / asyncTime);
    } else {
        _tprintf(_T("Note: Async was %.2fx slower\n"), asyncTime / syncTime);
    }
    
    return 0;
}

BOOL CreateTestFile(const TCHAR* filename, DWORD size)
{
    HANDLE hFile;
    BYTE* buffer;
    DWORD written, totalWritten = 0;
    DWORD bufferSize = BUFFER_SIZE;
    
    // Используем буферизацию для создания файла
    hFile = CreateFile(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        _tprintf(_T("CreateFile failed for test file (error %d)\n"), GetLastError());
        return FALSE;
    }
    
    // Выделяем буфер
    buffer = (BYTE*)malloc(bufferSize);
    if (!buffer) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    // Заполняем буфер псевдослучайными данными
    for (DWORD i = 0; i < bufferSize; i++) {
        buffer[i] = (BYTE)((i * 13 + 7) % 256);  // Более случайный паттерн
    }
    
    while (totalWritten < size) {
        DWORD toWrite = min(bufferSize, size - totalWritten);
        
        if (!WriteFile(hFile, buffer, toWrite, &written, NULL)) {
            free(buffer);
            CloseHandle(hFile);
            return FALSE;
        }
        
        totalWritten += written;
        
        // Прогресс каждые 5%
        if (totalWritten % (size / 20) < bufferSize) {
            PrintProgress(totalWritten, size);
        }
    }
    
    free(buffer);
    CloseHandle(hFile);
    return TRUE;
}

BOOL ProcessFileAsyncOptimized(const TCHAR* inputFile, const TCHAR* outputFile, int numOps)
{
    HANDLE hInput, hOutput;
    AsyncOperation* ops;
    HANDLE* events;
    DWORD i, eventCount;
    LARGE_INTEGER fileSize;
    LARGE_INTEGER readPos = {0}, writePos = {0};
    DWORD lastProgress = 0;
    
    // Выделяем память для операций
    ops = (AsyncOperation*)malloc(sizeof(AsyncOperation) * numOps);
    events = (HANDLE*)malloc(sizeof(HANDLE) * numOps * 2);
    if (!ops || !events) {
        _tprintf(_T("Memory allocation failed\n"));
        if (ops) free(ops);
        if (events) free(events);
        return FALSE;
    }
    
    // Открываем файлы с оптимизированными флагами
    hInput = CreateFile(
        inputFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING,
        NULL
    );
    
    if (hInput == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Failed to open input file (error %d)\n"), GetLastError());
        free(ops);
        free(events);
        return FALSE;
    }
    
    // Получаем размер файла
    if (!GetFileSizeEx(hInput, &fileSize)) {
        CloseHandle(hInput);
        free(ops);
        free(events);
        return FALSE;
    }
    
    // Выходной файл - с буферизацией для лучшей производительности
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
        _tprintf(_T("Failed to open output file (error %d)\n"), GetLastError());
        CloseHandle(hInput);
        free(ops);
        free(events);
        return FALSE;
    }
    
    // Инициализация асинхронных операций
    for (i = 0; i < numOps; i++) {
        ZeroMemory(&ops[i], sizeof(AsyncOperation));
        ops[i].buffer = (BYTE*)_aligned_malloc(BUFFER_SIZE, 4096);  // Выравнивание для FILE_FLAG_NO_BUFFERING
        if (!ops[i].buffer) {
            // Cleanup on failure
            for (DWORD j = 0; j < i; j++) {
                _aligned_free(ops[j].buffer);
            }
            CloseHandle(hInput);
            CloseHandle(hOutput);
            free(ops);
            free(events);
            return FALSE;
        }
        
        ops[i].overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ops[i].operationId = i;
        ops[i].type = OP_IDLE;
        events[i] = ops[i].overlapped.hEvent;
    }
    eventCount = numOps;
    
    // Начинаем с запуска операций чтения
    DWORD activeReads = 0;
    for (i = 0; i < numOps && readPos.QuadPart < fileSize.QuadPart; i++) {
        ops[i].overlapped.Offset = readPos.LowPart;
        ops[i].overlapped.OffsetHigh = readPos.HighPart;
        ops[i].type = OP_READ;
        
        if (!ReadFile(hInput, ops[i].buffer, BUFFER_SIZE, NULL, &ops[i].overlapped)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                _tprintf(_T("ReadFile failed (error %d)\n"), GetLastError());
                goto cleanup;
            }
        }
        
        readPos.QuadPart += BUFFER_SIZE;
        activeReads++;
        InterlockedIncrement(&g_activeOperations);
    }
    
    // Главный цикл обработки
    DWORD completedOps = 0;
    while (completedOps < numOps || g_activeOperations > 0) {
        // Ожидаем завершения хотя бы одной операции
        DWORD waitResult = WaitForMultipleObjects(
            min(eventCount, MAXIMUM_WAIT_OBJECTS), 
            events, 
            FALSE, 
            100  // Таймаут 100ms для обработки прогресса
        );
        
        if (waitResult == WAIT_FAILED) {
            _tprintf(_T("WaitForMultipleObjects failed (error %d)\n"), GetLastError());
            break;
        }
        
        // Обрабатываем завершенные операции
        for (i = 0; i < numOps; i++) {
            if (ops[i].type != OP_IDLE) {
                DWORD bytesTransferred;
                if (GetOverlappedResult(hInput, &ops[i].overlapped, &bytesTransferred, FALSE)) {
                    // Операция завершена успешно
                    ResetEvent(ops[i].overlapped.hEvent);
                    
                    if (ops[i].type == OP_READ) {
                        // Чтение завершено - обрабатываем и начинаем запись
                        ops[i].bytesTransferred = bytesTransferred;
                        ProcessBuffer(ops[i].buffer, bytesTransferred);
                        
                        // Начинаем асинхронную запись
                        ops[i].overlapped.Offset = ops[i].overlapped.Offset;  // Та же позиция
                        ops[i].overlapped.OffsetHigh = ops[i].overlapped.OffsetHigh;
                        ops[i].type = OP_WRITE;
                        
                        if (!WriteFile(hOutput, ops[i].buffer, bytesTransferred, 
                                     NULL, &ops[i].overlapped)) {
                            if (GetLastError() != ERROR_IO_PENDING) {
                                _tprintf(_T("WriteFile failed (error %d)\n"), GetLastError());
                                goto cleanup;
                            }
                        }
                        
                        // Обновляем прогресс
                        InterlockedAdd(&g_totalProcessed, bytesTransferred);
                        DWORD currentProgress = g_totalProcessed;
                        if (currentProgress - lastProgress > fileSize.QuadPart / 50) {
                            PrintProgress(currentProgress, fileSize.QuadPart);
                            lastProgress = currentProgress;
                        }
                        
                    } else if (ops[i].type == OP_WRITE) {
                        // Запись завершена - освобождаем слот
                        InterlockedDecrement(&g_activeOperations);
                        completedOps++;
                        ops[i].type = OP_IDLE;
                        
                        // Запускаем новую операцию чтения, если есть данные
                        if (readPos.QuadPart < fileSize.QuadPart) {
                            ops[i].overlapped.Offset = readPos.LowPart;
                            ops[i].overlapped.OffsetHigh = readPos.HighPart;
                            ops[i].type = OP_READ;
                            
                            if (!ReadFile(hInput, ops[i].buffer, BUFFER_SIZE, 
                                        NULL, &ops[i].overlapped)) {
                                if (GetLastError() != ERROR_IO_PENDING) {
                                    _tprintf(_T("ReadFile failed (error %d)\n"), GetLastError());
                                    goto cleanup;
                                }
                            }
                            
                            readPos.QuadPart += BUFFER_SIZE;
                            InterlockedIncrement(&g_activeOperations);
                        }
                    }
                }
            }
        }
        
        // Запускаем новые операции чтения для свободных слотов
        if (readPos.QuadPart < fileSize.QuadPart) {
            for (i = 0; i < numOps && readPos.QuadPart < fileSize.QuadPart; i++) {
                if (ops[i].type == OP_IDLE) {
                    ops[i].overlapped.Offset = readPos.LowPart;
                    ops[i].overlapped.OffsetHigh = readPos.HighPart;
                    ops[i].type = OP_READ;
                    
                    if (!ReadFile(hInput, ops[i].buffer, BUFFER_SIZE, 
                                NULL, &ops[i].overlapped)) {
                        if (GetLastError() != ERROR_IO_PENDING) {
                            _tprintf(_T("ReadFile failed (error %d)\n"), GetLastError());
                            goto cleanup;
                        }
                    }
                    
                    readPos.QuadPart += BUFFER_SIZE;
                    InterlockedIncrement(&g_activeOperations);
                }
            }
        }
    }
    
    PrintProgress(fileSize.QuadPart, fileSize.QuadPart);
    
cleanup:
    // Очистка ресурсов
    for (i = 0; i < numOps; i++) {
        if (ops[i].overlapped.hEvent) {
            CloseHandle(ops[i].overlapped.hEvent);
        }
        if (ops[i].buffer) {
            _aligned_free(ops[i].buffer);
        }
    }
    
    free(ops);
    free(events);
    CloseHandle(hInput);
    CloseHandle(hOutput);
    
    return TRUE;
}

BOOL ProcessFileSync(const TCHAR* inputFile, const TCHAR* outputFile)
{
    HANDLE hInput, hOutput;
    BYTE* buffer;
    DWORD bytesRead, bytesWritten;
    LARGE_INTEGER fileSize;
    DWORD totalProcessed = 0;
    
    buffer = (BYTE*)malloc(BUFFER_SIZE);
    if (!buffer) {
        return FALSE;
    }
    
    // Открываем файлы с оптимизацией для последовательного доступа
    hInput = CreateFile(
        inputFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    
    if (hInput == INVALID_HANDLE_VALUE) {
        free(buffer);
        return FALSE;
    }
    
    GetFileSizeEx(hInput, &fileSize);
    
    hOutput = CreateFile(
        outputFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    
    if (hOutput == INVALID_HANDLE_VALUE) {
        free(buffer);
        CloseHandle(hInput);
        return FALSE;
    }
    
    // Простой цикл чтение-обработка-запись
    while (ReadFile(hInput, buffer, BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0) {
        ProcessBuffer(buffer, bytesRead);
        
        if (!WriteFile(hOutput, buffer, bytesRead, &bytesWritten, NULL)) {
            free(buffer);
            CloseHandle(hInput);
            CloseHandle(hOutput);
            return FALSE;
        }
        
        totalProcessed += bytesRead;
        
        // Показываем прогресс
        if (totalProcessed % (fileSize.QuadPart / 20) < BUFFER_SIZE) {
            PrintProgress(totalProcessed, fileSize.QuadPart);
        }
    }
    
    free(buffer);
    CloseHandle(hInput);
    CloseHandle(hOutput);
    
    return TRUE;
}

void ProcessBuffer(BYTE* buffer, DWORD size)
{
    // Оптимизированная обработка - использует векторные операции
    DWORD i;
    for (i = 0; i < size; i++) {
        // Более легкая обработка для демонстрации I/O
        buffer[i] = ~buffer[i];  // Простая инверсия битов - быстрее чем сдвиги
    }
}

double GetElapsedTime(clock_t start, clock_t end)
{
    return (double)(end - start) / CLOCKS_PER_SEC;
}

void PrintProgress(DWORD current, DWORD total)
{
    int percent = (int)((current * 100LL) / total);
    _tprintf(_T("\rProgress: %3d%% [%u/%u MB]"), percent, 
             current / (1024 * 1024), total / (1024 * 1024));
}