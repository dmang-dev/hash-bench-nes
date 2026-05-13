@echo off
setlocal enableextensions
set CC65_HOME=I:\cc65
set PATH=I:\cc65\bin;%PATH%
cd /d "%~dp0"
if not exist build mkdir build
rem Link ONLY smoketest.c — none of the bench algos / harness.
rem smoketest.c lives in tests/ (NOT source/) so the main build.bat
rem doesn't pick it up via its source\*.c glob and end up with two main()s.
cl65 -t nes -O -m build\smoketest.map -o smoketest.nes tests\smoketest.c 2>build\smoketest.err
if not exist smoketest.nes (
    echo Smoketest build FAILED. See build\smoketest.err
    type build\smoketest.err
    exit /b 1
)
echo Smoketest OK: %~dp0smoketest.nes
type build\smoketest.err 2>nul
endlocal
