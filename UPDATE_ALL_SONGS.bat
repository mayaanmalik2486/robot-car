@echo off
rem UPDATE ALL SONGS - double-click me after you change Robot_Car_Main.
rem Every song file in Dances gets your change (each keeps its own song and choreography),
rem then every program is checked to make sure it still compiles.
rem Needs Node.js (https://nodejs.org) and the Arduino IDE.
cd /d "%~dp0"
node Tools\tools\update-all-songs.js
echo.
pause
