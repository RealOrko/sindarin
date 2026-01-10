# Building Sindarin

This document explains how to build the Sindarin compiler from source on Linux, macOS, and Windows using CMake.

## Prerequisites

All platforms require:
- **CMake** 3.16 or later
- **Ninja** build system (recommended) or Make
- A C99-compatible compiler (GCC or Clang)

## Quick Start

```bash
# Configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Verify
bin/sn --version
```

## Platform-Specific Instructions

### Linux

**Install dependencies (Ubuntu/Debian):**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
```

**Install dependencies (Fedora/RHEL):**

```bash
sudo dnf install -y gcc cmake ninja-build
```

**Install dependencies (Arch Linux):**

```bash
sudo pacman -S gcc cmake ninja
```

**Configure and build:**

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Verify the build:**

```bash
bin/sn --version
bin/tests  # Run unit tests
```

### macOS

**Install dependencies (via Homebrew):**

```bash
brew install cmake ninja
```

macOS includes Clang via Xcode Command Line Tools. Install if needed:

```bash
xcode-select --install
```

**Configure and build:**

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Verify the build:**

```bash
bin/sn --version
bin/tests  # Run unit tests
```

### Windows

**Install dependencies (via winget):**

```powershell
winget install Kitware.CMake
winget install Ninja-build.Ninja
winget install LLVM.LLVM
```

After installation, you may need to restart your terminal or add the install locations to your PATH.

**Configure and build (from PowerShell or cmd):**

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Verify the build:**

```powershell
bin\sn.exe --version
bin\tests.exe
```

**Alternative: LLVM-MinGW**

The GitHub CI uses [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw/releases), a self-contained Clang toolchain with MinGW-w64. This is useful if you need a fully isolated toolchain:

1. Download from GitHub releases
2. Extract to `C:\llvm-mingw`
3. Add the `bin` directory to your PATH

## Build Outputs

After a successful build, you'll find:

| Path | Description |
|------|-------------|
| `bin/sn` (or `bin/sn.exe`) | The Sindarin compiler |
| `bin/tests` (or `bin/tests.exe`) | Unit test runner |
| `bin/lib/gcc/*.o` | Runtime objects (Linux/macOS) |
| `bin/lib/clang/*.o` | Runtime objects (Windows) |
| `bin/include/` | Runtime headers |
| `bin/*.cfg` | Backend configuration files |

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build type: `Release` or `Debug` |
| `CMAKE_C_COMPILER` | System default | C compiler: `gcc`, `clang`, etc. |
| `SN_DEBUG` | `OFF` | Enable debug symbols and reduced optimization |
| `SN_ASAN` | `OFF` | Enable AddressSanitizer (GCC/Clang only) |

**Debug build with AddressSanitizer:**

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug -DSN_DEBUG=ON -DSN_ASAN=ON
cmake --build build
```

## Running Tests

After building, run the test suites:

```bash
# Unit tests (fast, tests compiler internals)
bin/tests

# Integration tests (compile and run .sn files)
./scripts/run_tests.sh integration

# Integration error tests (verify compile errors)
./scripts/run_tests.sh integration-errors

# Exploratory tests (extended test cases)
./scripts/run_tests.sh explore
```

On Windows, use PowerShell:

```powershell
.\scripts\run_integration_test.ps1 -TestType integration -All
.\scripts\run_integration_test.ps1 -TestType explore -All
```

## Compiling Sindarin Programs

Once built, compile `.sn` files:

```bash
# Compile to executable
bin/sn myprogram.sn -o myprogram
./myprogram

# Emit C code only (don't compile)
bin/sn myprogram.sn --emit-c -o myprogram.c

# Debug build (with symbols, enables ASAN on Linux)
bin/sn myprogram.sn -g -o myprogram
```

## C Compiler Configuration

The Sindarin compiler uses a C backend. Configure it via environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `SN_CC` | `gcc` (Linux), `clang` (macOS/Windows) | C compiler |
| `SN_STD` | `c99` | C standard |
| `SN_DEBUG_CFLAGS` | Platform-specific | Debug mode flags |
| `SN_RELEASE_CFLAGS` | `-O3 -flto` | Release mode flags |
| `SN_CFLAGS` | (empty) | Additional compiler flags |
| `SN_LDFLAGS` | (empty) | Additional linker flags |
| `SN_LDLIBS` | (empty) | Additional libraries |

**Example: Using Clang on Linux:**

```bash
SN_CC=clang bin/sn myprogram.sn -o myprogram
```

**Example: Linking additional libraries:**

```bash
SN_LDLIBS="-lssl -lcrypto" bin/sn myprogram.sn -o myprogram
```

## Troubleshooting

### "Runtime object not found"

The compiler can't find pre-built runtime objects. Ensure CMake completed successfully and check that `bin/lib/gcc/` (Linux/macOS) or `bin/lib/clang/` (Windows) contains `.o` files.

### "C compiler not found"

Set `SN_CC` to your compiler path:

```bash
SN_CC=/usr/bin/gcc bin/sn myprogram.sn -o myprogram
```

### Windows: "clang not found"

If using winget-installed LLVM, restart your terminal or add to PATH:

```powershell
$env:PATH = "C:\Program Files\LLVM\bin;$env:PATH"
```

If using LLVM-MinGW:

```powershell
$env:PATH = "C:\llvm-mingw\bin;$env:PATH"
```

### macOS: AddressSanitizer errors

ASAN has known issues with signal handling on macOS. Debug builds on macOS disable ASAN by default. To explicitly disable:

```bash
SN_DEBUG_CFLAGS="-g" bin/sn myprogram.sn -g -o myprogram
```

## CI/CD Reference

The project uses GitHub Actions for continuous integration. See the workflow files for authoritative build configurations:

- `.github/workflows/linux.yml` - Linux builds with GCC
- `.github/workflows/macos.yml` - macOS builds with Clang
- `.github/workflows/windows.yml` - Windows builds with LLVM-MinGW
