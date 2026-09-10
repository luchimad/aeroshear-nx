@echo off
setlocal enabledelayedexpansion

echo =========================================================
echo    COMPILANDO AEROSHEAR NX // ALPHA EDITION
echo =========================================================

:: Copiar raylib.dll al directorio raiz si existe
if exist "lib\raylib.dll" (
    copy /Y "lib\raylib.dll" "raylib.dll" >nul 2>&1
)

:: Compilación con GCC (MinGW-w64)
echo Compilando modulos C Alpha...
gcc -O2 -Wall -std=c99 -I include -I src ^
    src\main.c ^
    src\biome.c ^
    src\aircraft.c ^
    src\race.c ^
    src\player.c ^
    src\camera.c ^
    src\terrain.c ^
    src\scenery.c ^
    src\hud.c ^
    src\ui_theme.c ^
    src\ui_core.c ^
    src\ui_menu.c ^
    src\fx.c ^
    src\audio.c ^
    src\records.c ^
    src\music.c ^
    -o game.exe ^
    -L lib -lraylib -lopengl32 -lgdi32 -lwinmm -Wl,--defsym=stat64i32=_stat -Wl,--stack,16777216

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] La compilacion ha fallado. Revisa los mensajes anteriores.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [EXITO] Compilacion completada limpiamente: game.exe
echo Ejecutable standalone listo para itch.io.
echo =========================================================
