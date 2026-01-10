#Requires -Version 5.1
# Run integration tests on Windows with MSVC
# Usage: .\run_integration_test.ps1 [-TestType <type>] [-TestFile <file>] [-Compiler <path>]
# Test types: integration, integration-errors, explore, explore-errors
#
# Exit codes:
#   0 - All tests passed
#   1 - One or more tests failed or error occurred

param(
    [ValidateSet("integration", "integration-errors", "explore", "explore-errors")]
    [string]$TestType = "integration",
    [string]$TestFile = "",
    [string]$Compiler = "",
    [int]$CompileTimeout = 10,
    [int]$RunTimeout = 30,
    [switch]$All,
    [switch]$Help
)

# Show help
if ($Help) {
    Write-Host "Run integration tests for Sn compiler on Windows"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\run_integration_test.ps1 -All                       Run all integration tests"
    Write-Host "  .\run_integration_test.ps1 -TestType integration -All Run all integration tests"
    Write-Host "  .\run_integration_test.ps1 -TestType integration-errors -All"
    Write-Host "                                                         Run all error tests"
    Write-Host "  .\run_integration_test.ps1 -TestFile <path>            Run single test"
    Write-Host ""
    Write-Host "Parameters:"
    Write-Host "  -TestType        Type: integration, integration-errors, explore, explore-errors"
    Write-Host "  -TestFile        Path to a single .sn file to test"
    Write-Host "  -Compiler        Path to compiler (default: bin\sn.exe)"
    Write-Host "  -CompileTimeout  Compile timeout in seconds (default: 10)"
    Write-Host "  -RunTimeout      Run timeout in seconds (default: 30)"
    Write-Host "  -All             Run all tests of the specified type"
    exit 0
}

# Detect available C compiler and configure environment accordingly
# Priority: clang-cl (MSVC-compatible) > clang (MinGW) > gcc

$UseVsDevShell = $false
$ClangClFound = $false

# Check for clang-cl in standard LLVM install path
$LlvmPath = "C:\Program Files\LLVM\bin"
if (Test-Path (Join-Path $LlvmPath "clang-cl.exe")) {
    $ClangClFound = $true
    $env:PATH = "$LlvmPath;$env:PATH"
}

# If clang-cl found, try to set up VS Developer environment for MSVC libs
if ($ClangClFound) {
    $VsInstallPaths = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Community"
    )
    foreach ($VsPath in $VsInstallPaths) {
        $VsDevShellModule = Join-Path $VsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
        if (Test-Path $VsDevShellModule) {
            try {
                Import-Module $VsDevShellModule -ErrorAction SilentlyContinue
                Enter-VsDevShell -VsInstallPath $VsPath -DevCmdArguments '-arch=x64' -SkipAutomaticLocation 2>$null
                $UseVsDevShell = $true
                # Re-add LLVM to PATH after VS Dev Shell (it may have reset PATH)
                $env:PATH = "$LlvmPath;$env:PATH"
                break
            } catch {
                # VS Dev Shell not available, continue anyway
            }
        }
    }
}

# If clang-cl not found, check for MinGW clang or gcc and set SN_CC
if (-not $ClangClFound) {
    $ClangPath = (Get-Command clang -ErrorAction SilentlyContinue)
    $GccPath = (Get-Command gcc -ErrorAction SilentlyContinue)

    if ($ClangPath) {
        $env:SN_CC = "clang"
    } elseif ($GccPath) {
        $env:SN_CC = "gcc"
    }
    # If neither found, let the compiler produce its own error
}

# Paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
Set-Location $ProjectDir

# Determine compiler path
if (-not $Compiler) {
    $Compiler = Join-Path $ProjectDir "bin\sn.exe"
}

if (-not (Test-Path $Compiler)) {
    Write-Host "Error: Compiler not found at $Compiler" -ForegroundColor Red
    exit 1
}

# Resolve to absolute path for Process.Start()
$Compiler = (Resolve-Path $Compiler).Path

