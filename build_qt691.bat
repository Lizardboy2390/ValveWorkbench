@echo off
echo Setting up Qt 6.9.1 environment...

REM Set up Qt 6.9.1 environment with MinGW 13.1.0
set "PATH=C:\Qt\6.9.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"

echo Qt environment set up. Running qmake...
qmake
if %ERRORLEVEL% NEQ 0 (
    echo qmake failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo Running mingw32-make...
mingw32-make
if %ERRORLEVEL% NEQ 0 (
    echo mingw32-make failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo Build completed successfully!
pause
