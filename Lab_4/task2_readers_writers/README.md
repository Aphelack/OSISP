# Task 2: Readers-Writers Problem

## Description
Implementation of the Readers-Writers synchronization problem using Windows API primitives (Mutex, Semaphore, Critical Section).

The program demonstrates:
- Multiple reader threads accessing a shared resource concurrently.
- Writer threads accessing the shared resource exclusively.
- Prevention of race conditions and "dirty reads".
- Synchronization using `CreateSemaphore`, `CreateMutex`, `InitializeCriticalSection`, `WaitForSingleObject`.

## Build and Run

### Prerequisites
- MinGW-w64 (for cross-compilation on Linux)
- Wine (to run the Windows executable)
- CMake

### Using Script
```bash
./run.sh
```

### Manual Build (CMake)
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../toolchain-mingw64.cmake
make
wine readers_writers.exe
```

### Manual Build (Makefile)
```bash
make
wine readers_writers.exe
```
