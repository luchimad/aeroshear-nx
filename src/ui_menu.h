#ifndef UI_MENU_H
#define UI_MENU_H

#include "raylib.h"
#include "game_types.h"
#include "ui_theme.h"
#include "ui_core.h"
#include "aircraft.h"

// ============================================================================
// AEROSHEAR // MINIMAL MODULAR MENU SYSTEM (ALPHA EDITION)
// Flow: ENGAGE_ -> MAP SELECT -> VEHICLE SELECT -> SORTIE
//       OPTIONS_
//       CONTROLS_
//       EXIT_
// ============================================================================

typedef enum MenuAction {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_OPEN_MAP_SELECT,
    MENU_ACTION_OPEN_AIRCRAFT_SELECT,
    MENU_ACTION_OPEN_SEED_SELECT,
    MENU_ACTION_OPEN_SETTINGS,
    MENU_ACTION_OPEN_CONTROLS,
    MENU_ACTION_OPEN_RECORDS,
    MENU_ACTION_START_GAME,
    MENU_ACTION_RESUME_GAME,
    MENU_ACTION_RESTART_GAME,
    MENU_ACTION_TO_MAIN_MENU,
    MENU_ACTION_TO_MAP_SELECT,
    MENU_ACTION_TO_AIRCRAFT_SELECT,
    MENU_ACTION_EXIT_APP
} MenuAction;

void Menu_Init(void);
void Menu_SetSelectedBiome(BiomeType biome);

// Menú Principal (ENGAGE_, RECORDS_, OPTIONS_, CONTROLS_, EXIT_)
MenuAction Menu_UpdateMainMenu(void);
void Menu_DrawMainMenu(int screenWidth, int screenHeight);

// Pantalla de Récords / Hall of Fame (Top 5 Leaderboard)
MenuAction Menu_UpdateRecords(BiomeType *selectedBiome);
void Menu_DrawRecords(int screenWidth, int screenHeight, BiomeType selectedBiome);

// Selección de Mapa (Delta Straits / Hot Sands)
MenuAction Menu_UpdateMapSelect(BiomeType *selectedBiome);
void Menu_DrawMapSelect(int screenWidth, int screenHeight, BiomeType selectedBiome);

// Selección de Vehículo (RS-01 Phantom)
MenuAction Menu_UpdateAircraftSelect(int *selectedAircraftIdx);
void Menu_DrawAircraftSelect(int screenWidth, int screenHeight, int selectedAircraftIdx, const SpriteSheet *previewSprite);

// Configuración de Seed de Generación Procedural
MenuAction Menu_UpdateSeedSelect(char *seedBuffer, int maxLen, unsigned int *outSeed);
void Menu_DrawSeedSelect(int screenWidth, int screenHeight, const char *seedBuffer, BiomeType biome);

// Opciones (Fullscreen, Invert Pitch, CRT, Scanlines, Blue Grade, Volume)
MenuAction Menu_UpdateSettings(GameSettings *settings);
void Menu_DrawSettings(int screenWidth, int screenHeight, const GameSettings *settings);

// Guía Completa de Controles & Protocolos de Vuelo (CONTROLS_)
MenuAction Menu_UpdateControls(void);
void Menu_DrawControls(int screenWidth, int screenHeight);

// Menú de Pausa
MenuAction Menu_UpdatePauseMenu(void);
void Menu_DrawPauseMenu(int screenWidth, int screenHeight);

// Pantalla de Destrucción / Wreck Debriefing (Completamente en inglés)
MenuAction Menu_UpdateWreckDebriefing(void);
void Menu_DrawWreckDebriefing(int screenWidth, int screenHeight, const PlayerJet *player);

void Menu_Unload(void);

#endif // UI_MENU_H
