@echo off

set C_FILES=k15_win32_example_entry.cpp
set OUTPUT_FILE_NAME=k15_win32_software_renderer_example
set BUILD_CONFIGURATION=%1
call build_cl.bat

exit /b 0