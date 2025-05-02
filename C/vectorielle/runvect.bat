@echo off
setlocal

REM Compile all .c files
echo Compiling programs...
mingw32-make

REM Run each one in a new cmd window
echo Running programs...
start cmd /k ".\prog1_vector.exe"
start cmd /k ".\prog2_vector.exe"
start cmd /k ".\prog3_vector.exe"
start cmd /k ".\prog4_vector.exe"

endlocal
