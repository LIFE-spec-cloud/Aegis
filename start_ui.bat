@echo off
title KisanSetu (MandiFlow) Interactive Web UI
echo ======================================================================
echo    KisanSetu (MandiFlow) - Multi-Agent Simulation Web Application
echo ======================================================================
echo.
echo Starting local web server on http://localhost:8080 ...
echo Opening your default browser...
echo.

start http://localhost:8080/web/index.html
python -m http.server 8080
if %errorlevel% neq 0 (
    echo Python server failed, trying direct browser file launch...
    start "" "%~dp0web\index.html"
)
pause
