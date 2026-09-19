@echo off
call "%~dp0build.bat" Release %*
exit /b %errorlevel%
