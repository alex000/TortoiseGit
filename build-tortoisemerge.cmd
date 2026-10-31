@echo off
rem Command line build of TortoiseGitMerge (and only the projects it depends on).
rem
rem Requirements: Visual Studio 2022 (or Build Tools 2022) with
rem   "Desktop development with C++", "C++ ATL" and "C++ MFC" components.
rem   Optional for -clang: "C++ Clang tools for Windows" (clang-cl).
rem
rem Usage: build-tortoisemerge.cmd [debug] [x86^|arm64] [clang] [rebuild] [init]
rem   debug    build the Debug configuration (default: Release)
rem   x86      build 32 bit (default: x64), arm64 for ARM64
rem   clang    use the ClangCL toolset (clang-cl + MSVC STL/MFC) instead of MSVC
rem   rebuild  clean before building
rem   init     fetch the git submodules TortoiseGitMerge needs first
rem
rem Output: bin\Release64\bin\TortoiseGitMerge.exe (plus ScintillaLexer_tgit.dll,
rem         which is needed for syntax highlighting)

setlocal EnableDelayedExpansion
set CONFIG=Release
set PLATFORM=x64
set TOOLSET=
set TARGET=TortoiseGitMerge
set INIT=0

:parse
if "%~1"=="" goto parsed
if /i "%~1"=="debug" set CONFIG=Debug
if /i "%~1"=="release" set CONFIG=Release
if /i "%~1"=="x86" set PLATFORM=Win32
if /i "%~1"=="win32" set PLATFORM=Win32
if /i "%~1"=="x64" set PLATFORM=x64
if /i "%~1"=="arm64" set PLATFORM=ARM64
if /i "%~1"=="clang" set TOOLSET=/p:PlatformToolset=ClangCL
if /i "%~1"=="rebuild" set TARGET=TortoiseGitMerge:Rebuild
if /i "%~1"=="init" set INIT=1
shift
goto parse
:parsed

pushd "%~dp0"

if "%INIT%"=="1" (
	echo Fetching submodules...
	git submodule update --init ext/apr ext/apr-util ext/zlib ext/libgit2 ext/editorconfig ext/lexilla ext/Detours ext/tgit ext/pcre2 || goto error
)

set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (
	echo vswhere.exe not found - is Visual Studio 2022 / Build Tools installed?
	goto error
)
set MSBUILD=
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set MSBUILD=%%i
if not defined MSBUILD (
	echo MSBuild.exe not found
	goto error
)
echo Using %MSBUILD%
echo Building %TARGET% %CONFIG%^|%PLATFORM% %TOOLSET%

"%MSBUILD%" src\TortoiseGit.sln /t:%TARGET% /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% %TOOLSET% /m /nologo /v:minimal /clp:Summary
if errorlevel 1 goto error

popd
echo.
echo Build succeeded.
exit /b 0

:error
popd
echo.
echo Build FAILED.
exit /b 1
