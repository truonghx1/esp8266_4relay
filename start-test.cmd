@echo off
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (
  echo Please install Node.js 18 or newer, then run this file again.
  pause
  exit /b 1
)
echo Open http://localhost:8266 in your browser.
echo Keep this window open while testing. Press Ctrl+C to stop.
node simulator\server.cjs %*
pause
