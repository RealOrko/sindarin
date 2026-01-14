@echo off
setlocal EnableDelayedExpansion

REM ============================================================================
REM Sn Compiler - Windows Setup Script
REM Installs all dependencies required to build and test the Sn compiler
REM ============================================================================

echo.
echo ============================================================
echo   Sn Compiler - Windows Setup
echo ============================================================
echo.

set "SCRIPT_DIR=%~dp0"
set "VCPKG_DIR=%SCRIPT_DIR%vcpkg"
set "VCPKG_INSTALLED=%VCPKG_DIR%\installed\x64-windows"

REM ============================================================================
REM Step 1: Check for winget
REM ============================================================================
echo [1/6] Checking for winget...

where winget >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: winget is not installed.
    echo        winget comes with Windows 10/11. Please ensure you have:
    echo        - Windows 10 version 1809 or later
    echo        - App Installer from the Microsoft Store
    echo.
    exit /b 1
)
echo      winget found.

REM ============================================================================
REM Step 2: Install CMake if not present
REM ============================================================================
echo.
echo [2/6] Checking for CMake...

where cmake >nul 2>&1
if errorlevel 1 (
    echo      CMake not found. Installing via winget...
    winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
    if errorlevel 1 (
        echo ERROR: Failed to install CMake.
        exit /b 1
    )
    echo      CMake installed. You may need to restart your terminal.
) else (
    for /f "tokens=3" %%v in ('cmake --version ^| findstr /r "^cmake"') do (
        echo      CMake %%v found.
    )
)

REM ============================================================================
REM Step 3: Install LLVM-MinGW (Clang) if not present
REM ============================================================================
echo.
echo [3/6] Checking for Clang...

where clang >nul 2>&1
if errorlevel 1 (
    echo      Clang not found. Installing LLVM-MinGW via winget...
    winget install --id MartinStorsjo.LLVM-MinGW.UCRT --silent --accept-package-agreements --accept-source-agreements
    if errorlevel 1 (
        echo ERROR: Failed to install LLVM-MinGW.
        exit /b 1
    )
    echo      LLVM-MinGW installed. You may need to restart your terminal.
) else (
    for /f "tokens=3" %%v in ('clang --version ^| findstr /r "^clang"') do (
        echo      Clang %%v found.
    )
)

REM ============================================================================
REM Step 4: Clone and bootstrap vcpkg
REM ============================================================================
echo.
echo [4/6] Setting up vcpkg...

if exist "%VCPKG_DIR%\vcpkg.exe" (
    echo      vcpkg already bootstrapped.
) else (
    if exist "%VCPKG_DIR%" (
        echo      vcpkg directory exists but not bootstrapped. Bootstrapping...
    ) else (
        echo      Cloning vcpkg...
        git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
        if errorlevel 1 (
            echo ERROR: Failed to clone vcpkg.
            exit /b 1
        )
    )

    echo      Bootstrapping vcpkg...
    pushd "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat -disableMetrics
    popd

    if not exist "%VCPKG_DIR%\vcpkg.exe" (
        echo ERROR: vcpkg bootstrap failed.
        exit /b 1
    )
    echo      vcpkg bootstrapped successfully.
)

REM ============================================================================
REM Step 5: Install zlib via vcpkg
REM ============================================================================
echo.
echo [5/6] Installing zlib...

if exist "%VCPKG_INSTALLED%\lib\zlib.lib" (
    echo      zlib already installed.
) else (
    echo      Installing zlib via vcpkg...
    "%VCPKG_DIR%\vcpkg.exe" install zlib:x64-windows
    if errorlevel 1 (
        echo ERROR: Failed to install zlib.
        exit /b 1
    )
    echo      zlib installed successfully.
)

REM Create libz.a symlink for Unix-style -lz linking
if not exist "%VCPKG_INSTALLED%\lib\libz.a" (
    echo      Creating libz.a compatibility link...
    copy "%VCPKG_INSTALLED%\lib\zlib.lib" "%VCPKG_INSTALLED%\lib\libz.a" >nul
)

REM ============================================================================
REM Step 6: Configure compiler settings
REM ============================================================================
echo.
echo [6/6] Configuring compiler settings...

REM Create bin directory if it doesn't exist
if not exist "bin" mkdir bin

REM Copy zlib DLL to bin directory (needed at runtime)
if exist "%VCPKG_INSTALLED%\bin\zlib1.dll" (
    echo      Copying zlib1.dll to bin...
    copy "%VCPKG_INSTALLED%\bin\zlib1.dll" "bin\zlib1.dll" >nul
)

REM Generate sn-clang.cfg with vcpkg paths
echo # Sindarin Clang Backend Configuration (auto-generated by setup.bat)> bin\sn-clang.cfg
echo # Priority: Environment variable ^> Config file ^> Default>> bin\sn-clang.cfg
echo.>> bin\sn-clang.cfg
echo SN_CC=clang>> bin\sn-clang.cfg
echo SN_STD=c99>> bin\sn-clang.cfg
echo SN_RELEASE_CFLAGS=-O3 -flto>> bin\sn-clang.cfg
echo SN_DEBUG_CFLAGS=-fsanitize=address -fno-omit-frame-pointer -g>> bin\sn-clang.cfg
echo SN_CFLAGS=-I%VCPKG_INSTALLED%/include>> bin\sn-clang.cfg
echo SN_LDFLAGS=-L%VCPKG_INSTALLED%/lib>> bin\sn-clang.cfg
echo SN_LDLIBS=>> bin\sn-clang.cfg

echo      Created bin\sn-clang.cfg with vcpkg paths.

REM Copy other config files if they don't have vcpkg-specific settings
if exist "etc\sn-gcc.cfg" (
    if not exist "bin\sn-gcc.cfg" copy "etc\sn-gcc.cfg" "bin\sn-gcc.cfg" >nul
)
if exist "etc\sn-msvc.cfg" (
    if not exist "bin\sn-msvc.cfg" copy "etc\sn-msvc.cfg" "bin\sn-msvc.cfg" >nul
)
if exist "etc\sn-tcc.cfg" (
    if not exist "bin\sn-tcc.cfg" copy "etc\sn-tcc.cfg" "bin\sn-tcc.cfg" >nul
)

echo.
echo ============================================================
echo   Setup completed successfully!
echo ============================================================
echo.
echo   Next steps:
echo     1. If this is a fresh install, restart your terminal
echo        to pick up new PATH entries.
echo     2. Run: build.bat
echo     3. Run: test.bat
echo.
echo   vcpkg location: %VCPKG_DIR%
echo   zlib headers:   %VCPKG_INSTALLED%\include
echo   zlib library:   %VCPKG_INSTALLED%\lib
echo.

exit /b 0
