@echo off

SET PATH=%PATH%;..\..\tools
SET WDK=C:\appWinDDK\7600.16385.1
REM 32-bit:  %ComSpec% /c "scd . && pushd . && %WDK%\bin\setenv %WDK% fre x86 WXP no_oacr && popd && build"
%ComSpec% /c "scd . && pushd . && %WDK%\bin\setenv %WDK% fre x64 WIN7 no_oacr && popd && build"
copy ..\..\library\objfre_win7_amd64\amd64\interception.dll .\objfre_win7_amd64\amd64
pause
