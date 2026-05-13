@echo off
setlocal enableextensions enabledelayedexpansion
set CC65_HOME=I:\cc65
set PATH=I:\cc65\bin;%PATH%
cd /d "%~dp0"
if not exist build mkdir build

rem cl65 does not expand globs — enumerate sources via 'for'.
set SOURCES=
for %%f in (source\*.c) do set SOURCES=!SOURCES! %%f

rem cc65's "cc99" or "c99" standards forbid extern void[] arrays used by
rem nes.h (nes_stdjoy_joy[]) and joystick.h (joy_static_stddrv[]). The
rem default 'cc65' standard is C89 + cc65 extensions and allows them.
cl65 -t nes -O -I include -m build\hash-bench-nes.map -o hash-bench-nes.nes %SOURCES% 2>build\build.err
if not exist hash-bench-nes.nes (
    echo Build FAILED. See build\build.err
    type build\build.err
    exit /b 1
)
echo Build OK: %~dp0hash-bench-nes.nes
type build\build.err 2>nul
endlocal
