@echo off
setlocal EnableDelayedExpansion

REM ============================================================================
REM Sn Compiler - Windows Build Script
REM Uses CMake + Visual Studio (MSVC) toolchain
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
echo   Sn Compiler - Windows Build
echo ============================================================
echo.

REM ============================================================================
REM Step 1: Detect Visual Studio environment
REM ============================================================================
echo [1/4] Detecting Visual Studio environment...

REM Check if we're already in a VS Developer Command Prompt
if defined VSINSTALLDIR (
    echo      Found: VS environment already configured
    echo      VSINSTALLDIR: %VSINSTALLDIR%
    goto :vs_found
)

REM Try to find and setup VS environment using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not exist "%VSWHERE%" (
    echo.
    echo ERROR: Could not find Visual Studio installation.
    echo.
    echo Please ensure you have Visual Studio 2019 or later installed with
    echo the "Desktop development with C++" workload.
    echo.
    echo Alternatively, run this script from a "Developer Command Prompt"
    echo or "Developer PowerShell" for Visual Studio.
    echo.
    exit /b 1
)

REM Find VS installation path
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo.
    echo ERROR: Visual Studio found but C++ tools are not installed.
    echo.
    echo Please install the "Desktop development with C++" workload
    echo using the Visual Studio Installer.
    echo.
    exit /b 1
)

echo      Found: %VS_PATH%

REM Setup VS environment
set "VCVARSALL=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCVARSALL%" (
    echo.
    echo ERROR: vcvarsall.bat not found at expected location.
    echo      Expected: %VCVARSALL%
    echo.
    echo Please verify your Visual Studio installation is complete.
    echo.
    exit /b 1
)

echo      Configuring x64 native tools environment...
call "%VCVARSALL%" x64 >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: Failed to configure Visual Studio environment.
    echo      Please try running this script from a Developer Command Prompt.
    echo.
    exit /b 1
)

:vs_found
echo      Compiler: cl.exe

REM Verify cl.exe is available
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: cl.exe not found in PATH after VS environment setup.
    echo      Please run this script from a Developer Command Prompt.
    echo.
    exit /b 1
)

REM ============================================================================
REM Step 2: Check for CMake
REM ============================================================================
echo.
echo [2/4] Checking for CMake...

where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: CMake not found in PATH.
    echo.
    echo Please install CMake from https://cmake.org/download/
    echo and ensure it is added to your PATH.
    echo.
    echo You can also install CMake via:
    echo   - Visual Studio Installer ^(Individual Components ^> CMake tools^)
    echo   - winget: winget install Kitware.CMake
    echo   - chocolatey: choco install cmake
    echo.
    exit /b 1
)

for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /i "cmake version"') do (
    echo      Found: CMake %%v
)

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
set "CMAKE_ARGS=-G "NMake Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

if "%BUILD_TYPE%"=="Debug" (
    set "CMAKE_ARGS=%CMAKE_ARGS% -DSN_DEBUG=ON"
)

if "%VERBOSE%"=="1" (
    cmake -S . -B "%BUILD_DIR%" %CMAKE_ARGS%
) else (
    cmake -S . -B "%BUILD_DIR%" %CMAKE_ARGS% >nul 2>&1
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

if exist "bin\sn_runtime.lib" (
    echo      bin\lib\msvc\sn_runtime.lib - OK
) else if exist "bin\lib\msvc\sn_runtime.lib" (
    echo      bin\lib\msvc\sn_runtime.lib - OK
) else (
    echo      bin\lib\msvc\sn_runtime.lib - MISSING ^(optional^)
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
echo Sn Compiler - Windows Build Script
echo.
echo Usage: build.bat [options]
echo.
echo Options:
echo   --debug     Build with debug symbols and runtime checks
echo   --clean     Clean build directory before building
echo   --test      Run unit tests after building
echo   --verbose   Show detailed build output
echo   --help, -h  Show this help message
echo.
echo Examples:
echo   build.bat              Build release version
echo   build.bat --debug      Build debug version
echo   build.bat --clean      Clean rebuild
echo   build.bat --test       Build and run tests
echo.
echo Requirements:
echo   - Visual Studio 2019 or later with C++ tools
echo   - CMake 3.16 or later
echo.
echo Note: This script will automatically detect and configure the
echo       Visual Studio environment. You can also run it from a
echo       Developer Command Prompt if auto-detection fails.
echo.
exit /b 0
