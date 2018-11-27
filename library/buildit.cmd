@echo off

echo buildit...

SET PATH=%PATH%;..\tools

%ComSpec% /c "scd . && pushd . && %WDK%\bin\setenv %WDK% fre x64 WIN7 no_oacr && popd && build"
