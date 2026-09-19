@echo off
call "%~dp0build.bat" Editor %*
exit /b %errorlevel%
