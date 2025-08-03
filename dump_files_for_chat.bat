@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: dump_files_for_chat.bat file1 file2 ...
    exit /b 1
)

for %%F in (%*) do (
    echo ^<%%F^>
    echo ```
    type "%%F"
    echo ```
    echo ^</%%F^>
    echo.
)

endlocal
