@echo off
setlocal

REM Compile all .c files
echo Compiling programs...
mingw32-make

REM Run each one in a new cmd window
echo Running programs...
start cmd /k ".\prog1.exe"
start cmd /k ".\prog2.exe"
start cmd /k ".\prog3.exe"
start cmd /k ".\prog4.exe"

endlocal
