# Building Sn Compiler on Windows

This document provides step-by-step instructions for building the Sn compiler on Windows.

## Prerequisites

### Required Software

Install the following using winget (recommended):

```cmd
winget install MartinStorsjo.LLVM-MinGW.UCRT
winget install Kitware.CMake
```

Optionally, for faster builds:
```cmd
winget install Ninja-build.Ninja
```

That's it! No Visual Studio required.

### Alternative Installation Methods

**LLVM-MinGW:**
- Direct download: https://github.com/mstorsjo/llvm-mingw/releases

**CMake:**
- Direct download: https://cmake.org/download/
- Chocolatey: `choco install cmake`

### Verifying Prerequisites

Open a new command prompt (to pick up PATH changes) and verify:

```cmd
clang --version
cmake --version
```

## Building with build.bat (Recommended)

The easiest way to build on Windows is using the provided `build.bat` script:

### Basic Build

```cmd
build.bat
```

This will:
1. Detect Clang and CMake
2. Configure and run the build
3. Generate runtime object files for the compiler backend

### Build Options

```cmd
build.bat --help         Show all options
build.bat --debug        Build with debug symbols and AddressSanitizer
build.bat --clean        Clean build (remove previous build files)
build.bat --test         Build and run unit tests
build.bat --verbose      Show detailed build output
```

### Output Files

After a successful build:
- `bin\sn.exe` - The Sn compiler
- `bin\tests.exe` - Unit test runner
- `bin\lib\clang\*.o` - Runtime object files

## Building with CMake Manually

If you prefer to run CMake commands manually:

### Using Ninja (fastest)

```cmd
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang
cmake --build build
```

### Using MinGW Make

```cmd
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=clang
cmake --build build
```

### Build Options

Debug build with AddressSanitizer:
```cmd
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DSN_DEBUG=ON -DSN_ASAN=ON
cmake --build build
```

## Running Tests

### Unit Tests

```cmd
bin\tests.exe
```

### Integration Tests

Using the batch file (recommended):

```cmd
test.bat                      Run all tests
test.bat --unit               Run unit tests only
test.bat --integration        Run integration tests only
test.bat --explore            Run exploratory tests only
```

Using PowerShell directly:

```powershell
# Run all integration tests
.\scripts\run_integration_test.ps1 -TestType integration -All

# Run a single test file
.\scripts\run_integration_test.ps1 -TestFile tests\integration\hello_world.sn

# Show help
.\scripts\run_integration_test.ps1 -Help
```

## Using the Compiler

After building, compile Sn source files:

```cmd
bin\sn.exe samples\main.sn -o hello.exe
hello.exe
```

### Compiler Options

```cmd
bin\sn.exe <source.sn> [options]

Output options:
  -o <file>          Output executable name
  --emit-c           Only output C code, don't compile
  --keep-c           Keep intermediate C file

Debug options:
  -v                 Verbose mode
  -g                 Debug build
  -l <level>         Log level (0-4)

Optimization:
  -O0                No optimization
  -O1                Basic optimization
  -O2                Full optimization (default)
```

## Troubleshooting

### "clang not found"

**Problem:** Clang is not in your PATH.

**Solution:**
- Ensure LLVM-MinGW is installed: `winget install MartinStorsjo.LLVM-MinGW.UCRT`
- Open a new command prompt after installation
- Verify with: `clang --version`

### "CMake not found"

**Problem:** CMake is not installed or not in your PATH.

**Solution:**
- Install CMake: `winget install Kitware.CMake`
- Open a new command prompt after installation
- Verify with: `cmake --version`

### "ninja not found" or "mingw32-make not found"

**Problem:** No build tool available.

**Solution:**
- LLVM-MinGW includes `mingw32-make`, which build.bat will use automatically
- For faster builds, install Ninja: `winget install Ninja-build.Ninja`

### "Runtime object not found: bin\lib\clang\arena.o"

**Problem:** Runtime objects weren't built.

**Solution:**
- Run a clean rebuild: `build.bat --clean`
- This ensures the runtime object files are generated

### Build fails with compile errors

**Solution:**
- Run `build.bat --verbose` to see detailed error messages
- Try a clean build: `build.bat --clean`
- Ensure you have the latest LLVM-MinGW

## Advanced: Using Visual Studio (Optional)

If you prefer Visual Studio or need clang-cl compatibility:

1. Install Visual Studio 2022 with "Desktop development with C++" workload
2. Install LLVM: `winget install LLVM.LLVM`
3. The build scripts will auto-detect clang-cl and use MSVC-compatible mode

## Cross-Platform Notes

The Sn compiler supports Windows, Linux, and macOS. The CMakeLists.txt automatically detects the platform and applies appropriate settings.

Key differences on Windows:
- Executables have `.exe` extension
- Runtime uses Windows APIs (ws2_32 for networking, bcrypt for random)
- Path separators: both `\` and `/` work

For Linux/macOS build instructions, see the main [README.md](README.md).
