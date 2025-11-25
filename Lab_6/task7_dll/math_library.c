#include <windows.h>

// DLL entry point (optional, but good practice to show)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Code to run when the DLL is loaded
            break;
        case DLL_PROCESS_DETACH:
            // Code to run when the DLL is unloaded
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

// Exported functions
// extern "C" is needed in C++ to prevent name mangling, but in C it's default.
// We use __declspec(dllexport) to mark functions for export.

__declspec(dllexport) int Add(int a, int b) {
    return a + b;
}

__declspec(dllexport) int Subtract(int a, int b) {
    return a - b;
}

__declspec(dllexport) int Multiply(int a, int b) {
    return a * b;
}

__declspec(dllexport) double Divide(int a, int b) {
    if (b == 0) return 0.0;
    return (double)a / b;
}
