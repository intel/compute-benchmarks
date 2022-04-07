REM
REM Copyright (C) 2022 Intel Corporation
REM
REM SPDX-License-Identifier: MIT
REM

@echo off
setlocal enabledelayedexpansion

SET CLANG_FORMAT_BINARY="clang-format.exe"
SET EXTENSIONS_TO_PROCESS=(c, h, cpp, hpp, cxx, hxx, cc, inl)

IF NOT EXIST %CLANG_FORMAT_BINARY% (
  GOTO :FAIL
)


FOR %%e IN %EXTENSIONS_TO_PROCESS% DO (
  ECHO Extension: %%e
  FOR /R ..\source %%f IN (*.%%e) DO (
    %CLANG_FORMAT_BINARY% -i -style=file %%f
  )
)

:EXIT
EXIT /B 0

:FAIL
echo.
echo Script FAILED
EXIT /B 1
