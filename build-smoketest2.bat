@echo off
setlocal enableextensions
set PATH=I:\cc65\bin;%PATH%
cd /d "%~dp0"
if not exist build mkdir build
cl65 -t nes -O -m build\smoketest2.map -o smoketest2.nes tests\smoketest2.c 2>build\smoketest2.err
if not exist smoketest2.nes (
    echo Smoketest2 build FAILED. See build\smoketest2.err
    type build\smoketest2.err
    exit /b 1
)
echo Smoketest2 OK: %~dp0smoketest2.nes
type build\smoketest2.err 2>nul
endlocal
