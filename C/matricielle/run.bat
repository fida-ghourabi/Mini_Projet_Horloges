@echo off
setlocal

REM Compile all .c files
echo Compiling programs...
mingw32-make

REM Run each one in a new cmd window
echo Running programs...
start cmd /k ".\prog1_matrix.exe"
start cmd /k ".\prog2_matrix.exe"
start cmd /k ".\prog3_matrix.exe"
start cmd /k ".\prog4_matrix.exe"

endlocal
