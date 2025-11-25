# Task 7: Dynamic Link Library (DLL)

## Description
Demonstration of creating and using a Windows Dynamic Link Library (DLL).

- **math_library.dll**: A simple DLL exporting mathematical functions (`Add`, `Subtract`, `Multiply`, `Divide`).
- **client.exe**: A console application that loads the DLL at runtime (Explicit Linking) using `LoadLibrary` and `GetProcAddress`, executes the functions, and then unloads the DLL.

## Build and Run

### Prerequisites
- MinGW-w64
- Wine

### Using Script
```bash
./run.sh
```

### Manual Build
```bash
make
wine client.exe
```