# Test configuration based on type
$TestConfig = @{
    "integration" = @{
        Dir = "tests\integration"
        Pattern = "*.sn"
        ExpectCompileFail = $false
        Title = "Integration Tests"
        DefaultRunTimeout = 5
    }
    "integration-errors" = @{
        Dir = "tests\integration\errors"
        Pattern = "*.sn"
        ExpectCompileFail = $true
        Title = "Integration Error Tests (expected compile failures)"
        DefaultRunTimeout = 5
    }
    "explore" = @{
        Dir = "tests\exploratory"
        Pattern = "test_*.sn"
        ExpectCompileFail = $false
        Title = "Exploratory Tests"
        DefaultRunTimeout = 30
    }
    "explore-errors" = @{
        Dir = "tests\exploratory\errors"
        Pattern = "*.sn"
        ExpectCompileFail = $true
        Title = "Exploratory Error Tests (expected compile failures)"
        DefaultRunTimeout = 5
    }
}

$Config = $TestConfig[$TestType]
if ($TestType -eq "integration") {
    $RunTimeout = 5
}

# Create temp directory for test artifacts
$TempDir = Join-Path $env:TEMP "sn_test_runner_$PID"
New-Item -ItemType Directory -Path $TempDir -Force | Out-Null

# Cleanup function
function Cleanup {
    if (Test-Path $TempDir) {
        Remove-Item -Path $TempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# Register cleanup on script exit (using Register-EngineEvent for reliability)
try {
    Register-EngineEvent -SourceIdentifier PowerShell.Exiting -Action { Cleanup } -ErrorAction SilentlyContinue | Out-Null
} catch {
    # Event registration not critical, continue anyway
}
trap { Cleanup; break }

# Counters
$Passed = 0
$Failed = 0
$Skipped = 0

# Function to run a command with timeout
function Invoke-WithTimeout {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutSeconds,
        [string]$OutputFile,
        [string]$ErrorFile,
        [switch]$CaptureExitCode
    )

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $FilePath
    $pinfo.Arguments = $ArgumentList -join " "
    $pinfo.RedirectStandardOutput = $true
    $pinfo.RedirectStandardError = $true
    $pinfo.UseShellExecute = $false
    $pinfo.CreateNoWindow = $true
    $pinfo.WorkingDirectory = $ProjectDir
    $pinfo.StandardOutputEncoding = [System.Text.Encoding]::UTF8
    $pinfo.StandardErrorEncoding = [System.Text.Encoding]::UTF8

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $pinfo

    try {
        $process.Start() | Out-Null
    } catch {
        return @{
            TimedOut = $false
            ExitCode = -1
            Output = ""
            Error = $_.Exception.Message
        }
    }

    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()

    $completed = $process.WaitForExit($TimeoutSeconds * 1000)

    if (-not $completed) {
        try {
            $process.Kill()
            $process.WaitForExit(1000)
        } catch {
            # Process may have already exited
        }
        return @{
            TimedOut = $true
            ExitCode = -1
            Output = ""
            Error = "Process timed out after $TimeoutSeconds seconds"
        }
    }

    # Wait for async reads to complete
    [void]$stdout.Wait(1000)
    [void]$stderr.Wait(1000)

    $outputStr = if ($stdout.IsCompleted) { $stdout.Result } else { "" }
    $errorStr = if ($stderr.IsCompleted) { $stderr.Result } else { "" }

    if ($OutputFile) {
        $outputStr | Out-File -FilePath $OutputFile -Encoding UTF8 -NoNewline
    }
    if ($ErrorFile) {
        $errorStr | Out-File -FilePath $ErrorFile -Encoding UTF8 -NoNewline
    }

    return @{
        TimedOut = $false
        ExitCode = $process.ExitCode
        Output = $outputStr
        Error = $errorStr
    }
}

# Function to normalize line endings for comparison
function Normalize-LineEndings {
    param([string]$Text)
    # Replace CRLF with LF, then remove trailing whitespace from each line
    $Text = $Text -replace "`r`n", "`n"
    $Text = $Text -replace "`r", "`n"
    return $Text.Trim()
}

# Function to run a single test
function Run-Test {
    param([string]$TestPath, [bool]$ExpectCompileFail)

    $TestName = [System.IO.Path]::GetFileNameWithoutExtension($TestPath)
    $ExpectedPath = $TestPath -replace '\.sn$', '.expected'
    $PanicPath = $TestPath -replace '\.sn$', '.panic'
    $ExePath = Join-Path $TempDir "$TestName.exe"
    $OutputFile = Join-Path $TempDir "$TestName.out"
    $CompileErrFile = Join-Path $TempDir "$TestName.compile_err"

    # Pad test name for alignment
    $PaddedName = $TestName.PadRight(45)
    Write-Host -NoNewline "  $PaddedName "

    if ($ExpectCompileFail) {
        # Error test: should fail to compile
        if (-not (Test-Path $ExpectedPath)) {
            Write-Host "SKIP (no .expected)" -ForegroundColor Yellow
            return "skip"
        }

        # Try to compile (should fail)
        $result = Invoke-WithTimeout -FilePath $Compiler -ArgumentList @($TestPath, "-o", $ExePath, "-l", "1") -TimeoutSeconds $CompileTimeout -ErrorFile $CompileErrFile

        if ($result.TimedOut) {
            Write-Host "FAIL (compile timeout)" -ForegroundColor Red
            return "fail"
        }

        if ($result.ExitCode -eq 0) {
            Write-Host "FAIL (should not compile)" -ForegroundColor Red
            return "fail"
        }

        # Check if the error message matches expected
        $ExpectedError = (Get-Content $ExpectedPath -First 1 -Encoding UTF8 -ErrorAction SilentlyContinue)
        $CompileError = $result.Error

        # Use -like with wildcards instead of regex for safer pattern matching
        if ($CompileError -like "*$ExpectedError*") {
            Write-Host "PASS" -ForegroundColor Green
            return "pass"
        } else {
            Write-Host "FAIL (wrong error)" -ForegroundColor Red
            # Extract just the error message
            $ShortError = if ($CompileError.Length -gt 80) { $CompileError.Substring(0, 80) + "..." } else { $CompileError }
            Write-Host "    Expected: $ExpectedError"
            Write-Host "    Got:      $ShortError"
            return "fail"
        }
    } else {
        # Positive test: should compile and run
        $HasExpected = Test-Path $ExpectedPath
        $HasPanic = Test-Path $PanicPath

        if (-not $HasExpected -and $TestType -ne "explore") {
            Write-Host "SKIP (no .expected)" -ForegroundColor Yellow
            return "skip"
        }

        # Compile
        # Note: Don't use -g (enables ASAN) as it requires specific runtime libraries
        # that may not be available in all environments (e.g., GitHub Actions)
        $result = Invoke-WithTimeout -FilePath $Compiler -ArgumentList @($TestPath, "-o", $ExePath, "-l", "1", "-O0") -TimeoutSeconds $CompileTimeout -ErrorFile $CompileErrFile

        if ($result.TimedOut) {
            Write-Host "FAIL (compile timeout)" -ForegroundColor Red
            return "fail"
        }

        if ($result.ExitCode -ne 0) {
            Write-Host "FAIL (compile error)" -ForegroundColor Red
            # Show more error lines for debugging
            $ErrorLines = ($result.Error -split "`n" | Select-Object -First 10) -join "`n    "
            if ($ErrorLines) {
                Write-Host "    $ErrorLines"
            }
            # Also show stdout in case error is there
            $OutLines = ($result.Output -split "`n" | Select-Object -First 5) -join "`n    "
            if ($OutLines) {
                Write-Host "    [stdout]: $OutLines"
            }
            return "fail"
        }

        if (-not (Test-Path $ExePath)) {
            Write-Host "FAIL (no executable)" -ForegroundColor Red
            return "fail"
        }

        # Run with timeout
        $result = Invoke-WithTimeout -FilePath $ExePath -ArgumentList @() -TimeoutSeconds $RunTimeout -OutputFile $OutputFile

        if ($result.TimedOut) {
            Write-Host "FAIL (runtime timeout)" -ForegroundColor Red
            return "fail"
        }

        # Check for expected panic
        if ($HasPanic) {
            if ($result.ExitCode -eq 0) {
                Write-Host "FAIL (expected panic)" -ForegroundColor Red
                return "fail"
            }
        } else {
            if ($result.ExitCode -ne 0) {
                Write-Host "FAIL (exit code: $($result.ExitCode))" -ForegroundColor Red
                $ErrorLines = ($result.Output -split "`n" | Select-Object -First 3) -join "`n    "
                if ($ErrorLines) {
                    Write-Host "    $ErrorLines"
                }
                return "fail"
            }
        }

        # Compare output if expected file exists
        if ($HasExpected) {
            $Expected = Get-Content $ExpectedPath -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
            # Combine stderr and stdout for comparison (stderr first to match expected output format)
            $Actual = $result.Error + $result.Output

            # Handle null or empty values
            if ($null -eq $Expected) { $Expected = "" }
            if ($null -eq $Actual) { $Actual = "" }

            # Normalize line endings for comparison
            $NormalizedExpected = Normalize-LineEndings $Expected
            $NormalizedActual = Normalize-LineEndings $Actual

            if ($NormalizedActual -eq $NormalizedExpected) {
                Write-Host "PASS" -ForegroundColor Green
                return "pass"
            } else {
                Write-Host "FAIL (output mismatch)" -ForegroundColor Red
                $ExpectedFirst = ($Expected -split "`n" | Select-Object -First 1)
                $ActualFirst = ($Actual -split "`n" | Select-Object -First 1)
                Write-Host "    Expected: $ExpectedFirst"
                Write-Host "    Got:      $ActualFirst"
                return "fail"
            }
        } else {
            # No expected file but ran successfully (exploratory test)
            Write-Host "PASS" -ForegroundColor Green
            return "pass"
        }
    }
}

# Print header
Write-Host ""
Write-Host $Config.Title -ForegroundColor Cyan
Write-Host "============================================================"

# Run tests
if ($TestFile) {
    # Single test mode
    if (-not (Test-Path $TestFile)) {
        Write-Host "Error: Test file not found: $TestFile" -ForegroundColor Red
        Cleanup
        exit 1
    }
    $Result = Run-Test $TestFile $Config.ExpectCompileFail
    if ($Result -eq "pass") { $Passed++ }
    elseif ($Result -eq "fail") { $Failed++ }
    else { $Skipped++ }
} elseif ($All) {
    # Run all tests of specified type
    $TestDir = Join-Path $ProjectDir $Config.Dir
    if (-not (Test-Path $TestDir)) {
        Write-Host "Warning: Test directory not found: $TestDir" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "------------------------------------------------------------"
        Write-Host "Results: 0 passed, 0 failed, 0 skipped"
        Cleanup
        exit 0
    }

    $Tests = Get-ChildItem (Join-Path $TestDir $Config.Pattern) -ErrorAction SilentlyContinue
    if (-not $Tests -or $Tests.Count -eq 0) {
        Write-Host "No tests found in $TestDir matching $($Config.Pattern)" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "------------------------------------------------------------"
        Write-Host "Results: 0 passed, 0 failed, 0 skipped"
        Cleanup
        exit 0
    }

    foreach ($Test in $Tests) {
        $Result = Run-Test $Test.FullName $Config.ExpectCompileFail
        if ($Result -eq "pass") { $Passed++ }
        elseif ($Result -eq "fail") { $Failed++ }
        else { $Skipped++ }
    }
} else {
    Write-Host "No tests specified. Use -All to run all tests or -TestFile to run a single test." -ForegroundColor Yellow
    Write-Host "Use -Help for more information."
    Cleanup
    exit 0
}

# Print summary
Write-Host ""
Write-Host "------------------------------------------------------------"
Write-Host -NoNewline "Results: "
Write-Host -NoNewline "$Passed passed" -ForegroundColor Green
Write-Host -NoNewline ", "
Write-Host -NoNewline "$Failed failed" -ForegroundColor Red
Write-Host -NoNewline ", "
Write-Host "$Skipped skipped" -ForegroundColor Yellow

# Cleanup
Cleanup

# Exit with error if any tests failed
if ($Failed -gt 0) {
    exit 1
}
exit 0
