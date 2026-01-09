# Building Sn Compiler on Windows

This document provides step-by-step instructions for building the Sn compiler on Windows using Microsoft Visual Studio (MSVC) and CMake.

## Prerequisites

### Required Software

1. **Visual Studio 2019 or later** with the "Desktop development with C++" workload
   - Download from: https://visualstudio.microsoft.com/
   - During installation, select "Desktop development with C++"
   - This includes the MSVC compiler (cl.exe) and Windows SDK

2. **CMake 3.16 or later**
   - Download from: https://cmake.org/download/
   - During installation, select "Add CMake to system PATH"
   - Alternatively, install via:
     - Visual Studio Installer: Individual Components > CMake tools for Windows
     - winget: `winget install Kitware.CMake`
     - Chocolatey: `choco install cmake`

### Verifying Prerequisites

Open a command prompt and verify the tools are installed:

```cmd
cmake --version
```

Expected output: `cmake version 3.x.x` (where x.x is 16 or higher)

## Building with build.bat (Recommended)

The easiest way to build on Windows is using the provided `build.bat` script, which automatically detects Visual Studio and configures the build environment.

### Basic Build

```cmd
build.bat
```

This will:
1. Detect your Visual Studio installation
2. Configure the MSVC compiler environment
3. Run CMake to generate build files
4. Build the compiler and test binary

### Build Options

```cmd
build.bat --help         Show all options
build.bat --debug        Build with debug symbols
build.bat --clean        Clean build (remove previous build files)
build.bat --test         Build and run unit tests
build.bat --verbose      Show detailed build output
```

### Output Files

After a successful build:
- `bin\sn.exe` - The Sn compiler
- `bin\tests.exe` - Unit test runner

## Building with CMake Manually

If you prefer to run CMake commands manually or need more control over the build process:

### Step 1: Open Developer Command Prompt

Open "Developer Command Prompt for VS 2019/2022" from the Start menu, or run:

```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
```

### Step 2: Configure with CMake

```cmd
cmake -S . -B build -G "NMake Makefiles"
```

Or for Visual Studio solution files:

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

### Step 3: Build

For NMake:
```cmd
cmake --build build
```

For Visual Studio:
```cmd
cmake --build build --config Release
```

Or open `build\sn.sln` in Visual Studio and build from the IDE.

### Build Options

Configure debug build:
```cmd
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSN_DEBUG=ON
```

## Running Tests

### Unit Tests

After building, run the unit tests:

```cmd
bin\tests.exe
```

### Integration Tests

Integration tests can be run using the PowerShell script:

```powershell
# Run all integration tests
.\scripts\run_integration_test.ps1 -All

# Run all tests of a specific type
.\scripts\run_integration_test.ps1 -TestType integration -All
.\scripts\run_integration_test.ps1 -TestType integration-errors -All
.\scripts\run_integration_test.ps1 -TestType explore -All
.\scripts\run_integration_test.ps1 -TestType explore-errors -All

# Run a single test file
.\scripts\run_integration_test.ps1 -TestFile tests\integration\hello_world.sn

# Specify a different compiler
.\scripts\run_integration_test.ps1 -All -Compiler path\to\sn.exe

# Show help
.\scripts\run_integration_test.ps1 -Help
```

The script will:
- Compile each `.sn` test file using `bin\sn.exe`
- Execute the resulting executable
- Compare the output against the corresponding `.expected` file
- Report pass/fail status with colored output

Or using the batch file for quick testing:

```cmd
test.bat
```

## Troubleshooting

### "cl.exe not found"

**Problem:** The MSVC compiler is not in your PATH.

**Solution:**
- Run `build.bat` which auto-detects Visual Studio, or
- Open "Developer Command Prompt for VS" instead of a regular command prompt, or
- Manually run vcvarsall.bat as shown in Step 1 above

### "CMake not found"

**Problem:** CMake is not installed or not in your PATH.

**Solution:**
- Install CMake from https://cmake.org/download/
- Ensure "Add CMake to system PATH" was selected during installation
- Restart your command prompt after installation

### "Visual Studio not found"

**Problem:** Visual Studio with C++ tools is not installed.

**Solution:**
- Install Visual Studio 2019 or later
- In Visual Studio Installer, ensure "Desktop development with C++" workload is selected
- Restart your computer after installation

### "Build fails with compile errors"

**Problem:** Code compilation errors.

**Solution:**
- Run `build.bat --verbose` to see detailed error messages
- Ensure you're using a supported Visual Studio version (2019+)
- Try a clean build: `build.bat --clean`

### "nmake: command not found"

**Problem:** NMake is not in your PATH.

**Solution:**
- Use the Developer Command Prompt, not a regular command prompt
- Alternatively, use Visual Studio generator instead of NMake:
  ```cmd
  cmake -S . -B build -G "Visual Studio 17 2022"
  ```

## Using the Compiler

After building, compile Sn source files:

```cmd
bin\sn.exe samples\hello_world.sn -o hello.exe
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

## Alternative: MinGW/MSYS2

While MSVC is the recommended toolchain on Windows, you can also build using MinGW or MSYS2:

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 MINGW64 terminal
3. Install dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make
   ```
4. Build using Make:
   ```bash
   make build
   make test
   ```

## Cross-Platform Notes

The Sn compiler and its build system support both Windows (MSVC) and Linux (GCC/Clang). The CMakeLists.txt automatically detects the platform and applies appropriate compiler flags.

Key differences:
- Windows uses MSVC's `/` flag syntax, Linux uses GCC's `-` flag syntax
- Windows executables have `.exe` extension
- Path separators: Windows uses `\`, Linux uses `/` (the compiler handles both)

For Linux build instructions, see the main [README.md](README.md).
