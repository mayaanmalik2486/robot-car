@echo off
rem WATCH THE CAR - double-click me: your real program drives around a pretend room on the computer,
rem and a little film of it opens. Try new code here before you upload it to the car.
rem Needs Node.js (https://nodejs.org) and the Arduino IDE.
cd /d "%~dp0"
echo Modes:  I search    1 chaos    P pet    2 auto forward    O radar
set letter=I
set /p letter=Which mode (letter, Enter = I)?
echo Rooms:  room    empty    clutter    corridor
set room=room
set /p room=Which room (Enter = room)?
node Tools\tools\watch.js %letter% %room%
echo.
pause
