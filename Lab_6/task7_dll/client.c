#include <stdio.h>
#include <windows.h>

// Define function pointers matching the DLL function signatures
typedef int (*AddFunc)(int, int);
typedef int (*SubFunc)(int, int);
typedef int (*MulFunc)(int, int);
typedef double (*DivFunc)(int, int);

int main() {
    HINSTANCE hDll;
    AddFunc Add;
    SubFunc Subtract;
    MulFunc Multiply;
    DivFunc Divide;

    // 1. Load the DLL
    printf("Loading math_library.dll...\n");
    hDll = LoadLibrary("math_library.dll");
    
    if (hDll == NULL) {
        printf("Could not load the DLL. Error code: %lu\n", GetLastError());
        return 1;
    }
    printf("DLL loaded successfully.\n");

    // 2. Get pointers to the exported functions
    Add = (AddFunc)GetProcAddress(hDll, "Add");
    Subtract = (SubFunc)GetProcAddress(hDll, "Subtract");
    Multiply = (MulFunc)GetProcAddress(hDll, "Multiply");
    Divide = (DivFunc)GetProcAddress(hDll, "Divide");

    // Check if functions were found
    if (!Add || !Subtract || !Multiply || !Divide) {
        printf("Could not locate the functions. Error code: %lu\n", GetLastError());
        FreeLibrary(hDll);
        return 1;
    }

    // 3. Use the functions
    int a = 10, b = 5;
    printf("\nPerforming calculations with a=%d, b=%d:\n", a, b);
    printf("Add:      %d\n", Add(a, b));
    printf("Subtract: %d\n", Subtract(a, b));
    printf("Multiply: %d\n", Multiply(a, b));
    printf("Divide:   %.2f\n", Divide(a, b));

    // 4. Unload the DLL
    printf("\nUnloading DLL...\n");
    FreeLibrary(hDll);
    printf("Done.\n");

    return 0;
}
