@echo off
set "file=%~1"
set "tempfile=%~1.tmp"
setlocal enabledelayedexpansion
set FOUND=
for /f "usebackq delims=" %%a in ("%file%") do (
    if not defined FOUND (
        echo %%a | findstr /c:"#include \"atn/PredicateEvalInfo.h\"" >nul
        if !errorlevel! equ 0 (
            echo #include ^<chrono^>
            set FOUND=1
        )
    )
    echo %%a
) > "%tempfile%"
move /y "%tempfile%" "%file%" >nul
