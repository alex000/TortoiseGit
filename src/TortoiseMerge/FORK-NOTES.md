# TortoiseGitMerge fork notes

## Building from the command line

```
build-tortoisemerge.cmd init          :: first time: fetch the needed submodules
build-tortoisemerge.cmd               :: Release x64 -> bin\Release64\bin\TortoiseGitMerge.exe
build-tortoisemerge.cmd debug
build-tortoisemerge.cmd clang         :: clang-cl (ClangCL toolset) instead of cl.exe
```

The script finds MSBuild via `vswhere` and builds only the `TortoiseGitMerge` target
of `src\TortoiseGit.sln` (and the projects it references), so the IDE is not needed.
Requires Visual Studio 2022 or the free *Build Tools for Visual Studio 2022* with the
C++ desktop workload, ATL and MFC. The `clang` option also needs "C++ Clang tools for Windows".

### Why not MinGW / llvm-mingw

TortoiseGitMerge is an MFC application: `CView`/`CFrameWndEx` views, the MFC
feature pack (ribbon status bar, visual managers, `CMFC*` controls), MFC dialogs,
and ATL `CString` everywhere (also in the shared `src/Utils` code). MFC and ATL
are only distributed with MSVC and cannot be compiled by GCC/MinGW. A MinGW build
would mean rewriting the whole UI layer on another toolkit, i.e. a different program.

What does work with LLVM is `clang-cl` against the MSVC headers/libraries (the
`clang` option above). Cross-compiling from Linux is possible in principle with
clang-cl + lld-link and an MSVC sysroot that includes ATL/MFC (e.g. created with
msvc-wine), which requires accepting the Visual Studio license.

## Load-time profiling

Set `TGM_TIMING=1` before starting TortoiseGitMerge:

```
set TGM_TIMING=1
TortoiseGitMerge.exe /base:old.cpp /mine:new.cpp
type %TEMP%\TortoiseGitMerge-timing.log
```

Each line is `[time since process start] duration  step`; nested steps are indented
and are written when they *finish* (so a parent comes after its children). The log is
appended to, delete it between runs. Without the variable the instrumentation is a
no-op.

## Syntax highlighting

Uses the Lexilla lexers from `ScintillaLexer_tgit.dll`, which TortoiseGit already
ships for TortoiseGitBlame/TortoiseGitUDiff (must be next to `TortoiseGitMerge.exe`;
the build script builds it). Without the DLL highlighting is silently disabled.

- Toggle: ribbon *View → Syntax highlighting* (or the View menu without ribbons),
  stored in `HKCU\Software\TortoiseGitMerge\SyntaxHighlight` (DWORD, default 1).
- The language is chosen from the extension of the real file name (reflected name),
  falling back to the temp file name and the view title. See the table in
  `SyntaxHighlighter.cpp` (C/C++, C#, Java/Kotlin, JS/TS, Go, Rust, Python, Pascal,
  Lua, shell, batch, PowerShell, SQL, VB, Perl, Ruby, HTML/XML, JSON, YAML, INI,
  CMake, Makefile, diff, Markdown, asm).
- Diff background colors are kept; only the text color changes. Disabled in Windows
  high contrast mode.

## Encodings: OEM 866

- New encoding `OEM-866` in the encoding menus of the ribbon status bar and the
  context menu. Ctrl+click an encoding in the status bar menu to *reload* the file
  with that encoding (the normal click only changes the encoding used for saving).
- To decode all 8-bit files that are not valid UTF-8 as 866 by default:
  `HKCU\Software\TortoiseGitMerge\Default8BitCodePage` (DWORD) = `866`.
