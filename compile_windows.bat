@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>&1
if errorlevel 1 (
  echo CMake absent. Installer CMake 3.22+ et Visual Studio 2022 C++.
  pause
  exit /b 1
)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :fail
cmake --build build --config Release --parallel 4 --target MOOGER_VST3 MOOGER_Standalone anks_audio_smoke mooger_modules mooger_percussion mooger_modulation mooger_processor_tests
if errorlevel 1 goto :fail
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto :fail
echo.
echo FINI : verifier build\MOOGER_artefacts\Release\VST3\
echo       et build\MOOGER_artefacts\Release\Standalone\
pause
exit /b 0
:fail
echo Erreur de compilation. Lire les messages ci-dessus.
pause
exit /b 1
