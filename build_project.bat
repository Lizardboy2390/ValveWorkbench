@echo off
echo Searching for Qt installation...

REM Try to find Qt in common installation paths
IF EXIST "C:\Qt\6.4.0\mingw_64\bin\qmake.exe" (
    echo Found Qt 6.4.0
    set "PATH=C:\Qt\6.4.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"
) ELSE IF EXIST "C:\Qt\6.3.0\mingw_64\bin\qmake.exe" (
    echo Found Qt 6.3.0
    set "PATH=C:\Qt\6.3.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"
) ELSE IF EXIST "C:\Qt\6.2.0\mingw_64\bin\qmake.exe" (
    echo Found Qt 6.2.0
    set "PATH=C:\Qt\6.2.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"
) ELSE IF EXIST "C:\Qt\5.15.2\mingw81_64\bin\qmake.exe" (
    echo Found Qt 5.15.2
    set "PATH=C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;%PATH%"
) ELSE (
    echo Could not find Qt installation. Please update the script with your Qt path.
    pause
    exit /b 1
)

echo Qt environment set up. Running qmake...
qmake

IF %ERRORLEVEL% NEQ 0 (
    echo qmake failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo Running mingw32-make...
mingw32-make

IF %ERRORLEVEL% NEQ 0 (
    echo mingw32-make failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo Build completed successfully!
pause
