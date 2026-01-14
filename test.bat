@echo off
setlocal EnableDelayedExpansion

REM ============================================================================
REM Sn Compiler - Windows Test Script
REM Runs integration tests on Windows
REM ============================================================================

set "SN=bin\sn.exe"
set "TEST_TYPE=all"
set "RUN_ERRORS=0"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :done_args
if /i "%~1"=="--unit" (
    set "TEST_TYPE=unit"
    shift
    goto :parse_args
)
if /i "%~1"=="--integration" (
    set "TEST_TYPE=integration"
    shift
    goto :parse_args
)
if /i "%~1"=="--integration-errors" (
    set "TEST_TYPE=integration-errors"
    shift
    goto :parse_args
)
if /i "%~1"=="--explore" (
    set "TEST_TYPE=explore"
    shift
    goto :parse_args
)
if /i "%~1"=="--explore-errors" (
    set "TEST_TYPE=explore-errors"
    shift
    goto :parse_args
)
if /i "%~1"=="--sdk" (
    set "TEST_TYPE=sdk"
    shift
    goto :parse_args
)
if /i "%~1"=="--errors" (
    set "RUN_ERRORS=1"
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
echo   Sn Compiler - Windows Test Suite
echo ============================================================
echo.

REM Check if compiler exists
if not exist "%SN%" (
    echo ERROR: Compiler not found at %SN%
    echo        Please run build.bat first.
    exit /b 1
)

REM Add bin directory to PATH for DLL resolution (e.g., zlib1.dll)
set "PATH=%~dp0bin;%PATH%"

REM Track overall exit code
set "EXIT_CODE=0"

REM ============================================================================
REM Unit Tests
REM ============================================================================
if "%TEST_TYPE%"=="all" goto :run_unit
if "%TEST_TYPE%"=="unit" goto :run_unit
goto :skip_unit

:run_unit
echo [Unit Tests]
echo ============================================================

if not exist "bin\tests.exe" (
    echo ERROR: Test binary not found at bin\tests.exe
    echo        Please run build.bat first.
    exit /b 1
)

bin\tests.exe
if errorlevel 1 (
    echo.
    echo Unit tests FAILED
    set "EXIT_CODE=1"
) else (
    echo.
    echo Unit tests PASSED
)
echo.

:skip_unit

REM ============================================================================
REM Integration Tests (using PowerShell runner for proper timeout handling)
REM ============================================================================
if "%TEST_TYPE%"=="all" goto :run_integration
if "%TEST_TYPE%"=="integration" goto :run_integration
goto :skip_integration

:run_integration
REM Use PowerShell test runner for better timeout and line ending handling
powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType integration -All -Compiler "%SN%"
if errorlevel 1 set "EXIT_CODE=1"

if "%RUN_ERRORS%"=="1" (
    powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType integration-errors -All -Compiler "%SN%"
    if errorlevel 1 set "EXIT_CODE=1"
)

if "%TEST_TYPE%"=="all" (
    powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType integration-errors -All -Compiler "%SN%"
    if errorlevel 1 set "EXIT_CODE=1"
)

:skip_integration

REM ============================================================================
REM Integration Error Tests Only
REM ============================================================================
if "%TEST_TYPE%"=="integration-errors" (
    powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType integration-errors -All -Compiler "%SN%"
    if errorlevel 1 set "EXIT_CODE=1"
)

REM ============================================================================
REM Exploratory Tests
REM ============================================================================
if "%TEST_TYPE%"=="all" goto :run_explore
if "%TEST_TYPE%"=="explore" goto :run_explore
goto :skip_explore

:run_explore
powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType explore -All -Compiler "%SN%"
if errorlevel 1 set "EXIT_CODE=1"

if "%RUN_ERRORS%"=="1" (
    powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType explore-errors -All -Compiler "%SN%"
    if errorlevel 1 set "EXIT_CODE=1"
)

if "%TEST_TYPE%"=="all" (
    powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType explore-errors -All -Compiler "%SN%"
    if errorlevel 1 set "EXIT_CODE=1"
)

:skip_explore

REM ============================================================================
REM Exploratory Error Tests Only
REM ============================================================================
if "%TEST_TYPE%"=="explore-errors" (
    powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType explore-errors -All -Compiler "%SN%"
    if errorlevel 1 set "EXIT_CODE=1"
)

REM ============================================================================
REM SDK Tests
REM ============================================================================
if "%TEST_TYPE%"=="all" goto :run_sdk
if "%TEST_TYPE%"=="sdk" goto :run_sdk
goto :skip_sdk

:run_sdk
powershell.exe -ExecutionPolicy Bypass -File scripts\run_integration_test.ps1 -TestType sdk -All -Compiler "%SN%"
if errorlevel 1 set "EXIT_CODE=1"

:skip_sdk

REM ============================================================================
REM Summary
REM ============================================================================
echo.
echo ============================================================
if %EXIT_CODE% EQU 0 (
    echo   All tests PASSED!
) else (
    echo   Some tests FAILED.
)
echo ============================================================
echo.

exit /b %EXIT_CODE%

REM ============================================================================
REM Help message
REM ============================================================================
:show_help
echo.
echo Sn Compiler - Windows Test Script
echo.
echo Usage: test.bat [options]
echo.
echo Options:
echo   --unit               Run only unit tests
echo   --integration        Run only integration tests
echo   --integration-errors Run only integration error tests
echo   --explore            Run only exploratory tests
echo   --explore-errors     Run only exploratory error tests
echo   --sdk                Run only SDK tests
echo   --errors             Also run error tests with integration/explore tests
echo   --help, -h           Show this help message
echo.
echo Examples:
echo   test.bat                      Run all tests (unit + integration + explore + sdk + errors)
echo   test.bat --unit               Run unit tests only
echo   test.bat --integration        Run integration tests (positive only)
echo   test.bat --integration --errors  Run integration tests including error tests
echo   test.bat --sdk                Run SDK tests only
echo.
echo Note: This script uses PowerShell for integration tests to provide
echo       proper timeout handling and Windows line ending normalization.
echo.
exit /b 0
