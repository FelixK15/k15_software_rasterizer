::version 1

set SCRIPT_DIRECTORY=%~dp0
if "!C_FILES!"=="" (
    echo Variable C_FILES not defined.
    goto ERROR_FAILURE
)

if "!OUTPUT_FOLDER!"=="" (
    echo Variable OUTPUT_FOLDER not defined.
    goto ERROR_FAILURE
)

if "!OUTPUT_FILE_NAME!"=="" (
    echo Variable OUTPUT_FILE_NAME not defined.
    goto ERROR_FAILURE
)

if "!SOURCE_FOLDER!"=="" (
    echo Variable SOURCE_FOLDER not defined.
    goto ERROR_FAILURE
)

if "!COMPILER_OPTIONS!"=="" (
    echo Variable COMPILER_OPTIONS not defined.
    goto ERROR_FAILURE
)

if "!LINKER_OPTIONS!"=="" (
    echo Variable LINKER_OPTIONS not defined.
    goto ERROR_FAILURE
)

set CL_EXE_PATH_SCRIPT_PATH=!SCRIPT_DIRECTORY!find_cl_exe_path.bat
set CL_EXE_REPOSITORY_PATH=https://raw.githubusercontent.com/FelixK15/k15_batch_scripts/main/find_cl_exe_path.bat

if not exist !CL_EXE_PATH_SCRIPT_PATH! (
    ::FK: Download file from github repository
    bitsadmin.exe /transfer "'cl.exe find' script download job" !CL_EXE_REPOSITORY_PATH! !CL_EXE_PATH_SCRIPT_PATH!

    if not ERRORLEVEL 0 (
        echo Error trying to download script from '!CL_EXE_REPOSITORY_PATH!'. Please check your internet connection.
        goto ERROR_FAILURE
    )
)

FOR /F "tokens=*" %%g IN ('call !CL_EXE_PATH_SCRIPT_PATH!') do (SET CL_EXE_PATH=%%g)

(for %%a in (!C_FILES!) do ( 
   set C_FILES_CONCATENATED="!SCRIPT_DIRECTORY!!SOURCE_FOLDER!%%a" !C_FILES_CONCATENATED!
))

set CL_COMMAND="!CL_EXE_PATH!" !COMPILER_OPTIONS! !C_FILES_CONCATENATED!/link !LINKER_OPTIONS!
echo !CL_COMMAND!
!CL_COMMAND!
exit /b

:ERROR_FAILURE
echo build script exited with error
exit /b 1