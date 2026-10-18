@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
if not exist build\Tests mkdir build\Tests
cl /nologo /std:c++17 /EHsc /W4 /MD /utf-8 /I. NativeTests.cpp CsvFormat.cpp OdbcCsvExporter.cpp /Fe:build\Tests\NativeTests.exe /Fo:build\Tests\ /link odbc32.lib
if errorlevel 1 exit /b %errorlevel%
build\Tests\NativeTests.exe
exit /b %errorlevel%
