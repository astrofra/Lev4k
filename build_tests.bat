@echo off
call "%~dp0build.bat" Test %*
exit /b %errorlevel%
