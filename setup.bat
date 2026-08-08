@echo off
setlocal EnableExtensions EnableDelayedExpansion

title Stockaro - Setup & Launch

:: ============================================================
:: Stockaro - One-Shot Setup & Run Script (Windows)
:: Usage: setup.bat
:: ============================================================

cd /d "%~dp0"

:: ------------------------------------------------------------
:: Color helpers
:: ------------------------------------------------------------
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "CYAN=[96m"
set "BOLD=[1m"
set "RESET=[0m"

echo.
echo   %BOLD%Gomoku AI Engine - Setup ^& Launch%RESET%
echo.

:: ============================================================
:: STEP 1 - Check C++ compiler
:: ============================================================

echo %CYAN%[INFO]%RESET% Checking C++ compiler...

where g++ >nul 2>&1

if errorlevel 1 (
    echo %YELLOW%[WARN]%RESET% g++ not found.
    echo.
    echo Please install MinGW-w64 / MSYS2 and make sure g++ is
    echo available in your PATH.
    echo.
    echo Example:
    echo   https://www.msys2.org/
    echo.
    goto ERROR
)

for /f "tokens=*" %%A in ('g++ --version 2^>^&1 ^| findstr /i "g++"') do (
    set "GCC_VER=%%A"
    goto GCC_FOUND
)

:GCC_FOUND
echo %GREEN%[OK]%RESET% Compiler: !GCC_VER!

:: ============================================================
:: STEP 2 - Check Node.js
:: ============================================================

echo.
echo %CYAN%[INFO]%RESET% Checking Node.js...

where node >nul 2>&1

if errorlevel 1 (
    echo %YELLOW%[WARN]%RESET% Node.js not found.
    echo.
    echo Please install Node.js LTS:
    echo https://nodejs.org/
    echo.
    goto ERROR
)

for /f "tokens=*" %%A in ('node --version') do set "NODE_VER=%%A"
for /f "tokens=*" %%A in ('npm --version') do set "NPM_VER=%%A"

echo %GREEN%[OK]%RESET% Node.js !NODE_VER! ^| npm !NPM_VER!

:: ============================================================
:: STEP 3 - Install Node.js dependencies
:: ============================================================

echo.
echo %CYAN%[INFO]%RESET% Installing Node.js packages...

if not exist "package.json" (
    echo %RED%[ERROR]%RESET% package.json not found.
    goto ERROR
)

call npm install --silent

if errorlevel 1 (
    echo %RED%[ERROR]%RESET% npm install failed.
    goto ERROR
)

echo %GREEN%[OK]%RESET% npm packages installed

:: ============================================================
:: STEP 4 - Compile C++ engine
:: ============================================================

echo.
echo %CYAN%[INFO]%RESET% Compiling Stockaro C++ engine...

:: Prefer mingw32-make on Windows
where mingw32-make >nul 2>&1

if not errorlevel 1 (
    set "MAKE=mingw32-make"
    goto MAKE_FOUND
)

:: Try regular make
where make >nul 2>&1

if not errorlevel 1 (
    set "MAKE=make"
    goto MAKE_FOUND
)

echo %RED%[ERROR]%RESET% make/mingw32-make not found.
echo.
echo Install MSYS2 / MinGW-w64 and make sure the tools
echo are available in PATH.
goto ERROR

:MAKE_FOUND

echo %CYAN%[INFO]%RESET% Using !MAKE!

!MAKE! clean >nul 2>&1
!MAKE!

if errorlevel 1 (
    echo %RED%[ERROR]%RESET% C++ compilation failed.
    goto ERROR
)

if not exist "Stockaro.exe" (
    if not exist "Stockaro" (
        echo %RED%[ERROR]%RESET% Stockaro executable was not created.
        goto ERROR
    )
)

echo %GREEN%[OK]%RESET% Engine compiled

:: ============================================================
:: STEP 5 - Run benchmark
:: ============================================================

echo.
echo %CYAN%[INFO]%RESET% Running engine self-test ^(benchmark^)...
echo ----------------------------------------

if exist "Stockaro.exe" (
    Stockaro.exe --bench
) else (
    Stockaro --bench
)

if errorlevel 1 (
    echo ----------------------------------------
    echo %RED%[ERROR]%RESET% Engine self-test failed.
    goto ERROR
)

echo ----------------------------------------
echo %GREEN%[OK]%RESET% Engine self-test passed!

:: ============================================================
:: STEP 6 - Launch web server
:: ============================================================

echo.
echo %GREEN%%BOLD%Setup complete!%RESET%
echo   Opening web server at %CYAN%http://localhost:3000%RESET%
echo   Press %BOLD%Ctrl+C%RESET% to stop.
echo.

call node server.js

goto END

:: ============================================================
:: ERROR
:: ============================================================

:ERROR

echo.
echo %RED%%BOLD%Setup failed.%RESET%
echo.
pause
exit /b 1

:: ============================================================
:: END
:: ============================================================

:END

echo.
echo Server stopped.
pause
exit /b 0
