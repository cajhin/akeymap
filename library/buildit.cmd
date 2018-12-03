@echo off

echo buildit...

SET PATH=%PATH%;..\tools

if exist C:\appWinDDK\7600.16385.1 SET WDK=C:\appWinDDK\7600.16385.1
if exist C:\WinDDK\7600.16385.1 SET WDK=C:\WinDDK\7600.16385.1
echo WDK: %WDK%

%ComSpec% /c "scd . && pushd . && %WDK%\bin\setenv %WDK% fre x64 WIN7 no_oacr && popd && build"

pause
