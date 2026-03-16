@echo off
cd /d "%~dp0"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cl.exe /EHsc /O2 /W4 /std:c++17 /Fe:sysping.exe sysping.cpp iphlpapi.lib ws2_32.lib 2>&1
