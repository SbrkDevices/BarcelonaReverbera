@echo off
echo.
echo GETTING EXTERNAL LIBRARIES...
echo.

set "JUCE_VERSION=8.0.1"
set "PFFFT_COMMIT_ID=fbc4058"  REM using specific commit tag instead of version number

set "SRC_DIR=%~dp0..\src"
set "WORK_DIR=%~dp0libraries_work_tmp"

set "JUCE_DIR=%SRC_DIR%\juce"
set "PFFFT_DIR=%SRC_DIR%\pffft"

rmdir /s /q "%WORK_DIR%" 2>nul

mkdir "%WORK_DIR%"
cd /d "%WORK_DIR%"

if not exist "%JUCE_DIR%" (
  powershell -Command "Invoke-WebRequest -Uri https://github.com/juce-framework/JUCE/archive/refs/tags/%JUCE_VERSION%.zip -OutFile juce-%JUCE_VERSION%.zip" || exit /b 1
  mkdir "%JUCE_DIR%" || exit /b 1
  powershell -Command "Expand-Archive -Path juce-%JUCE_VERSION%.zip -DestinationPath %JUCE_DIR%" || exit /b 1
)

if not exist "%PFFFT_DIR%" (
  powershell -Command "Invoke-WebRequest -Uri https://bitbucket.org/jpommier/pffft/get/%PFFFT_COMMIT_ID%.tar.gz -OutFile pffft-%PFFFT_COMMIT_ID%.tar.gz" || exit /b 1
  mkdir "%PFFFT_DIR%" || exit /b 1
  powershell -Command "tar -xzf pffft-%PFFFT_COMMIT_ID%.tar.gz -C %PFFFT_DIR% --strip-components=1" || exit /b 1
)

rmdir /s /q "%WORK_DIR%" 2>nul

echo DONE!
