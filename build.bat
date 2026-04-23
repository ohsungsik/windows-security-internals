@echo off
setlocal

rem Find latest Visual Studio installation using vswhere.exe
set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [error] vswhere.exe not found.
    exit /b 1
)

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"

if not defined VSINSTALL (
    echo [error] Visual Studio installation not found.
    exit /b 1
)

set "VCVARSALL=%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat"

if not exist "%VCVARSALL%" (
    echo [error] vcvarsall.bat not found.
    exit /b 1
)

echo [env] %VSINSTALL%
call "%VCVARSALL%" x64 >nul 2>&1
echo.

rem Build each cpp file found in subdirectories.
rem Adding a new folder with a cpp file automatically includes it.
for /d %%d in ("%~dp0*") do (
    if exist "%%d\*.cpp" (
        for %%f in ("%%d\*.cpp") do (
            echo [build] %%~nxd\%%~nxf
            cl.exe /nologo /EHsc /W4 /utf-8 ^
                /Fo:"%TEMP%\\" ^
                /Fe:"%%~dpf%%~nf.exe" ^
                "%%f" ^
                /link /SUBSYSTEM:CONSOLE
            del /q "%TEMP%\%%~nf.obj" 2>nul
            echo.
        )
    )
)

echo build completed.
pause
endlocal
