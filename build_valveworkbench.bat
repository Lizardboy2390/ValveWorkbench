@echo off
echo Building ValveWorkbench with Ceres Solver...
echo.

REM Set environment variable for Ceres
set CMAKE_PREFIX_PATH=C:/ceres_install/install_final
echo CMAKE_PREFIX_PATH set to %CMAKE_PREFIX_PATH%
echo.

REM Find Qt installation
echo Looking for Qt installation...
where qmake
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: qmake not found. Make sure Qt is installed and in your PATH.
    pause
    exit /b %ERRORLEVEL%
)
echo Qt installation found.
echo.

REM Run qmake
echo Running qmake...
qmake ValveWorkbench.pro
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: qmake failed.
    pause
    exit /b %ERRORLEVEL%
)
echo qmake completed successfully.
echo.

REM Build with nmake
echo Building with nmake...
nmake
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed.
    pause
    exit /b %ERRORLEVEL%
)
echo Build completed successfully.
echo.

echo ValveWorkbench has been built successfully.
echo You can run it by executing: ValveWorkbench.exe
echo.
pause
