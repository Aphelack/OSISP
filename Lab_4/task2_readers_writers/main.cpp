#include <iostream>
#include <windows.h>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>

HANDLE readSemaphore;
HANDLE writeMutex;
int readersCount = 0;
int writersWaiting = 0;
int shared_data = 0;
CRITICAL_SECTION countLock;

std::mutex statsMutex;
int successfulReads = 0;
int successfulWrites = 0;
int failedReads = 0;
int failedWrites = 0;
double totalReadTime = 0.0;
double totalWriteTime = 0.0;
double totalReadBlockTime = 0.0;
double totalWriteBlockTime = 0.0;

std::atomic<bool> isRunning(true);

void reader(int id, int duration) {
    while (isRunning) {
        auto blockStartTime = std::chrono::high_resolution_clock::now();
        EnterCriticalSection(&countLock);
        if (readersCount == 0 && writersWaiting > 0) {
            LeaveCriticalSection(&countLock);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            {
                std::lock_guard<std::mutex> lock(statsMutex);
                failedReads++;
                std::cout << "Reader " << id << " failed to read (waiting for writers)" << std::endl;
            }
            continue;
        }

        readersCount++;
        LeaveCriticalSection(&countLock);
        auto blockEndTime = std::chrono::high_resolution_clock::now();
        totalReadBlockTime += std::chrono::duration<double>(blockEndTime - blockStartTime).count();

        auto startTime = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        std::cout << "Reader " << id << " reads data: " << shared_data << std::endl;
        auto endTime = std::chrono::high_resolution_clock::now();
        EnterCriticalSection(&countLock);
        readersCount--;
        if (readersCount == 0) {
            ReleaseSemaphore(readSemaphore, 1, NULL);
        }
        LeaveCriticalSection(&countLock);
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            successfulReads++;
            totalReadTime += std::chrono::duration<double>(endTime - startTime).count();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void writer(int id, int duration) {
    while (isRunning) {
        auto blockStartTime = std::chrono::high_resolution_clock::now();
        ++writersWaiting;
        DWORD waitResult = WaitForSingleObject(writeMutex, 10); // Тайм-аут 10 миллисекунд
        if (waitResult == WAIT_TIMEOUT) {
            std::lock_guard<std::mutex> lock(statsMutex);
            failedWrites++;
            --writersWaiting;
            std::cout << "Writer " << id << " timed out waiting to write." << std::endl;
            continue;
        }
        auto blockEndTime = std::chrono::high_resolution_clock::now();
        totalWriteBlockTime += std::chrono::duration<double>(blockEndTime - blockStartTime).count();

        shared_data++;
        std::cout << "Writer " << id << " writes data: " << shared_data << std::endl;
        ReleaseMutex(writeMutex);
        --writersWaiting;
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            successfulWrites++;
            totalWriteTime += duration / 1000.0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void cleanup() {
    CloseHandle(readSemaphore);
    CloseHandle(writeMutex);
    DeleteCriticalSection(&countLock);
}

int main() {
    int numReaders = 3;
    int numWriters = 4;
    int readDuration = 200;
    int writeDuration = 1000;

    readSemaphore = CreateSemaphore(NULL, numReaders, numReaders, NULL);
    writeMutex = CreateMutex(NULL, FALSE, NULL);
    InitializeCriticalSection(&countLock);
    std::vector<std::thread> threads;

    for (int i = 0; i < numReaders; ++i) {
        threads.emplace_back(reader, i + 1, readDuration);
    }

    for (int i = 0; i < numWriters; ++i) {
        threads.emplace_back(writer, i + 1, writeDuration);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    isRunning = false;
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "\n=== Simulation Results ===\n";
    std::cout << "Successful reads: " << successfulReads << std::endl;
    std::cout << "Failed reads: " << failedReads << std::endl;
    std::cout << "Successful writes: " << successfulWrites << std::endl;
    std::cout << "Average read time: " << (successfulReads ? totalReadTime / successfulReads : 0) << " sec." << std::endl;
    std::cout << "Average write time: " << (successfulWrites ? totalWriteTime / successfulWrites : 0) << " sec." << std::endl;
    std::cout << "Total read block time: " << totalReadBlockTime << " sec." << std::endl;
    std::cout << "Total write block time: " << totalWriteBlockTime << " sec." << std::endl;
    cleanup();
    return 0;
}
//g++ -o readers_writers.exe main.cpp -lwinmm
//./readers_writers.exe