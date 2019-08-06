@echo off
REM This script links Arduino libraries to libraries in the project, so that Arduino IDE
REM can compile the project in the first place. Unfortunately, it's not possible to reference
REM libraries in the project directly, only via symlinks.

SET ARDUINO_LIBS_DIR=%USERPROFILE%\Documents\Arduino\libraries
SET ROOT=%~dp0
echo "Linking Arduino libraries to libraries in source code"

for /d %%i in (%ROOT%libraries\*) do (
    echo   - linking "%ARDUINO_LIBS_DIR%\%%~ni" to "%%i"
    mklink /j "%ARDUINO_LIBS_DIR%\%%~ni" "%%i"
)
