@echo off

set C_FILES=../tests/k15_math_tests.cpp
set OUTPUT_FILE_NAME=k15_software_rasterizer_tests
set LINKER_OPTIONS=/SUBSYSTEM:CONSOLE
call build_cl.bat

exit /b 0