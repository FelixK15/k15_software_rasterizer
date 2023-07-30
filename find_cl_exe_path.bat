@echo off
setlocal enableextensions enabledelayedexpansion

set VCVARS_FILE=vcvars64.bat
set CL_PATH_CACHE_FILE=.cl_path.txt
set CL_EXE_PATH=cl.exe

::is cl.exe part of PATH?
where /Q cl.exe
if !errorlevel! == 0 (
	echo Found cl.exe in PATH
	goto SCRIPT_END
)

if exist "!CL_PATH_CACHE_FILE!" (
	set /p CL_EXE_PATH=<!CL_PATH_CACHE_FILE!
	echo Using cl.exe from cache file: !CL_EXE_PATH!
	goto SCRIPT_END
)

echo Didn't find cl.exe in PATH - searching for Visual Studio installation...

set FOUND_PATH=0
set VS_PATH=

::check whether this is 64bit windows or not
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT

IF %OS%==64BIT (
	set REG_FOLDER=HKLM\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7
	set VS_WHERE_PATH="%PROGRAMFILES(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
)

IF %OS%==32BIT (
	set REG_FOLDER=HKLM\SOFTWARE\Microsoft\VisualStudio\SxS\VS7
	set VS_WHERE_PATH="%PROGRAMFILES%\Microsoft Visual Studio\Installer\vswhere.exe"
)

::First try to find the visual studio installation via vswhere (AFAIK this is the only way for VS2022 and upward :( )
IF exist !VS_WHERE_PATH! (
	set VS_WHERE_COMMAND=!VS_WHERE_PATH! -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
	FOR /f "delims=" %%i IN ('!VS_WHERE_COMMAND!') do set VS_PATH=%%i\

	if "!VS_PATH!"=="" (
		GOTO PATH_FOUND
	)
	set FOUND_PATH=1
) else (
	::Go to end if nothing was found
	IF %REG_FOLDER%=="" GOTO PATH_FOUND

	::try to get get visual studio path from registry for different versions
	FOR /l %%G IN (20, -1, 8) DO (
		set REG_COMMAND=reg query !REG_FOLDER! /v %%G.0
		!REG_COMMAND! >nul 2>nul

		::if errorlevel is 0, we found a valid installDir
		if !errorlevel! == 0 (
			::issue reg command again but evaluate output
			FOR /F "skip=2 tokens=*" %%A IN ('!REG_COMMAND!') DO (
				set VS_PATH=%%A
				::truncate stuff we don't want from the output
				set VS_PATH=!VS_PATH:~18!
				set FOUND_PATH=1
				goto PATH_FOUND
			)
		)
	)
)

:PATH_FOUND
::check if a path was found
IF !FOUND_PATH!==0 (
	echo Could not find valid Visual Studio installation.
	exit /b 1
) ELSE (
	echo Found Visual Studio installation at !VS_PATH!
	echo Searching and executing !VCVARS_FILE! ...
	set OLD_VCVARS_PATH="!VS_PATH!VC\!VCVARS_FILE!"

	call !OLD_VCVARS_PATH! >nul 2>nul

	if !errorlevel! neq 0 (
		set NEW_VCVARS_PATH="!VS_PATH!VC\Auxiliary\Build\!VCVARS_FILE!"
		echo !NEW_VCVARS_PATH!
		call !NEW_VCVARS_PATH! >nul 2>nul

		if !errorlevel! neq 0 (
			echo Error executing !NEW_VCVARS_PATH! or !OLD_VCVARS_PATH! - Does the file exist?
		)

		FOR /F "tokens=*" %%g IN ('where cl.exe') do (SET CL_EXE_PATH=%%g)
		echo !CL_EXE_PATH! > !CL_PATH_CACHE_FILE!
		attrib !CL_PATH_CACHE_FILE! +h
	)
) 

:SCRIPT_END
echo !CL_EXE_PATH!