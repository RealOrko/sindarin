@echo off
setlocal EnableDelayedExpansion

REM ============================================================================
REM Sn Compiler - Windows Build Script
REM Uses CMake + Clang/MinGW toolchain
REM ============================================================================

REM Default configuration
set "BUILD_TYPE=Release"
set "CLEAN_BUILD=0"
set "RUN_TESTS=0"
set "VERBOSE=0"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :done_args
if /i "%~1"=="--debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set "CLEAN_BUILD=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--test" (
    set "RUN_TESTS=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--verbose" (
    set "VERBOSE=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="-h" goto :show_help
echo Unknown option: %~1
goto :show_help
:done_args

echo.
echo ============================================================
echo   Sn Compiler - Windows Build (Clang/MinGW)
echo ============================================================
echo.

REM ============================================================================
REM Step 1: Check for Clang
REM ============================================================================
echo [1/4] Checking for Clang...

where clang.exe >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: clang.exe not found in PATH.
    echo.
    echo Please install LLVM/Clang. Options:
    echo   - winget: winget install LLVM.LLVM
    echo   - choco:  choco install llvm
    echo   - Direct: https://releases.llvm.org/
    echo.
    echo Make sure the LLVM bin directory is in your PATH.
    echo.
    exit /b 1
)

for /f "tokens=3" %%v in ('clang --version 2^>^&1 ^| findstr /i /c:"clang version"') do (
    echo      Found: Clang %%v
    goto :done_clang_version
)
:done_clang_version

REM ============================================================================
REM Step 2: Check for CMake and Ninja
REM ============================================================================
echo.
echo [2/4] Checking for CMake and Ninja...

where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: CMake not found in PATH.
    echo.
    echo Please install CMake:
    echo   - winget: winget install Kitware.CMake
    echo   - choco:  choco install cmake
    echo   - Direct: https://cmake.org/download/
    echo.
    exit /b 1
)

for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /i /c:"cmake version"') do (
    echo      Found: CMake %%v
    goto :done_cmake_version
)
:done_cmake_version

REM Check for Ninja (preferred) or fall back to MinGW Make
where ninja.exe >nul 2>&1
if not errorlevel 1 (
    set "CMAKE_GENERATOR=Ninja"
    echo      Using: Ninja
    goto :done_generator
)

echo      Ninja not found, checking for MinGW Make...
where mingw32-make.exe >nul 2>&1
if not errorlevel 1 (
    set "CMAKE_GENERATOR=MinGW Makefiles"
    echo      Using: MinGW Makefiles
    goto :done_generator
)

echo.
echo ERROR: Neither Ninja nor mingw32-make found in PATH.
echo.
echo Please install Ninja ^(recommended^):
echo   - winget: winget install Ninja-build.Ninja
echo   - choco:  choco install ninja
echo.
echo Or install MinGW with make.
echo.
exit /b 1

:done_generator

REM ============================================================================
REM Step 3: Configure and Build
REM ============================================================================
echo.
echo [3/4] Building compiler ^(%BUILD_TYPE%^)...

set "BUILD_DIR=build"

REM Clean build if requested
if "%CLEAN_BUILD%"=="1" (
    echo      Cleaning previous build...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Configure with CMake
echo      Configuring CMake...

REM Build cmake command based on build type
if "%BUILD_TYPE%"=="Debug" (
    set "CMAKE_DEBUG_FLAGS=-DSN_DEBUG=ON -DSN_ASAN=ON"
) else (
    set "CMAKE_DEBUG_FLAGS="
)

if "%VERBOSE%"=="1" (
    cmake -S . -B "%BUILD_DIR%" -G "%CMAKE_GENERATOR%" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %CMAKE_DEBUG_FLAGS%
) else (
    cmake -S . -B "%BUILD_DIR%" -G "%CMAKE_GENERATOR%" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %CMAKE_DEBUG_FLAGS% >nul 2>&1
)

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed.
    echo.
    echo Run with --verbose flag to see detailed error messages:
    echo   build.bat --verbose
    echo.
    exit /b 1
)

REM Build
echo      Building...
if "%VERBOSE%"=="1" (
    cmake --build "%BUILD_DIR%"
) else (
    cmake --build "%BUILD_DIR%" >nul 2>&1
)

if errorlevel 1 (
    echo.
    echo ERROR: Build failed.
    echo.
    echo Run with --verbose flag to see detailed error messages:
    echo   build.bat --verbose
    echo.
    exit /b 1
)

REM ============================================================================
REM Step 4: Verify build outputs
REM ============================================================================
echo.
echo [4/4] Verifying build outputs...

set "BUILD_SUCCESS=1"

if exist "bin\sn.exe" (
    echo      bin\sn.exe      - OK
) else (
    echo      bin\sn.exe      - MISSING
    set "BUILD_SUCCESS=0"
)

if exist "bin\tests.exe" (
    echo      bin\tests.exe   - OK
) else (
    echo      bin\tests.exe   - MISSING
    set "BUILD_SUCCESS=0"
)

if "%BUILD_SUCCESS%"=="0" (
    echo.
    echo ERROR: Build completed but expected outputs are missing.
    echo      Please check the build log for errors.
    echo.
    exit /b 1
)

echo.
echo ============================================================
echo   Build completed successfully!
echo ============================================================
echo.
echo   Compiler:     bin\sn.exe
echo   Unit Tests:   bin\tests.exe
echo.
echo Usage:
echo   bin\sn.exe ^<source.sn^> -o ^<output.exe^>
echo   bin\tests.exe                             ^(run unit tests^)
echo.

REM ============================================================================
REM Run tests if requested
REM ============================================================================
if "%RUN_TESTS%"=="1" (
    echo Running unit tests...
    echo.
    bin\tests.exe
    if errorlevel 1 (
        echo.
        echo ERROR: Unit tests failed.
        exit /b 1
    )
    echo.
    echo Unit tests passed!
)

exit /b 0

REM ============================================================================
REM Help message
REM ============================================================================
:show_help
echo.
echo Sn Compiler - Windows Build Script (Clang/MinGW)
echo.
echo Usage: build.bat [options]
echo.
echo Options:
echo   --debug     Build with debug symbols and AddressSanitizer
echo   --clean     Clean build directory before building
echo   --test      Run unit tests after building
echo   --verbose   Show detailed build output
echo   --help, -h  Show this help message
echo.
echo Examples:
echo   build.bat              Build release version
echo   build.bat --debug      Build debug version with ASAN
echo   build.bat --clean      Clean rebuild
echo   build.bat --test       Build and run tests
echo.
echo Requirements:
echo   - LLVM/Clang (clang.exe in PATH)
echo   - CMake 3.16 or later
echo   - Ninja (recommended) or MinGW Make
echo.
exit /b 0
