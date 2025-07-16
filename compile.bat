@echo off
setlocal ENABLEEXTENSIONS

:: Check for GCC
where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo [!] GCC not found. Install MinGW or TDM-GCC and make sure it's in your PATH.
    pause
    exit /b 1
)

:: Compile main.c to DemonPayload.exe
echo [*] Compiling main.c...
gcc main.c -o DemonPayload.exe -lwinmm

if exist DemonPayload.exe (
    echo [+] Build succeeded: DemonPayload.exe created.
    echo [!] Running payload... (Only do this in a VM!)
    DemonPayload.exe
) else (
    echo [X] Compilation failed. Check for errors in main.c.
)

pause
