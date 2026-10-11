@echo off
echo ========================================================
echo Installing MidiGlide VST3 Plugin to System VST3 Directory
echo ========================================================

set SRC=%~dp0build\MidiGlide_artefacts\Release\VST3\MidiGlide.vst3
set USER_DEST=%LOCALAPPDATA%\Programs\Common\VST3\MidiGlide.vst3
set SYS_DEST=%CommonProgramFiles%\VST3\MidiGlide.vst3

echo Cleaning up obsolete MidiScope plugin...
if exist "%LOCALAPPDATA%\Programs\Common\VST3\MidiScope.vst3" (
    rmdir /s /q "%LOCALAPPDATA%\Programs\Common\VST3\MidiScope.vst3" 2>nul
    echo [OK] Removed obsolete MidiScope from User VST3 directory.
)
if exist "%LOCALAPPDATA%\Programs\Common\VST3\MidiScope.vst3.old" (
    rmdir /s /q "%LOCALAPPDATA%\Programs\Common\VST3\MidiScope.vst3.old" 2>nul
)
if exist "%CommonProgramFiles%\VST3\MidiScope.vst3" (
    rmdir /s /q "%CommonProgramFiles%\VST3\MidiScope.vst3" 2>nul
    if not exist "%CommonProgramFiles%\VST3\MidiScope.vst3" (
        echo [OK] Removed obsolete MidiScope from System VST3 directory.
    ) else (
        echo [NOTE] Removing MidiScope from System VST3 requires Administrator privileges.
    )
)

echo.
echo [1/2] Installing to User VST3 directory (%USER_DEST%)...
xcopy /E /I /Y "%SRC%" "%USER_DEST%" >nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] Successfully copied to User VST3 directory.
) else (
    echo [WARN] Failed copying to User VST3 directory.
)

echo.
echo [2/2] Attempting to install to System VST3 directory (%SYS_DEST%)...
xcopy /E /I /Y "%SRC%" "%SYS_DEST%" >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] Successfully copied to System VST3 directory.
) else (
    echo [NOTE] System VST3 directory requires Administrator privileges.
    echo        If your DAW only scans %CommonProgramFiles%\VST3, right-click
    echo        this install.bat and select 'Run as Administrator'.
)

echo.
echo Installation complete!
pause
