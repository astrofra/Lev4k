@echo off
call "%~dp0build.bat" Snapshot %*
exit /b %errorlevel%
