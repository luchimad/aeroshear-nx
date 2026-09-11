#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "config.h"
#include "game_types.h"
#include "aircraft.h"
#include "biome.h"
#include "race.h"
#include "player.h"
#include "camera.h"
#include "terrain.h"
#include "scenery.h"
#include "hud.h"
#include "ui_theme.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "fx.h"
#include "audio.h"
#include "music.h"
#include "records.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static void PresentScreenWithCRT(RenderTexture2D target, Shader scanShader, int locAlpha, int locDistort, int locPhosphor, int locBlue,
                                 float scanAlpha, float distort, float phosphor, float blueGrade, bool crtEnabled, int screenW, int screenH, float crtPowerOnTimer) {
    BeginDrawing();
        ClearBackground(BLACK);
        if (crtEnabled || blueGrade > 0.001f) {
            SetShaderValue(scanShader, locAlpha, &scanAlpha, SHADER_UNIFORM_FLOAT);
            SetShaderValue(scanShader, locDistort, &distort, SHADER_UNIFORM_FLOAT);
            SetShaderValue(scanShader, locPhosphor, &phosphor, SHADER_UNIFORM_FLOAT);
            if (locBlue >= 0) {
                SetShaderValue(scanShader, locBlue, &blueGrade, SHADER_UNIFORM_FLOAT);
            }

            BeginShaderMode(scanShader);
                Rectangle src = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
                Rectangle dst = { 0.0f, 0.0f, (float)screenW, (float)screenH };
                DrawTexturePro(target.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
            EndShaderMode();
        } else {
            Rectangle src = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
            Rectangle dst = { 0.0f, 0.0f, (float)screenW, (float)screenH };
            DrawTexturePro(target.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }
        FX_DrawCRTPowerOn(crtPowerOnTimer, screenW, screenH);
        if (IsKeyPressed(KEY_F12)) {
            TakeScreenshot("screenshot.png");
        }
    EndDrawing();
}

static TerrainSystem terrain;
static ScenerySystem scenery;
static PlayerJet player;
static FlightCamera flightCamera;
static RaceTrack race;

static void StartOrRestartFlight(RaceTrack *racePtr, PlayerJet *playerPtr, FlightCamera *cameraPtr,
                                 TerrainSystem *terrainPtr, ScenerySystem *sceneryPtr,
                                 BiomeType biome, int aircraftIdx, const GameSettings *settings,
                                 unsigned int seed) {
    // 1. Establecer el bioma activo primero para que el cálculo de alturas y relieve sea 100% exacto
    Biome_SetActive(biome);

    Vector3 startPos = { 0 };
    float startYaw = 0.0f;
    Race_Init(racePtr, biome, seed, &startPos, &startYaw);

    Player_Init(playerPtr);
    playerPtr->runSeed = seed;
    const AircraftDefinition *jet = Aircraft_Get(aircraftIdx);
    Player_ApplyAircraft(playerPtr, jet);
    Terrain_LoadBiome(terrainPtr, biome);
    Terrain_SetRenderDistance(terrainPtr, settings->renderDistance);
    Terrain_SetSpawnClearance(startPos, 750.0f);

    Scenery_LoadBiome(sceneryPtr, biome, seed);
    Scenery_SetTrackClearance(sceneryPtr, racePtr);
    Scenery_SetRenderDistance(sceneryPtr, settings->renderDistance);

    // Ajustar altura inicial al terreno plano recién nivelado en el spawn con colchón de 10m
    startPos.y = Terrain_GetHeight(startPos.x, startPos.z) + 11.0f;

    playerPtr->position = startPos;
    playerPtr->heading = startYaw;
    playerPtr->forward = (Vector3){ -sinf(startYaw), 0.0f, cosf(startYaw) };
    playerPtr->velocity = Vector3Scale(playerPtr->forward, playerPtr->cruiseSpeed);
    FlightCamera_Init(cameraPtr, playerPtr->position, playerPtr->forward);
    Terrain_ForceCenter(terrainPtr, playerPtr->position);
    Scenery_ForceCenter(sceneryPtr, playerPtr->position);
    FX_Init();
    Music_StartBiome(biome);
}

int main(int argc, char *argv[]) {
    bool autoCapture = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--capture") == 0) autoCapture = true;
    }
    int captureFrame = 0;
    // 1. Configuración de Ventana
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetExitKey(KEY_NULL);
    SetWindowMinSize(640, 360);
    SetTargetFPS(TARGET_FPS);
    rlSetClipPlanes(0.1, 16000.0);

    // 2. Inicialización de Subsistemas
    Audio_Init();
    Music_Init();
    FX_Init();
    UITheme_Init();
    Menu_Init();
    Records_Init();

    Terrain_Init(&terrain);
    Scenery_Init(&scenery, 101824u);
    Aircraft_Init();
    Player_Init(&player);
    FlightCamera_Init(&flightCamera, player.position, player.forward);

    // 3. Opciones Globales
    GameSettings gameSettings = {
        .renderDistance = RENDER_DIST_LOW,
        .pixelFilterEnabled = true,
        .scanlinesEnabled = true,
        .blueFilterEnabled = true,
        .hudTheme = HUD_THEME_CYAN,
        .masterVolume = 1.0f,
        .musicVolume = 0.80f,
        .voiceVolume = 0.40f, // Reducido 50%
        .nadiaEnabled = true,
        .invertPitch = false,
        .fullscreen = false
    };
    Terrain_SetRenderDistance(&terrain, gameSettings.renderDistance);
    Audio_SetMasterVolume(gameSettings.masterVolume);
    Music_SetVolume(gameSettings.musicVolume);
    Audio_SetVoiceVolume(gameSettings.voiceVolume);
    Audio_SetNadiaEnabled(gameSettings.nadiaEnabled);
    UITheme_SetHUDTheme(gameSettings.hudTheme);

    // Reproducción simultánea del efecto de encendido CRT y tema exclusivo de primer booteo
    Audio_PlayCRTPowerOn();
    Music_PlayBootMenu();

    int initialW = GetScreenWidth();
    int initialH = GetScreenHeight();

    RenderTexture2D sceneTarget = LoadRenderTexture(initialW, initialH);
    SetTextureFilter(sceneTarget.texture, TEXTURE_FILTER_BILINEAR);

    Shader scanlineShader = LoadShader("shaders/scanline.vs", "shaders/scanline.fs");
    int locScanRes      = GetShaderLocation(scanlineShader, "uResolution");
    int locScanAlpha    = GetShaderLocation(scanlineShader, "uScanlineAlpha");
    int locScanTime     = GetShaderLocation(scanlineShader, "uTime");
    int locScanDistort  = GetShaderLocation(scanlineShader, "uDistortion");
    int locScanPhosphor = GetShaderLocation(scanlineShader, "uPhosphorMask");
    int locScanBlue     = GetShaderLocation(scanlineShader, "uBlueGrade");

    Vector2 resVec = { (float)initialW, (float)initialH };
    SetShaderValue(scanlineShader, locScanRes, &resVec, SHADER_UNIFORM_VEC2);

    float currentDistortion = 0.0f;
    float crtPowerOnTimer = 0.0f;

    // Máquina de Estados
    GameState gameState = GAME_STATE_MAIN_MENU;
    int selectedAircraftIdx = 0;
    BiomeType selectedBiome = BIOME_DELTASTRAITS;
    char seedBuffer[16] = "";
    unsigned int currentRunSeed = 101824u;
    Menu_SetSelectedBiome(selectedBiome);

    // 4. Bucle Principal
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        Music_Update(dt);

        int curWidth = GetScreenWidth();
        int curHeight = GetScreenHeight();

        // Atajo Global para Pantalla Completa: F11 o Alt+Enter
        if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
            gameSettings.fullscreen = !gameSettings.fullscreen;
            ToggleFullscreen();
        }

        // Redimensionamiento de ventana
        if (IsWindowResized() || curWidth != sceneTarget.texture.width || curHeight != sceneTarget.texture.height) {
            if (curWidth > 0 && curHeight > 0) {
                UnloadRenderTexture(sceneTarget);
                sceneTarget = LoadRenderTexture(curWidth, curHeight);
                SetTextureFilter(sceneTarget.texture, TEXTURE_FILTER_BILINEAR);

                resVec = (Vector2){ (float)curWidth, (float)curHeight };
                SetShaderValue(scanlineShader, locScanRes, &resVec, SHADER_UNIFORM_VEC2);
            }
        }

        crtPowerOnTimer += dt;
        if (crtPowerOnTimer < 1.15f && (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
            crtPowerOnTimer = 2.0f;
        }

        if (autoCapture) {
            captureFrame++;
            if (captureFrame == 10) {
                crtPowerOnTimer = 2.0f;
            }
            if (captureFrame == 25) {
                TakeScreenshot("alpha_main_menu.png");
                gameState = GAME_STATE_RECORDS;
            }
            if (captureFrame == 45) {
                TakeScreenshot("alpha_records.png");
                selectedBiome = BIOME_HOTSANDS;
                gameState = GAME_STATE_SEED_SELECT;
                strcpy(seedBuffer, "519820");
            }
            if (captureFrame == 65) {
                TakeScreenshot("alpha_seed_menu.png");
                gameState = GAME_STATE_PLAYING;
                StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, selectedBiome, selectedAircraftIdx, &gameSettings, 519820u);
                race.isCountdown = false;
                race.isFinished = false;
            }
            if (captureFrame == 90) {
                TakeScreenshot("alpha_desert_city.png");
                // Cambiar a Delta Straits para verificar el agua, árboles y spawn parejo
                StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, BIOME_DELTASTRAITS, selectedAircraftIdx, &gameSettings, 777123u);
                race.isCountdown = false;
                race.isFinished = false;
            }
            if (captureFrame == 120) {
                TakeScreenshot("alpha_delta_straits.png");
                // Trigger death to capture Wreck Debriefing screen
                player.hullIntegrity = 0.0f;
                player.isDead = true;
                player.deathTimer = 2.5f; // CRT power-on completed, Wreck Debriefing visible
                player.fatalReason = "URBAN MONOLITH COLLAPSE";
                player.deathQuoteIndex = 0;
            }
            if (captureFrame == 140) {
                TakeScreenshot("alpha_wreck_screen.png");
                break;
            }
        }

        float totalTime = (float)GetTime();
        SetShaderValue(scanlineShader, locScanTime, &totalTime, SHADER_UNIFORM_FLOAT);

        // Distorsión por afterburner
        float speedDelta = (player.forwardSpeed - player.cruiseSpeed);
        float turboT = (player.afterburnerSpeed > player.cruiseSpeed) ? (speedDelta / (player.afterburnerSpeed - player.cruiseSpeed)) : 0.0f;
        turboT = Clamp(turboT, 0.0f, 1.0f);
        float targetDistort = (gameState == GAME_STATE_PLAYING) ? (turboT * 0.85f) : 0.0f;
        currentDistortion = Lerp(currentDistortion, targetDistort, dt * 3.5f);

        // ==========================================
        // ESTADO 1: MENÚ PRINCIPAL
        // ==========================================
        if (gameState == GAME_STATE_MAIN_MENU) {
            MenuAction action = Menu_UpdateMainMenu();
            if (action == MENU_ACTION_OPEN_MAP_SELECT) {
                gameState = GAME_STATE_MAP_SELECT;
            } else if (action == MENU_ACTION_OPEN_RECORDS) {
                gameState = GAME_STATE_RECORDS;
            } else if (action == MENU_ACTION_OPEN_SETTINGS) {
                gameState = GAME_STATE_SETTINGS;
            } else if (action == MENU_ACTION_OPEN_CONTROLS) {
                gameState = GAME_STATE_CONTROLS;
            } else if (action == MENU_ACTION_EXIT_APP) {
                break;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawMainMenu(curWidth, curHeight);
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 1.1: SALÓN DE LA FAMA (RECORDS_)
        // ==========================================
        else if (gameState == GAME_STATE_RECORDS) {
            MenuAction action = Menu_UpdateRecords(&selectedBiome);
            if (action == MENU_ACTION_TO_MAIN_MENU) {
                gameState = GAME_STATE_MAIN_MENU;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawRecords(curWidth, curHeight, selectedBiome);
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 2: SELECCIÓN DE MAPA (DELTA / HOTSANDS)
        // ==========================================
        else if (gameState == GAME_STATE_MAP_SELECT) {
            MenuAction action = Menu_UpdateMapSelect(&selectedBiome);
            if (action == MENU_ACTION_OPEN_AIRCRAFT_SELECT) {
                gameState = GAME_STATE_AIRCRAFT_SELECT;
            } else if (action == MENU_ACTION_TO_MAIN_MENU) {
                gameState = GAME_STATE_MAIN_MENU;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawMapSelect(curWidth, curHeight, selectedBiome);
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 3: SELECCIÓN DE VEHÍCULO (HANGAR)
        // ==========================================
        else if (gameState == GAME_STATE_AIRCRAFT_SELECT) {
            MenuAction action = Menu_UpdateAircraftSelect(&selectedAircraftIdx);
            if (action == MENU_ACTION_OPEN_SEED_SELECT) {
                gameState = GAME_STATE_SEED_SELECT;
            } else if (action == MENU_ACTION_START_GAME) {
                StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, selectedBiome, selectedAircraftIdx, &gameSettings, currentRunSeed);
                gameState = GAME_STATE_PLAYING;
            } else if (action == MENU_ACTION_TO_MAP_SELECT) {
                gameState = GAME_STATE_MAP_SELECT;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawAircraftSelect(curWidth, curHeight, selectedAircraftIdx, Aircraft_GetSprite(selectedAircraftIdx));
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 3.1: CONFIGURACIÓN DE SEED PROCEDURAL
        // ==========================================
        else if (gameState == GAME_STATE_SEED_SELECT) {
            MenuAction action = Menu_UpdateSeedSelect(seedBuffer, sizeof(seedBuffer), &currentRunSeed);
            if (action == MENU_ACTION_START_GAME) {
                StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, selectedBiome, selectedAircraftIdx, &gameSettings, currentRunSeed);
                gameState = GAME_STATE_PLAYING;
            } else if (action == MENU_ACTION_TO_AIRCRAFT_SELECT) {
                gameState = GAME_STATE_AIRCRAFT_SELECT;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawSeedSelect(curWidth, curHeight, seedBuffer, selectedBiome);
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 4: CONFIGURACIÓN Y AJUSTES
        // ==========================================
        else if (gameState == GAME_STATE_SETTINGS) {
            MenuAction action = Menu_UpdateSettings(&gameSettings);
            if (action == MENU_ACTION_TO_MAIN_MENU) {
                Terrain_SetRenderDistance(&terrain, gameSettings.renderDistance);
                Scenery_SetRenderDistance(&scenery, gameSettings.renderDistance);
                gameState = GAME_STATE_MAIN_MENU;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawSettings(curWidth, curHeight, &gameSettings);
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 5: MANUAL DE VUELO & CONTROLES (CONTROLS_)
        // ==========================================
        else if (gameState == GAME_STATE_CONTROLS) {
            MenuAction action = Menu_UpdateControls();
            if (action == MENU_ACTION_TO_MAIN_MENU) {
                gameState = GAME_STATE_MAIN_MENU;
            }

            BeginTextureMode(sceneTarget);
                ClearBackground(BLACK);
                Menu_DrawControls(curWidth, curHeight);
            EndTextureMode();

            float menuPhosphor = gameSettings.pixelFilterEnabled ? 0.65f : 0.25f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.045f, 0.0f, menuPhosphor, 0.0f, gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 6: VUELO ACTIVO (PLAYING)
        // ==========================================
        else if (gameState == GAME_STATE_PLAYING) {
            static bool s_wasDead = false;
            static bool s_deathCrtSoundPlayed = false;

            // --- GESTIÓN DE MUERTE CATASTRÓFICA Y WRECK DEBRIEFING ---
            if (player.isDead) {
                if (!s_wasDead) {
                    s_wasDead = true;
                    s_deathCrtSoundPlayed = false;
                    Music_Stop();
                    player.deathQuoteIndex = rand() % 8;
                }

                player.deathTimer += dt;

                // FASE 0 (0.0s a 0.20s): IMPACT FLASH & VISIBLE CRASH FRAME
                // Muestra la nave estampada en el obstáculo con trauma de cámara y fogonazo rojo
                if (player.deathTimer < 0.20f) {
                    const BiomeDefinition *bDef = Biome_Get(terrain.currentBiome);
                    FlightCamera_Update(&flightCamera, &player, dt);

                    BeginTextureMode(sceneTarget);
                        ClearBackground(bDef->skyZenithColor);
                        DrawRectangleGradientV(0, 0, curWidth, curHeight, bDef->skyZenithColor, bDef->skyHorizonColor);

                        BeginMode3D(flightCamera.camera);
                            Terrain_Draw(&terrain, &flightCamera.camera);
                            Scenery_Draw(&scenery, &flightCamera.camera);
                            Race_Draw3D(&race, &flightCamera.camera);
                            FX_DrawPlayerShadow(&player);
                            FX_Draw3D(&flightCamera.camera);
                        EndMode3D();

                        DrawPlayerSprite(&player, Aircraft_GetSprite(selectedAircraftIdx), &flightCamera.camera, curWidth, curHeight);
                        FX_Draw2D(&player, curWidth, curHeight);

                        // Fogonazo rojo de impacto catastrófico
                        float flashA = (0.20f - player.deathTimer) / 0.20f;
                        DrawRectangle(0, 0, curWidth, curHeight, (Color){ 255, 25, 25, (unsigned char)(140.0f * flashA) });
                    EndTextureMode();

                    float deathPhosphor = gameSettings.pixelFilterEnabled ? 0.75f : 0.35f;
                    PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                         0.105f, 0.45f, deathPhosphor, 0.0f,
                                         gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
                    continue;
                }

                // FASE 1: 0.20s a 2.0s -> PANTALLA EN NEGRO TOTAL Y SILENCIO ABSOLUTO TRAS EL IMPACTO
                if (player.deathTimer < 2.0f) {
                    Audio_StopAll();
                    BeginTextureMode(sceneTarget);
                        ClearBackground(BLACK);
                    EndTextureMode();

                    PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                         0.0f, 0.0f, 0.0f, 0.0f, false, curWidth, curHeight, 2.0f);
                } else {
                    // FASE 2: >= 2.0s -> REINICIO DE TUBO CRT + PANTALLA WRECK DEBRIEFING EN INGLÉS
                    if (!s_deathCrtSoundPlayed) {
                        Audio_PlayCRTPowerOn();
                        s_deathCrtSoundPlayed = true;
                    }

                    float deathCrtTimer = player.deathTimer - 2.0f;

                    BeginTextureMode(sceneTarget);
                        ClearBackground(BLACK);
                        Menu_DrawWreckDebriefing(curWidth, curHeight, &player);
                    EndTextureMode();

                    float deathPhosphor = gameSettings.pixelFilterEnabled ? 0.75f : 0.35f;
                    float deathBlue = gameSettings.blueFilterEnabled ? 0.60f : 0.0f;
                    PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                         0.085f, 0.0f, deathPhosphor, deathBlue,
                                         gameSettings.scanlinesEnabled, curWidth, curHeight, deathCrtTimer);

                    // Sólo procesar entradas de reinicio cuando el tubo CRT ya empezó a iluminarse
                    if (deathCrtTimer >= 0.30f) {
                        MenuAction wreckAction = Menu_UpdateWreckDebriefing();
                        if (wreckAction == MENU_ACTION_RESTART_GAME) {
                            StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, selectedBiome, selectedAircraftIdx, &gameSettings, currentRunSeed);
                            s_wasDead = false;
                            s_deathCrtSoundPlayed = false;
                        } else if (wreckAction == MENU_ACTION_TO_MAIN_MENU) {
                            gameState = GAME_STATE_MAIN_MENU;
                            Music_PlayMenu();
                            s_wasDead = false;
                            s_deathCrtSoundPlayed = false;
                        }
                    }
                }
                continue;
            } else {
                s_wasDead = false;
                s_deathCrtSoundPlayed = false;
            }

            // Reinicio tras finalizar carrera (solo si no se están ingresando iniciales)
            if (race.isFinished) {
                if (race.nameEntered || race.qualifyingRank < 0) {
                    bool wantRestart = IsKeyPressed(KEY_R) || IsKeyPressed(KEY_SPACE);
                    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) wantRestart = true;

                    if (wantRestart) {
                        StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, selectedBiome, selectedAircraftIdx, &gameSettings, currentRunSeed);
                    } else if (IsKeyPressed(KEY_ESCAPE)) {
                        gameState = GAME_STATE_MAIN_MENU;
                        Music_PlayMenu();
                    }
                }
            } else {
                bool wantPause = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P);
                if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) wantPause = true;

                if (wantPause) {
                    gameState = GAME_STATE_PAUSED;
                }
            }

            // Simulación física de vuelo
            if (!race.isCountdown && !race.isFinished) {
                float prevFlash = player.damageFlashTimer;
                Player_Update(&player, &gameSettings, dt);
                bool diedThisFrame = Scenery_CheckCollisions(&scenery, &player, dt);

                if (diedThisFrame) {
                    FlightCamera_AddTrauma(&flightCamera, 1.0f);
                    Audio_PlayWallImpact();
                    FX_SpawnWallSparks(player.position, (Vector3){ 0.0f, 1.0f, 0.0f }, 36);
                    continue;
                }

                // Si ocurrió un impacto con árbol en este frame, generar trauma de cámara y sonido de golpe
                if (prevFlash <= 0.0f && player.damageFlashTimer > 0.0f) {
                    FlightCamera_AddTrauma(&flightCamera, 0.55f);
                    Audio_PlayWallImpact();
                }

                if (player.wallImpactTriggered) {
                    FlightCamera_AddTrauma(&flightCamera, 0.45f);
                    player.wallImpactTriggered = false;
                }
                FlightCamera_Update(&flightCamera, &player, dt);
                Terrain_Update(&terrain, player.position);
                Scenery_Update(&scenery, player.position);

                const BiomeDefinition *bDef = Biome_Get(terrain.currentBiome);
                FX_Update(&player, dt, bDef->hasWater, terrain.currentBiome == BIOME_HOTSANDS, bDef->waterLevel);

                float speedRatio = player.forwardSpeed / player.afterburnerSpeed;
                Audio_Update(speedRatio, player.isAfterburner, player.isAirbrake, player.altitudeAGL, dt);
            } else if (race.isCountdown) {
                // Durante la cuenta atrás: actualizar cámara, terreno y vegetación para que el suelo esté 100% generado y visible
                FlightCamera_Update(&flightCamera, &player, dt);
                Terrain_Update(&terrain, player.position);
                Scenery_Update(&scenery, player.position);
                Audio_Update(0.0f, false, false, player.altitudeAGL, dt);
            } else if (race.isFinished) {
                // Detener propulsión y audio de vuelo en segundo plano al cruzar la meta
                Audio_Update(0.0f, false, false, player.altitudeAGL, dt);
            }

            Race_Update(&race, &player, dt);
            if (race.cameraSnapRequested) {
                FlightCamera_Init(&flightCamera, player.position, player.forward);
                race.cameraSnapRequested = false;
            }

            const BiomeDefinition *bDef = Biome_Get(terrain.currentBiome);

            // PASS 1: Render 3D Scene en resolución nativa
            BeginTextureMode(sceneTarget);
                ClearBackground(bDef->skyZenithColor);
                DrawRectangleGradientV(0, 0, curWidth, curHeight, bDef->skyZenithColor, bDef->skyHorizonColor);

                BeginMode3D(flightCamera.camera);
                    Terrain_Draw(&terrain, &flightCamera.camera);
                    Scenery_Draw(&scenery, &flightCamera.camera);
                    Race_Draw3D(&race, &flightCamera.camera);
                    FX_DrawPlayerShadow(&player);
                    FX_Draw3D(&flightCamera.camera);
                EndMode3D();

                DrawPlayerSprite(&player, Aircraft_GetSprite(selectedAircraftIdx), &flightCamera.camera, curWidth, curHeight);
                FX_Draw2D(&player, curWidth, curHeight);

                // PASS 2: Flight HUD tDR & Navegador
                if (!race.isFinished) {
                    HUD_Draw(&player, curWidth, curHeight);
                }
                Race_DrawHUD(&race, &player, &flightCamera.camera, curWidth, curHeight);
            EndTextureMode();

            // PASS 3: CRT Scanlines & Post-procesado cinematográfico
            float gamePhosphor = gameSettings.pixelFilterEnabled ? 0.85f : 0.40f;
            float gameBlue = gameSettings.blueFilterEnabled ? 0.90f : 0.0f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.105f, currentDistortion, gamePhosphor, gameBlue,
                                 gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
        // ==========================================
        // ESTADO 7: MENÚ DE PAUSA TÁCTICO
        // ==========================================
        else if (gameState == GAME_STATE_PAUSED) {
            Audio_Update(0.0f, false, false, 100.0f, dt);

            MenuAction action = Menu_UpdatePauseMenu();
            if (action == MENU_ACTION_RESUME_GAME) {
                gameState = GAME_STATE_PLAYING;
            } else if (action == MENU_ACTION_RESTART_GAME) {
                StartOrRestartFlight(&race, &player, &flightCamera, &terrain, &scenery, selectedBiome, selectedAircraftIdx, &gameSettings, currentRunSeed);
                gameState = GAME_STATE_PLAYING;
            } else if (action == MENU_ACTION_TO_MAIN_MENU) {
                gameState = GAME_STATE_MAIN_MENU;
                Music_PlayMenu();
            } else if (action == MENU_ACTION_EXIT_APP) {
                break;
            }

            BeginTextureMode(sceneTarget);
                Menu_DrawPauseMenu(curWidth, curHeight);
            EndTextureMode();

            float pauseBlue = gameSettings.blueFilterEnabled ? 0.90f : 0.0f;
            PresentScreenWithCRT(sceneTarget, scanlineShader, locScanAlpha, locScanDistort, locScanPhosphor, locScanBlue,
                                 0.065f, 0.0f, 0.50f, pauseBlue,
                                 gameSettings.scanlinesEnabled, curWidth, curHeight, crtPowerOnTimer);
        }
    }

    // 5. Limpieza de Recursos
    UnloadRenderTexture(sceneTarget);
    UnloadShader(scanlineShader);
    FX_Unload();
    Music_Unload();
    Audio_Unload();
    Race_Unload(&race);
    Scenery_Unload(&scenery);
    Terrain_Unload(&terrain);
    Menu_Unload();
    Aircraft_UnloadAll();
    UITheme_Unload();
    CloseWindow();

    return 0;
}
