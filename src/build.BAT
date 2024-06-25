set PATH=%PATH%;C:\BC30\BIN\
rem Set "errorlevel="
bcc -IC:\BC30\INCLUDE -IC:\dest -LC:\BC30\LIB -ml -est.exe main.c util.c init.c
if errorlevel 1 goto failure
if errorlevel 0 goto end

:failure
pause
goto end

:end

rem EXIT /B 1
rem EXIT /B 0
