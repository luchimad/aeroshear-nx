#include "ui_menu.h"
#include "config.h"
#include "biome.h"
#include "audio.h"
#include "music.h"
#include "records.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static int s_mainMenuSelection = 0;
static int s_mapSelection = 0;
static int s_settingsSelection = 0;
static int s_pauseSelection = 0;

void Menu_Init(void) {
    s_mainMenuSelection = 0;
    s_mapSelection = 0;
    s_settingsSelection = 0;
    s_pauseSelection = 0;
}

void Menu_SetSelectedBiome(BiomeType biome) {
    s_mapSelection = (int)biome;
}

// ============================================================================
// 1. MENÚ PRINCIPAL MINIMALISTA (ENGAGE_, RECORDS_, OPTIONS_, CONTROLS_, EXIT_)
// ============================================================================
MenuAction Menu_UpdateMainMenu(void) {
    const int totalOptions = 5;

    // Navegación Teclado
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_mainMenuSelection = (s_mainMenuSelection - 1 + totalOptions) % totalOptions;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_mainMenuSelection = (s_mainMenuSelection + 1) % totalOptions;
    }

    // Navegación Gamepad
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
            s_mainMenuSelection = (s_mainMenuSelection - 1 + totalOptions) % totalOptions;
        }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
            s_mainMenuSelection = (s_mainMenuSelection + 1) % totalOptions;
        }
        float gpY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        static bool s_gpNavCooldown = false;
        if (fabsf(gpY) > 0.65f) {
            if (!s_gpNavCooldown) {
                if (gpY < -0.65f) s_mainMenuSelection = (s_mainMenuSelection - 1 + totalOptions) % totalOptions;
                if (gpY > 0.65f)  s_mainMenuSelection = (s_mainMenuSelection + 1) % totalOptions;
                s_gpNavCooldown = true;
            }
        } else {
            s_gpNavCooldown = false;
        }
    }

    // Confirmación
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        confirm = true;
    }

    Vector2 mousePos = GetMousePosition();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int leftX = (int)fmaxf(48.0f, (float)screenW * 0.085f);
    int btnW = 280;
    int btnH = 42;
    int startY = (int)((float)screenH * 0.38f);
    int spacing = 52;

    for (int i = 0; i < totalOptions; i++) {
        Rectangle btnRec = { (float)leftX, (float)(startY + i * spacing), (float)btnW, (float)btnH };
        if (CheckCollisionPointRec(mousePos, btnRec)) {
            s_mainMenuSelection = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                confirm = true;
            }
        }
    }

    if (confirm) {
        if (s_mainMenuSelection == 0) return MENU_ACTION_OPEN_MAP_SELECT;
        if (s_mainMenuSelection == 1) return MENU_ACTION_OPEN_RECORDS;
        if (s_mainMenuSelection == 2) return MENU_ACTION_OPEN_SETTINGS;
        if (s_mainMenuSelection == 3) return MENU_ACTION_OPEN_CONTROLS;
        if (s_mainMenuSelection == 4) return MENU_ACTION_EXIT_APP;
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawMainMenu(int screenWidth, int screenHeight) {
    float time = (float)GetTime();

    // Mouse parallax calculation para reactividad orgánica
    Vector2 mousePos = GetMousePosition();
    Vector2 targetParallax = {
        ((mousePos.x / (float)screenWidth) - 0.5f) * 2.0f,
        ((mousePos.y / (float)screenHeight) - 0.5f) * 2.0f
    };
    Color biomeTint = (Color){ 82, 175, 240, 255 };

    // 1. Fondo reactivo orgánico shader en GPU Dark Future Blue
    UI_DrawOrbitalLimbEx(screenWidth, screenHeight, time, targetParallax, biomeTint);

    // 2. Barra de registro técnico superior
    UI_DrawTopRegistrationBar(screenWidth, 44);

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int titleY = (int)((float)screenHeight * 0.16f);

    // 3. Logo oficial en Neuropolitical y subtítulos con espaciado respirable
    UI_DrawTitleAeroshear(leftX, titleY, 1.0f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextTitle("FLIGHT CORE // PROOF OF CONCEPT", (float)leftX, (float)(titleY + 56), 15.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("PoC Build 0.1", (float)leftX, (float)(titleY + 80), 11.0f, UI_COLOR_MUTED_TEXT);

    DrawLine(leftX, titleY + 102, leftX + 440, titleY + 102, (Color){ 45, 95, 145, 140 });

    const char *options[5] = { "ENGAGE_", "RECORDS_", "OPTIONS_", "CONTROLS_", "EXIT_" };

    int startY = (int)((float)screenHeight * 0.38f);
    int spacing = 52;
    int btnW = (int)fminf(320.0f, (float)screenWidth * 0.35f);
    int btnH = 42;

    for (int i = 0; i < 5; i++) {
        Rectangle btnRec = { (float)leftX, (float)(startY + i * spacing), (float)btnW, (float)btnH };
        bool isSelected = (i == s_mainMenuSelection);
        UI_DrawMenuButton(btnRec, options[i], isSelected, mousePos);
    }

    UI_DrawNavHelp(screenWidth, screenHeight, "[W/S / UP/DN / D-PAD]: NAVIGATE    [ENTER / SPACE / (A)]: SELECT    [F11 / ALT+ENTER]: FULLSCREEN");
}

// ============================================================================
// 1.1. SALÓN DE LA FAMA & RÉCORDS LOCALES (HALL OF FAME TOP 5)
// ============================================================================
MenuAction Menu_UpdateRecords(BiomeType *selectedBiome) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) return MENU_ACTION_TO_MAIN_MENU;
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        return MENU_ACTION_TO_MAIN_MENU;
    }

    // Conmutar circuito (Delta Straits <-> Hot Sands)
    bool switchCircuit = false;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_TAB)) {
        switchCircuit = true;
    }
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
            IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
            switchCircuit = true;
        }
    }

    if (switchCircuit && selectedBiome) {
        *selectedBiome = (*selectedBiome == BIOME_DELTASTRAITS) ? BIOME_HOTSANDS : BIOME_DELTASTRAITS;
    }

    // Clics con ratón en las pestañas o botón volver
    Vector2 mousePos = GetMousePosition();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int leftX = (int)fmaxf(48.0f, (float)screenW * 0.085f);
    int topY = (int)((float)screenH * 0.16f);
    int tabY = topY + 56;
    int tabW = 230;
    int tabH = 38;

    Rectangle tab0 = { (float)leftX, (float)tabY, (float)tabW, (float)tabH };
    Rectangle tab1 = { (float)(leftX + tabW + 14), (float)tabY, (float)tabW, (float)tabH };
    int retW = 160;
    Rectangle retRec = { (float)(screenW - leftX - retW), (float)tabY, (float)retW, (float)tabH };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePos, tab0) && selectedBiome) {
            *selectedBiome = BIOME_DELTASTRAITS;
        } else if (CheckCollisionPointRec(mousePos, tab1) && selectedBiome) {
            *selectedBiome = BIOME_HOTSANDS;
        } else if (CheckCollisionPointRec(mousePos, retRec)) {
            return MENU_ACTION_TO_MAIN_MENU;
        }
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawRecords(int screenWidth, int screenHeight, BiomeType selectedBiome) {
    float time = (float)GetTime();
    Vector2 mousePos = GetMousePosition();
    Vector2 targetParallax = {
        ((mousePos.x / (float)screenWidth) - 0.5f) * 2.0f,
        ((mousePos.y / (float)screenHeight) - 0.5f) * 2.0f
    };
    const BiomeDefinition *bDef = Biome_Get(selectedBiome);
    Color accent = bDef ? bDef->themeAccentColor : UI_COLOR_AC4_CYAN;

    // 1. Fondo reactivo orbital con tinte dinámico de bioma
    UI_DrawOrbitalLimbEx(screenWidth, screenHeight, time, targetParallax, accent);

    // 2. Cabecera táctica unificada
    UI_DrawScreenHeader(screenWidth, "CIRCUIT RECORDS // HALL OF FAME", "VERIFIED LOCAL FLIGHT TIME TRIALS [TOP 5]", "CLASSIFIED ARCHIVE // REC.01");

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int topY = (int)((float)screenHeight * 0.16f);
    int tabY = topY + 56;
    int tabW = 230;
    int tabH = 38;

    // 3. Pestañas de circuitos (Tabs)
    const char *circuitNames[2] = { "01 // DELTA STRAITS", "02 // HOT SANDS" };
    for (int i = 0; i < 2; i++) {
        Rectangle tRec = { (float)(leftX + i * (tabW + 14)), (float)tabY, (float)tabW, (float)tabH };
        bool isCurrent = (i == (int)selectedBiome);
        const BiomeDefinition *tabBDef = Biome_Get((BiomeType)i);
        Color tabCol = tabBDef ? tabBDef->themeAccentColor : UI_COLOR_AC4_CYAN;

        if (isCurrent) {
            DrawRectangleRounded(tRec, 0.25f, 4, (Color){ 12, 28, 48, 230 });
            DrawRectangleRoundedLinesEx(tRec, 0.25f, 4, 1.2f, tabCol);
            DrawRectangleGradientH((int)tRec.x + 2, (int)tRec.y + 2, (int)(tRec.width * 0.7f), (int)tRec.height - 4,
                                   (Color){ tabCol.r, tabCol.g, tabCol.b, 60 }, (Color){ tabCol.r, tabCol.g, tabCol.b, 0 });
            UI_DrawTextMenu(circuitNames[i], tRec.x + 18, tRec.y + 10, 13.0f, UI_COLOR_STEEL_WHITE);
        } else {
            DrawRectangleRounded(tRec, 0.25f, 4, (Color){ 4, 10, 20, 160 });
            DrawRectangleRoundedLinesEx(tRec, 0.25f, 4, 1.0f, (Color){ 35, 65, 95, 120 });
            UI_DrawTextMenu(circuitNames[i], tRec.x + 18, tRec.y + 10, 13.0f, UI_COLOR_MUTED_TEXT);
        }
    }

    // Botón interactivo de retorno
    int retW = 160;
    Rectangle retRec = { (float)(screenWidth - leftX - retW), (float)tabY, (float)retW, (float)tabH };
    bool isRetHover = CheckCollisionPointRec(mousePos, retRec);
    DrawRectangleRounded(retRec, 0.25f, 4, isRetHover ? (Color){ 20, 42, 68, 220 } : (Color){ 4, 10, 20, 160 });
    DrawRectangleRoundedLinesEx(retRec, 0.25f, 4, 1.0f, isRetHover ? UI_COLOR_AC4_CYAN : (Color){ 35, 65, 95, 120 });
    UI_DrawTextMenu("< RETURN", retRec.x + 26, retRec.y + 10, 12.0f, isRetHover ? UI_COLOR_STEEL_WHITE : UI_COLOR_MUTED_TEXT);

    // 4. Panel de Leaderboard Top 5 en Glass Panel
    int panelY = tabY + tabH + 16;
    int panelW = screenWidth - leftX * 2;
    int panelH = (int)fminf(380.0f, (float)screenHeight - panelY - 70.0f);
    Rectangle pRec = { (float)leftX, (float)panelY, (float)panelW, (float)panelH };

    UI_DrawGlassPanel(pRec, "TOP 5 TELEMETRY ARCHIVE // ASNX-V2 ENCRYPTED RECORD", accent, UI_COLOR_PANEL_BG);

    // Cabecera de columnas de la tabla
    int colPos   = leftX + 24;
    int colTag   = leftX + 86;
    int colSeed  = leftX + 168;
    int colTime  = leftX + 265;
    int colSpeed = leftX + (int)(panelW * 0.44f);
    int colRank  = leftX + (int)(panelW * 0.65f);
    int headerY  = panelY + 36;

    UI_DrawTextHud("POS",       (float)colPos,   (float)headerY, 11.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("PILOT",     (float)colTag,   (float)headerY, 11.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("SEED",      (float)colSeed,  (float)headerY, 11.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("CHRONO",    (float)colTime,  (float)headerY, 11.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("PEAK VELOCITY", (float)colSpeed, (float)headerY, 11.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("CLASSIFICATION / STATUS", (float)colRank,  (float)headerY, 11.0f, UI_COLOR_AC4_CYAN);

    DrawLine(leftX + 20, headerY + 18, leftX + panelW - 20, headerY + 18, (Color){ 45, 95, 145, 150 });

    const CircuitLeaderboard *board = Records_GetLeaderboard(selectedBiome);
    int rowStartY = headerY + 28;
    int rowSpacing = (panelH - 100) / 5;
    if (rowSpacing < 36) rowSpacing = 36;

    for (int i = 0; i < TOP_SCORES_COUNT; i++) {
        int rY = rowStartY + i * rowSpacing;
        const HighscoreEntry *e = &board->entries[i];

        // Barra de fondo alternada para cada fila
        if (i % 2 == 1) {
            DrawRectangle(leftX + 16, rY - 4, panelW - 32, rowSpacing - 4, (Color){ 12, 26, 44, 75 });
        }

        // Color según posición en podio
        Color rankCol = UI_COLOR_STEEL_WHITE;
        if (i == 0) rankCol = (Color){ 255, 215, 60, 255 };      // Oro / 1st place
        else if (i == 1) rankCol = (Color){ 215, 230, 245, 240 }; // Plata
        else if (i == 2) rankCol = (Color){ 220, 160, 100, 230 }; // Bronce
        else rankCol = UI_COLOR_MUTED_TEXT;

        // POS
        const char *posStr = TextFormat("#0%d", i + 1);
        UI_DrawTextTitle(posStr, (float)colPos, (float)rY, 14.0f, rankCol);

        // PILOT TAG
        if (e->isValid) {
            UI_DrawTextTitle(TextFormat("[%s]", e->pilotTag), (float)colTag, (float)rY, 14.0f, (i == 0) ? (Color){ 255, 225, 100, 255 } : UI_COLOR_STEEL_WHITE);
        } else {
            UI_DrawTextTitle("[---]", (float)colTag, (float)rY, 14.0f, UI_COLOR_MUTED_TEXT);
        }

        // SEED
        if (e->isValid && e->seed > 0) {
            UI_DrawTextHud(TextFormat("%06u", e->seed), (float)colSeed, (float)(rY + 3), 11.0f, UI_COLOR_AC4_CYAN);
        } else if (e->isValid) {
            UI_DrawTextHud("AUTO", (float)colSeed, (float)(rY + 3), 11.0f, UI_COLOR_MUTED_TEXT);
        } else {
            UI_DrawTextHud("------", (float)colSeed, (float)(rY + 3), 11.0f, UI_COLOR_MUTED_TEXT);
        }

        // TIME (CHRONO)
        if (e->isValid && e->finishTime > 0.0f) {
            int mins = (int)(e->finishTime / 60.0f);
            float secs = fmodf(e->finishTime, 60.0f);
            UI_DrawTextMenu(TextFormat("%02d:%05.2f", mins, secs), (float)colTime, (float)(rY + 1), 14.0f, (i == 0) ? UI_COLOR_AC4_CYAN : UI_COLOR_STEEL_WHITE);
        } else {
            UI_DrawTextMenu("--:--.--", (float)colTime, (float)(rY + 1), 14.0f, UI_COLOR_MUTED_TEXT);
        }

        // PEAK SPEED
        if (e->isValid && e->maxSpeedKmh > 0.0f) {
            UI_DrawTextHud(TextFormat("%.0f KM/H", e->maxSpeedKmh), (float)colSpeed, (float)(rY + 3), 11.0f, UI_COLOR_AC4_GREEN);
        } else {
            UI_DrawTextHud("--- KM/H", (float)colSpeed, (float)(rY + 3), 11.0f, UI_COLOR_MUTED_TEXT);
        }

        // RANK / ACCREDITATION
        if (e->isValid && strlen(e->rank) > 0) {
            UI_DrawTextHud(e->rank, (float)colRank, (float)(rY + 3), 11.0f, (i == 0) ? UI_COLOR_AC4_CYAN : UI_COLOR_STEEL_WHITE);
        } else {
            UI_DrawTextHud("STANDBY", (float)colRank, (float)(rY + 3), 11.0f, UI_COLOR_MUTED_TEXT);
        }
    }

    // Pie de panel técnico
    int footY = panelY + panelH - 24;
    DrawLine(leftX + 20, footY - 6, leftX + panelW - 20, footY - 6, (Color){ 35, 75, 115, 120 });
    UI_DrawTextHud("STORAGE BUS // records.dat [V3-ASNX] // HALL OF FAME FLIGHT DATABASE", (float)(leftX + 24), (float)footY, 10.0f, UI_COLOR_MUTED_TEXT);

    // Ayuda de navegación
    UI_DrawNavHelp(screenWidth, screenHeight, "[A/D / LEFT/RIGHT / TAB]: SWITCH CIRCUIT    [ESC / (B)]: RETURN TO MAIN MENU");
}

// ============================================================================
// 2. SELECCIÓN DE MAPA (DELTA STRAITS / HOT SANDS)
// ============================================================================
MenuAction Menu_UpdateMapSelect(BiomeType *selectedBiome) {
    const int totalMaps = 2;

    if (IsKeyPressed(KEY_ESCAPE)) return MENU_ACTION_TO_MAIN_MENU;
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        return MENU_ACTION_TO_MAIN_MENU;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        s_mapSelection = (s_mapSelection - 1 + totalMaps) % totalMaps;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        s_mapSelection = (s_mapSelection + 1) % totalMaps;
    }

    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
            s_mapSelection = (s_mapSelection - 1 + totalMaps) % totalMaps;
        }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
            s_mapSelection = (s_mapSelection + 1) % totalMaps;
        }
    }

    if (selectedBiome) *selectedBiome = (BiomeType)s_mapSelection;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        return MENU_ACTION_OPEN_AIRCRAFT_SELECT;
    }
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        return MENU_ACTION_OPEN_AIRCRAFT_SELECT;
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawMapSelect(int screenWidth, int screenHeight, BiomeType selectedBiome) {
    float time = (float)GetTime();
    Vector2 mousePos = GetMousePosition();
    Vector2 targetParallax = {
        ((mousePos.x / (float)screenWidth) - 0.5f) * 2.0f,
        ((mousePos.y / (float)screenHeight) - 0.5f) * 2.0f
    };
    const BiomeDefinition *bDef = Biome_Get(selectedBiome);
    Color biomeTint = bDef ? bDef->themeAccentColor : (Color){ 82, 175, 240, 255 };

    // Fondo reactivo orgánico shader en GPU Dark Future Blue con tinte de bioma
    UI_DrawOrbitalLimbEx(screenWidth, screenHeight, time, targetParallax, biomeTint);

    // Cabecera táctica unificada
    UI_DrawScreenHeader(screenWidth, "THEATER SELECTION // SORTIE ZONE", "SELECT TARGET ENVIRONMENT", "SECTOR DEPLOYMENT // SEC.01");

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int topY = (int)((float)screenHeight * 0.16f);

    int btnW = (int)fminf(340.0f, (float)screenWidth * 0.36f);
    int btnH = 54;
    int btnY = topY + 65;

    // Botones de los 2 mapas en cápsulas Liquid Glass Dark Future Blue
    for (int i = 0; i < 2; i++) {
        const BiomeDefinition *b = Biome_Get((BiomeType)i);
        bool isSel = (i == (int)selectedBiome);
        Rectangle r = { (float)leftX, (float)(btnY + i * 68), (float)btnW, (float)btnH };
        Color accent = b->themeAccentColor;

        if (isSel) {
            DrawRectangleRounded((Rectangle){ r.x, r.y + 3, r.width, r.height }, 0.22f, 6, (Color){ 0, 15, 35, 140 });
            DrawRectangleRounded(r, 0.22f, 6, (Color){ 14, 32, 56, 220 });
            DrawRectangleGradientH((int)r.x + 2, (int)r.y + 2, (int)(r.width * 0.75f), (int)r.height - 4,
                (Color){ accent.r, accent.g, accent.b, 65 }, (Color){ accent.r, accent.g, accent.b, 0 });
            DrawRectangleGradientV((int)r.x + 4, (int)r.y + 2, (int)r.width - 8, (int)(r.height * 0.46f),
                (Color){ 255, 255, 255, 100 }, (Color){ 255, 255, 255, 0 });
            DrawLine((int)r.x + 12, (int)r.y, (int)(r.x + r.width - 12), (int)r.y, (Color){ 255, 255, 255, 150 });
            DrawRectangleRoundedLinesEx(r, 0.22f, 6, 1.2f, accent);
            DrawRectangleRounded((Rectangle){ (float)(leftX + 3), (float)(btnY + i * 68 + 5), 4.0f, 44.0f }, 0.5f, 4, accent);

            UI_DrawTextMenu(b->name, (float)(leftX + 22), (float)(btnY + i * 68 + 10), 16.0f, UI_COLOR_STEEL_WHITE);
            UI_DrawTextHud(b->subtitle, (float)(leftX + 22), (float)(btnY + i * 68 + 32), 10.0f, accent);
        } else {
            DrawRectangleRounded(r, 0.22f, 6, (Color){ 6, 16, 30, 115 });
            DrawLine((int)r.x + 12, (int)r.y, (int)(r.x + r.width - 12), (int)r.y, (Color){ 255, 255, 255, 25 });
            DrawRectangleRoundedLinesEx(r, 0.22f, 6, 1.0f, (Color){ 45, 90, 140, 60 });

            UI_DrawTextMenu(b->name, (float)(leftX + 18), (float)(btnY + i * 68 + 10), 16.0f, (Color){ 140, 180, 215, 195 });
            UI_DrawTextHud(b->climate, (float)(leftX + 18), (float)(btnY + i * 68 + 32), 10.0f, (Color){ 80, 130, 175, 150 });
        }
    }

    // Panel de información del mapa (lado derecho, Glass Pod oscuro con tinte de bioma)
    int cardW = (int)fminf(510.0f, (float)screenWidth * 0.44f);
    int cardX = screenWidth - cardW - leftX;
    int cardY = topY + 65;
    int cardH = 285;
    Rectangle cardRec = { (float)cardX, (float)cardY, (float)cardW, (float)cardH };

    UI_DrawGlassPanel(cardRec, "SECTOR INTEL // RECONNAISSANCE", bDef->themeAccentColor, UI_COLOR_PANEL_BG);

    Color textPrimary = UI_COLOR_STEEL_WHITE;
    Color textAccent = bDef->themeAccentColor;

    UI_DrawTextTitle(bDef->name, (float)(cardX + 22), (float)(cardY + 24), 17.0f, textPrimary);
    UI_DrawTextHud(bDef->climate, (float)(cardX + 22), (float)(cardY + 48), 11.0f, textAccent);
    DrawLine(cardX + 22, cardY + 66, cardX + cardW - 22, cardY + 66, (Color){ 35, 75, 115, 140 });

    UI_DrawTextHud("TACTICAL BRIEFING:", (float)(cardX + 22), (float)(cardY + 78), 10.0f, UI_COLOR_MUTED_TEXT);

    // Descripción: limitar a maxW para no salirse del panel
    float maxDescW = (float)(cardW - 44);
    {
        Vector2 descSz = UI_MeasureTextMenu(bDef->description, 11.0f);
        if (descSz.x <= maxDescW) {
            UI_DrawTextMenu(bDef->description, (float)(cardX + 22), (float)(cardY + 98), 11.0f, textPrimary);
        } else {
            char line1[64]; char line2[64];
            snprintf(line1, 50, "%s", bDef->description);
            snprintf(line2, 64, "%s", bDef->description + 48);
            line1[49] = '\0';
            UI_DrawTextMenu(line1, (float)(cardX + 22), (float)(cardY + 98), 11.0f, textPrimary);
            UI_DrawTextMenu(line2, (float)(cardX + 22), (float)(cardY + 118), 11.0f, textPrimary);
        }
    }

    UI_DrawTextMenu(TextFormat("CEILING: %.0fm   WATER: %s", bDef->maxHeight, bDef->hasWater ? "YES" : "NO"),
                   (float)(cardX + 22), (float)(cardY + 156), 11.0f, textPrimary);
    UI_DrawTextMenu("CIRCUIT: 18 CHECKPOINTS [CROSS RAID]", (float)(cardX + 22), (float)(cardY + 176), 11.0f, UI_COLOR_AC4_GREEN);

    const HighscoreEntry *best = Records_GetBest(selectedBiome);
    if (best && best->isValid && best->finishTime > 0.0f) {
        int mins = (int)(best->finishTime / 60.0f);
        float secs = fmodf(best->finishTime, 60.0f);
        UI_DrawTextMenu(TextFormat("CIRCUIT RECORD: %02d:%05.2f [%s] %s",
                                   mins, secs, best->pilotTag, best->rank),
                        (float)(cardX + 22), (float)(cardY + 200), 11.0f, UI_COLOR_AC4_AMBER);
    } else {
        UI_DrawTextMenu("CIRCUIT RECORD: NO SORTIE LOGGED", (float)(cardX + 22), (float)(cardY + 200), 11.0f, UI_COLOR_MUTED_TEXT);
    }

    UI_DrawNavHelp(screenWidth, screenHeight, "[ENTER / SPACE / (A)]: PROCEED TO HOVERCRAFT BAY    [ESC / (B)]: BACK TO MAIN MENU");
}

// ============================================================================
// 3. SELECCIÓN DE HOVERCRAFT (AEROSHEAR RS-01 PHANTOM)
// ============================================================================
MenuAction Menu_UpdateAircraftSelect(int *selectedAircraftIdx) {
    if (IsKeyPressed(KEY_ESCAPE)) return MENU_ACTION_TO_MAP_SELECT;
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        return MENU_ACTION_TO_MAP_SELECT;
    }

    if (selectedAircraftIdx) *selectedAircraftIdx = 0; // RS-01 HCRB Spec

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        return MENU_ACTION_OPEN_SEED_SELECT;
    }
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        return MENU_ACTION_OPEN_SEED_SELECT;
    }

    return MENU_ACTION_NONE;
}

// ============================================================================
// 3.1 CONFIGURACIÓN DE SEED DE GENERACIÓN PROCEDURAL
// ============================================================================
MenuAction Menu_UpdateSeedSelect(char *seedBuffer, int maxLen, unsigned int *outSeed) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        return MENU_ACTION_TO_AIRCRAFT_SELECT;
    }

    // Procesamiento de entrada de texto alfanumérico para el seed
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= '0' && key <= '9') || (key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
            int len = (int)strlen(seedBuffer);
            if (len < maxLen - 1) {
                seedBuffer[len] = (char)toupper((unsigned char)key);
                seedBuffer[len + 1] = '\0';
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(seedBuffer);
        if (len > 0) {
            seedBuffer[len - 1] = '\0';
        }
    }

    // Lanzamiento directo con ENTER o barra espaciadora (si el buffer no está vacío)
    if (IsKeyPressed(KEY_ENTER)) {
        unsigned int s = 0;
        if (strlen(seedBuffer) > 0) {
            char *endPtr = NULL;
            unsigned long val = strtoul(seedBuffer, &endPtr, 10);
            if (endPtr && *endPtr == '\0' && val > 0) {
                s = (unsigned int)val;
            } else {
                s = 5381;
                for (int i = 0; seedBuffer[i] != '\0'; i++) {
                    s = ((s << 5) + s) + (unsigned char)seedBuffer[i];
                }
                s = (s % 900000) + 100000;
            }
        } else {
            // Si está vacío, generar seed procedural aleatorio
            s = (unsigned int)GetRandomValue(100000, 999999);
        }
        if (outSeed) *outSeed = s;
        return MENU_ACTION_START_GAME;
    }

    // Navegación con Gamepad
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            unsigned int s = 0;
            if (strlen(seedBuffer) > 0) {
                s = (unsigned int)strtoul(seedBuffer, NULL, 10);
                if (s == 0) s = (unsigned int)GetRandomValue(100000, 999999);
            } else {
                s = (unsigned int)GetRandomValue(100000, 999999);
            }
            if (outSeed) *outSeed = s;
            return MENU_ACTION_START_GAME;
        }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            return MENU_ACTION_TO_AIRCRAFT_SELECT;
        }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
            unsigned int rnd = (unsigned int)GetRandomValue(100000, 999999);
            snprintf(seedBuffer, maxLen, "%u", rnd);
        }
    }

    // Interacciones con mouse
    Vector2 mousePos = GetMousePosition();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int cardW = (int)fminf(700.0f, (float)screenW * 0.86f);
    int cardH = 390;
    int cardX = (screenW - cardW) / 2;
    int cardY = (screenH - cardH) / 2;

    int boxW = cardW - 70;
    int boxH = 50;
    int boxX = cardX + 35;
    int boxY = cardY + 108;

    int btnSubW = (boxW - 16) / 2;
    Rectangle rReroll = { (float)boxX, (float)(boxY + boxH + 12), (float)btnSubW, 34.0f };
    Rectangle rClear  = { (float)(boxX + btnSubW + 16), (float)(boxY + boxH + 12), (float)btnSubW, 34.0f };

    int btnH = 42;
    int btnY = cardY + cardH - 58;
    int btnW = (cardW - 70) / 2;
    Rectangle rBack   = { (float)(cardX + 35), (float)btnY, (float)(btnW - 10), (float)btnH };
    Rectangle rLaunch = { (float)(cardX + 35 + btnW + 10), (float)btnY, (float)(btnW - 10), (float)btnH };

    if (CheckCollisionPointRec(mousePos, rReroll) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        unsigned int rnd = (unsigned int)GetRandomValue(100000, 999999);
        snprintf(seedBuffer, maxLen, "%u", rnd);
    }
    if (CheckCollisionPointRec(mousePos, rClear) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        seedBuffer[0] = '\0';
    }
    if (CheckCollisionPointRec(mousePos, rBack) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return MENU_ACTION_TO_AIRCRAFT_SELECT;
    }
    if (CheckCollisionPointRec(mousePos, rLaunch) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        unsigned int s = 0;
        if (strlen(seedBuffer) > 0) {
            char *endPtr = NULL;
            unsigned long val = strtoul(seedBuffer, &endPtr, 10);
            if (endPtr && *endPtr == '\0' && val > 0) {
                s = (unsigned int)val;
            } else {
                s = 5381;
                for (int i = 0; seedBuffer[i] != '\0'; i++) {
                    s = ((s << 5) + s) + (unsigned char)seedBuffer[i];
                }
                s = (s % 900000) + 100000;
            }
        } else {
            s = (unsigned int)GetRandomValue(100000, 999999);
        }
        if (outSeed) *outSeed = s;
        return MENU_ACTION_START_GAME;
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawSeedSelect(int screenWidth, int screenHeight, const char *seedBuffer, BiomeType biome) {
    float time = (float)GetTime();
    const BiomeDefinition *bDef = Biome_Get(biome);
    Color accent = bDef ? bDef->themeAccentColor : UI_COLOR_AC4_CYAN;

    Vector2 mousePos = GetMousePosition();
    Vector2 targetParallax = {
        ((mousePos.x / (float)screenWidth) - 0.5f) * 2.0f,
        ((mousePos.y / (float)screenHeight) - 0.5f) * 2.0f
    };

    // Fondo reactivo táctico
    UI_DrawOrbitalLimbEx(screenWidth, screenHeight, time, targetParallax, accent);
    UI_DrawScreenHeader(screenWidth, "SORTIE PROTOCOL // MISSION SEED", "PROCEDURAL WORLD CONFIGURATION", "SEED GENERATOR // GEN.01");

    int cardW = (int)fminf(700.0f, (float)screenWidth * 0.86f);
    int cardH = 390;
    int cardX = (screenWidth - cardW) / 2;
    int cardY = (screenHeight - cardH) / 2;
    Rectangle cardRec = { (float)cardX, (float)cardY, (float)cardW, (float)cardH };

    UI_DrawGlassPanel(cardRec, "MISSION SEED SPECIFICATION // PROCEDURAL MATRIX", accent, UI_COLOR_PANEL_BG);

    UI_DrawTextTitle("PROCEDURAL RUN CONFIGURATION", (float)(cardX + 35), (float)(cardY + 28), 16.0f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextHud(TextFormat("THEATER: %s // 18-CHECKPOINT CIRCUIT GENERATION", bDef ? bDef->name : "DELTA STRAITS"),
                   (float)(cardX + 35), (float)(cardY + 52), 11.0f, accent);
    DrawLine(cardX + 35, cardY + 70, cardX + cardW - 35, cardY + 70, (Color){ 35, 75, 115, 140 });

    UI_DrawTextMenu("ENTER CUSTOM NUMERIC SEED OR LEAVE BLANK FOR RANDOM RUN:", (float)(cardX + 35), (float)(cardY + 86), 11.0f, UI_COLOR_STEEL_WHITE);

    // Input box
    int boxW = cardW - 70;
    int boxH = 50;
    int boxX = cardX + 35;
    int boxY = cardY + 108;
    Rectangle boxRec = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };

    DrawRectangleRounded(boxRec, 0.18f, 4, (Color){ 4, 14, 26, 240 });
    DrawRectangleRoundedLinesEx(boxRec, 0.18f, 4, 1.4f, accent);

    float blink = sinf(time * 8.0f) * 0.5f + 0.5f;

    if (seedBuffer && seedBuffer[0] != '\0') {
        const char *displayStr = TextFormat("SEED :  %s", seedBuffer);
        UI_DrawTextTitle(displayStr, (float)(boxX + 20), (float)(boxY + 14), 20.0f, (Color){ 255, 225, 100, 255 });
        Vector2 sSz = UI_MeasureTextTitle(displayStr, 20.0f);
        if (blink > 0.3f) {
            DrawRectangle((int)(boxX + 22 + sSz.x), (int)(boxY + 14), 10, 22, accent);
        }
    } else {
        UI_DrawTextMenu("[ RANDOM PROCEDURAL SEED // UNIQUE RUN GUARANTEED ]", (float)(boxX + 20), (float)(boxY + 17), 13.0f, (Color){ 100, 160, 200, 160 });
        if (blink > 0.3f) {
            DrawRectangle((int)(boxX + 20), (int)(boxY + 14), 8, 22, accent);
        }
    }

    // Botones auxiliares Reroll / Clear
    int btnSubW = (boxW - 16) / 2;
    Rectangle rReroll = { (float)boxX, (float)(boxY + boxH + 12), (float)btnSubW, 34.0f };
    Rectangle rClear  = { (float)(boxX + btnSubW + 16), (float)(boxY + boxH + 12), (float)btnSubW, 34.0f };

    bool hoverReroll = CheckCollisionPointRec(mousePos, rReroll);
    DrawRectangleRounded(rReroll, 0.2f, 4, hoverReroll ? (Color){ 16, 40, 68, 220 } : (Color){ 8, 20, 36, 170 });
    DrawRectangleRoundedLinesEx(rReroll, 0.2f, 4, 1.0f, hoverReroll ? UI_COLOR_AC4_CYAN : (Color){ 40, 80, 120, 140 });
    UI_DrawTextHud(">> GENERATE RANDOM SEED", rReroll.x + 18, rReroll.y + 10, 10.5f, hoverReroll ? UI_COLOR_STEEL_WHITE : UI_COLOR_AC4_CYAN);

    bool hoverClear = CheckCollisionPointRec(mousePos, rClear);
    DrawRectangleRounded(rClear, 0.2f, 4, hoverClear ? (Color){ 16, 40, 68, 220 } : (Color){ 8, 20, 36, 170 });
    DrawRectangleRoundedLinesEx(rClear, 0.2f, 4, 1.0f, hoverClear ? UI_COLOR_AC4_AMBER : (Color){ 40, 80, 120, 140 });
    UI_DrawTextHud("X  CLEAR (USE RANDOM)", rClear.x + 20, rClear.y + 10, 10.5f, hoverClear ? UI_COLOR_STEEL_WHITE : UI_COLOR_AC4_AMBER);

    // Caja informativa de protocolo
    int infoBoxY = (int)(rReroll.y + rReroll.height + 14);
    int infoBoxH = 82;
    Rectangle infoRec = { (float)boxX, (float)infoBoxY, (float)boxW, (float)infoBoxH };
    DrawRectangleRounded(infoRec, 0.12f, 4, (Color){ 6, 16, 28, 190 });
    DrawRectangleRoundedLinesEx(infoRec, 0.12f, 4, 1.0f, (Color){ 25, 55, 85, 120 });

    UI_DrawTextHud("* SEED TELEMETRY LOGGING:", infoRec.x + 16, infoRec.y + 10, 10.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextMenu("- Run seed is stamped on Wreck Debriefing & Race Debriefing cards upon conclusion.",
                    infoRec.x + 16, infoRec.y + 26, 10.5f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextMenu("- Re-enter this seed code anytime to duplicate the exact circuit topology & ruins layout.",
                    infoRec.x + 16, infoRec.y + 44, 10.5f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextMenu("- Highscore leaderboard archives the deployment seed next to every verified chrono.",
                    infoRec.x + 16, infoRec.y + 62, 10.5f, UI_COLOR_AC4_AMBER);

    // Botones inferiores de acción
    int btnH = 42;
    int btnY = cardY + cardH - 58;
    int btnW = (cardW - 70) / 2;
    Rectangle rBack   = { (float)(cardX + 35), (float)btnY, (float)(btnW - 10), (float)btnH };
    Rectangle rLaunch = { (float)(cardX + 35 + btnW + 10), (float)btnY, (float)(btnW - 10), (float)btnH };

    bool hoverBack = CheckCollisionPointRec(mousePos, rBack);
    DrawRectangleRounded(rBack, 0.22f, 4, hoverBack ? (Color){ 16, 36, 60, 220 } : (Color){ 6, 14, 26, 180 });
    DrawRectangleRoundedLinesEx(rBack, 0.22f, 4, 1.0f, hoverBack ? UI_COLOR_AC4_CYAN : (Color){ 35, 75, 115, 140 });
    UI_DrawTextMenu("< BACK TO HANGAR [ESC]", rBack.x + 24, rBack.y + 13, 12.0f, hoverBack ? UI_COLOR_STEEL_WHITE : UI_COLOR_MUTED_TEXT);

    bool hoverLaunch = CheckCollisionPointRec(mousePos, rLaunch);
    Color launchBg = hoverLaunch ? (Color){ 20, 56, 88, 240 } : (Color){ 10, 32, 54, 220 };
    DrawRectangleRounded(rLaunch, 0.22f, 4, launchBg);
    DrawRectangleRoundedLinesEx(rLaunch, 0.22f, 4, 1.4f, accent);
    UI_DrawTextTitle("[ENTER / (A)] LAUNCH SORTIE >", rLaunch.x + 20, rLaunch.y + 12, 13.0f, UI_COLOR_STEEL_WHITE);

    UI_DrawNavHelp(screenWidth, screenHeight, "[A-Z / 0-9]: TYPE SEED    [BACKSPACE]: DELETE    [ENTER / (A)]: LAUNCH SORTIE    [ESC / (B)]: BACK");
}

void Menu_DrawAircraftSelect(int screenWidth, int screenHeight, int selectedAircraftIdx, const SpriteSheet *previewSprite) {
    float time = (float)GetTime();

    // Fondo reactivo táctico en Dark Future Blue
    UI_DrawTacticalBackdropEx(screenWidth, screenHeight, time, (Color){ 70, 160, 230, 255 });
    UI_DrawScreenHeader(screenWidth, "HOVERCRAFT BAY // HCRB REQUISITION", "PRIMARY TRANSONIC HOVERCRAFT // HCRB SPEC", "RACING DIVISION // CRAFT: RS-01");

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int topY = (int)((float)screenHeight * 0.16f);

    const AircraftDefinition *jet = Aircraft_Get(selectedAircraftIdx);

    // Previsualización del Sprite en el centro-izquierda
    int previewBoxW = (int)fminf(430.0f, (float)screenWidth * 0.44f);
    int previewBoxH = 270;
    int previewX = leftX;
    int previewY = topY + 65;
    Rectangle pRec = { (float)previewX, (float)previewY, (float)previewBoxW, (float)previewBoxH };

    UI_DrawGlassPanel(pRec, "HOVERCRAFT BAY DOCK", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);

    if (previewSprite && previewSprite->isLoaded) {
        Rectangle src = {
            3.0f * previewSprite->frameWidth,
            2.0f * previewSprite->frameHeight,
            previewSprite->frameWidth,
            previewSprite->frameHeight
        };
        float scale = 0.95f;
        float dw = previewSprite->frameWidth * scale;
        float dh = previewSprite->frameHeight * scale;
        Rectangle dst = {
            (float)(previewX + (previewBoxW - dw) / 2),
            (float)(previewY + (previewBoxH - dh) / 2 + 8),
            dw, dh
        };
        DrawTexturePro(previewSprite->texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }

    // Panel de Estadísticas y Especificaciones (Derecha)
    int statsW = (int)fminf(480.0f, (float)screenWidth * 0.44f);
    int statsX = screenWidth - statsW - leftX;
    int statsY = topY + 65;
    int statsH = 270;
    Rectangle sRec = { (float)statsX, (float)statsY, (float)statsW, (float)statsH };

    UI_DrawGlassPanel(sRec, "PERFORMANCE SPECIFICATIONS", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);

    UI_DrawTextTitle(jet->name, (float)(statsX + 25), (float)(statsY + 28), 16.0f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextHud(jet->role, (float)(statsX + 25), (float)(statsY + 50), 10.0f, UI_COLOR_AC4_CYAN);
    DrawLine(statsX + 25, statsY + 68, statsX + statsW - 25, statsY + 68, (Color){ 35, 75, 115, 140 });

    // Barras de Estadísticas con pistas translúcidas y brillo líquido
    const char *statLabels[4] = { "SPEED", "MOBILITY", "STEALTH", "ARMOR" };
    int statValues[4] = { jet->statSpeed, jet->statMobility, jet->statStealth, jet->statArmor };

    int barStartY = statsY + 84;
    for (int s = 0; s < 4; s++) {
        UI_DrawTextHud(statLabels[s], (float)(statsX + 25), (float)(barStartY + s * 30), 11.0f, UI_COLOR_MUTED_TEXT);
        int barX = statsX + 115;
        int barW = statsW - 150;
        int barH = 10;

        DrawRectangleRounded((Rectangle){ (float)barX, (float)(barStartY + s * 30 + 2), (float)barW, (float)barH }, 0.4f, 4, (Color){ 8, 20, 34, 210 });
        DrawRectangleRoundedLinesEx((Rectangle){ (float)barX, (float)(barStartY + s * 30 + 2), (float)barW, (float)barH }, 0.4f, 4, 1.0f, (Color){ 35, 75, 115, 120 });

        int fillW = (int)((float)barW * ((float)statValues[s] / 10.0f));
        if (fillW > 4) {
            DrawRectangleRounded((Rectangle){ (float)barX, (float)(barStartY + s * 30 + 2), (float)fillW, (float)barH }, 0.4f, 4, UI_COLOR_AC4_CYAN);
            DrawLine(barX + 2, barStartY + s * 30 + 3, barX + fillW - 2, barStartY + s * 30 + 3, (Color){ 255, 255, 255, 130 });
        }
    }

    UI_DrawTextHud(TextFormat("CRUISE: %.0f M/S   AFTERBURNER: %.0f M/S", jet->cruiseSpeed, jet->afterburnerSpeed),
                   (float)(statsX + 25), (float)(statsY + statsH - 32), 10.0f, UI_COLOR_AC4_GREEN);

    UI_DrawNavHelp(screenWidth, screenHeight, "[ENTER / SPACE / (A)]: CONFIGURE MISSION SEED    [ESC / (B)]: BACK TO CIRCUIT SELECTION");
}

// ============================================================================
// ============================================================================
// 4. MENÚ DE OPCIONES (FULLSCREEN, INVERT PITCH, CRT, AUDIO, MUSIC, NADIA AI)
// ============================================================================
MenuAction Menu_UpdateSettings(GameSettings *settings) {
    const int totalOpts = 11;

    if (IsKeyPressed(KEY_ESCAPE)) return MENU_ACTION_TO_MAIN_MENU;
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        return MENU_ACTION_TO_MAIN_MENU;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_settingsSelection = (s_settingsSelection - 1 + totalOpts) % totalOpts;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_settingsSelection = (s_settingsSelection + 1) % totalOpts;
    }

    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) s_settingsSelection = (s_settingsSelection - 1 + totalOpts) % totalOpts;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) s_settingsSelection = (s_settingsSelection + 1) % totalOpts;
    }

    bool toggle = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT);
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) toggle = true;

    // Soporte para clics con el ratón
    Vector2 mousePos = GetMousePosition();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int leftX = (int)fmaxf(48.0f, (float)screenW * 0.085f);
    int topY = (int)((float)screenH * 0.16f);
    int startY = topY + 44;
    int spacing = 34;
    int btnW = (int)fminf(600.0f, (float)screenW * 0.62f);
    int rowH = 30;

    for (int i = 0; i < totalOpts; i++) {
        Rectangle r = { (float)leftX, (float)(startY + i * spacing), (float)btnW, (float)rowH };
        if (CheckCollisionPointRec(mousePos, r)) {
            s_settingsSelection = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                toggle = true;
            }
        }
    }

    if (toggle) {
        if (s_settingsSelection == 0) {
            settings->fullscreen = !settings->fullscreen;
            ToggleFullscreen();
        } else if (s_settingsSelection == 1) {
            settings->invertPitch = !settings->invertPitch;
        } else if (s_settingsSelection == 2) {
            settings->scanlinesEnabled = !settings->scanlinesEnabled;
        } else if (s_settingsSelection == 3) {
            settings->pixelFilterEnabled = !settings->pixelFilterEnabled;
        } else if (s_settingsSelection == 4) {
            settings->blueFilterEnabled = !settings->blueFilterEnabled;
        } else if (s_settingsSelection == 5) {
            int cur = (int)settings->hudTheme;
            cur = (cur + 1) % HUD_THEME_COUNT;
            settings->hudTheme = (HUDColorTheme)cur;
            UITheme_SetHUDTheme(settings->hudTheme);
        } else if (s_settingsSelection == 6) {
            bool goLeft = IsKeyPressed(KEY_LEFT) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
            if (goLeft) {
                settings->masterVolume -= 0.10f;
                if (settings->masterVolume < -0.01f) settings->masterVolume = 1.0f;
            } else {
                settings->masterVolume += 0.10f;
                if (settings->masterVolume > 1.01f) settings->masterVolume = 0.0f;
            }
            Audio_SetMasterVolume(settings->masterVolume);
            Audio_PlayCheckpoint();
        } else if (s_settingsSelection == 7) {
            bool goLeft = IsKeyPressed(KEY_LEFT) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
            if (goLeft) {
                settings->musicVolume -= 0.10f;
                if (settings->musicVolume < -0.01f) settings->musicVolume = 1.0f;
            } else {
                settings->musicVolume += 0.10f;
                if (settings->musicVolume > 1.01f) settings->musicVolume = 0.0f;
            }
            Music_SetVolume(settings->musicVolume);
        } else if (s_settingsSelection == 8) {
            bool goLeft = IsKeyPressed(KEY_LEFT) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
            if (goLeft) {
                settings->voiceVolume -= 0.10f;
                if (settings->voiceVolume < -0.01f) settings->voiceVolume = 1.0f;
            } else {
                settings->voiceVolume += 0.10f;
                if (settings->voiceVolume > 1.01f) settings->voiceVolume = 0.0f;
            }
            Audio_SetVoiceVolume(settings->voiceVolume);
            Audio_PlayNadia(NADIA_CALLOUT_PERFECT_GATE);
        } else if (s_settingsSelection == 9) {
            settings->nadiaEnabled = !settings->nadiaEnabled;
            Audio_SetNadiaEnabled(settings->nadiaEnabled);
            if (settings->nadiaEnabled) {
                Audio_PlayNadia(NADIA_CALLOUT_PERFECT_GATE);
            }
        } else if (s_settingsSelection == 10) {
            return MENU_ACTION_TO_MAIN_MENU;
        }
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawSettings(int screenWidth, int screenHeight, const GameSettings *settings) {
    float time = (float)GetTime();
    Vector2 mousePos = GetMousePosition();

    // Fondo reactivo táctico Dark Future Blue
    UI_DrawTacticalBackdropEx(screenWidth, screenHeight, time, (Color){ 65, 140, 215, 255 });
    UI_DrawScreenHeader(screenWidth, "SYSTEM CONFIG // AVIONICS", "FLIGHT CONTROLS, POST-FX AND MASTER AUDIO", "AVIONICS CONFIG // SYS.04");

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int topY = (int)((float)screenHeight * 0.16f);

    const char *labels[11] = {
        "DISPLAY MODE",
        "PITCH CONTROL AXIS",
        "CRT SCANLINES SHADER",
        "APERTURE PHOSPHOR MASK",
        "CINEMATIC BLUE GRADE",
        "HUD PALETTE THEME",
        "MASTER SFX VOLUME",
        "MUSIC STREAM VOLUME",
        "NADIA AI VOICE VOLUME",
        "NADIA AI COPILOT",
        "RETURN TO MAIN MENU"
    };

    const char *values[11];
    values[0] = IsWindowFullscreen() ? "FULLSCREEN" : "WINDOWED";
    values[1] = settings->invertPitch ? "INVERTED" : "NORMAL [W=CLIMB]";
    values[2] = settings->scanlinesEnabled ? "ON" : "OFF";
    values[3] = settings->pixelFilterEnabled ? "ON" : "OFF";
    values[4] = settings->blueFilterEnabled ? "ON" : "OFF";
    values[5] = UITheme_GetHUDThemeName(settings->hudTheme);
    char volBuffer[32];
    snprintf(volBuffer, sizeof(volBuffer), "%.0f%%", settings->masterVolume * 100.0f);
    values[6] = volBuffer;
    char musVolBuffer[32];
    snprintf(musVolBuffer, sizeof(musVolBuffer), "%.0f%%", settings->musicVolume * 100.0f);
    values[7] = musVolBuffer;
    char voiceVolBuffer[32];
    snprintf(voiceVolBuffer, sizeof(voiceVolBuffer), "%.0f%%", settings->voiceVolume * 100.0f);
    values[8] = voiceVolBuffer;
    values[9] = settings->nadiaEnabled ? "ENABLED" : "DISABLED";
    values[10] = "[ESC / (B)]";

    int startY = topY + 44;
    int spacing = 34;
    int btnW = (int)fminf(600.0f, (float)screenWidth * 0.62f);
    int rowH = 30;

    for (int i = 0; i < 11; i++) {
        bool isSel = (i == s_settingsSelection);
        Rectangle r = { (float)leftX, (float)(startY + i * spacing), (float)btnW, (float)rowH };
        UI_DrawMinimalOptionRow(r, labels[i], values[i], isSel, mousePos);
    }

    UI_DrawNavHelp(screenWidth, screenHeight, "[ENTER / (A)]: TOGGLE OPTION    [ESC / (B)]: RETURN TO MAIN MENU");
}

// ============================================================================
// ============================================================================
// 5. GUÍA COMPLETA DE CONTROLES & PROTOCOLOS DE VUELO (CONTROLS_)
// ============================================================================
static void DrawControlBindingRow(int colX, int rowY, const char *keyName, const char *actionDesc, Color keyColor) {
    // 1. Píldora de cristal oscuro táctico para el atajo/tecla
    Rectangle badgeRec = { (float)(colX + 18), (float)(rowY - 2), 178.0f, 22.0f };
    DrawRectangleRounded(badgeRec, 0.28f, 4, (Color){ 8, 20, 38, 200 });
    DrawRectangleRoundedLinesEx(badgeRec, 0.28f, 4, 1.0f, (Color){ keyColor.r, keyColor.g, keyColor.b, 90 });
    DrawLine((int)badgeRec.x + 8, (int)badgeRec.y, (int)(badgeRec.x + badgeRec.width - 8), (int)badgeRec.y, (Color){ 255, 255, 255, 60 });

    // Texto de tecla alineado dentro de su píldora con padding
    UI_DrawTextHud(keyName, (float)(colX + 26), (float)(rowY + 3), 10.0f, keyColor);

    // 2. Descripción de la acción claramente separada (inicia en colX + 214)
    UI_DrawTextMenu(actionDesc, (float)(colX + 214), (float)(rowY + 2), 12.0f, UI_COLOR_STEEL_WHITE);
}

MenuAction Menu_UpdateControls(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        return MENU_ACTION_TO_MAIN_MENU;
    }
    if (IsGamepadAvailable(0) && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
        return MENU_ACTION_TO_MAIN_MENU;
    }
    return MENU_ACTION_NONE;
}

void Menu_DrawControls(int screenWidth, int screenHeight) {
    float time = (float)GetTime();

    // Fondo reactivo táctico Dark Future Blue
    UI_DrawTacticalBackdropEx(screenWidth, screenHeight, time, (Color){ 50, 120, 200, 255 });
    UI_DrawScreenHeader(screenWidth, "FLIGHT CONTROLS // AVIONICS MANUAL", "TACTICAL FLIGHT PROTOCOLS FOR KEYBOARD & GAMEPAD", "AVIONICS // PROTOCOL.01");

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int topY = (int)((float)screenHeight * 0.16f);

    int colW = (screenWidth - leftX * 2 - 36) / 2;
    int col1X = leftX;
    int col2X = leftX + colW + 36;
    int cardY = topY + 65;
    int cardH = 330;

    // Columna 1: Teclado
    Rectangle r1 = { (float)col1X, (float)cardY, (float)colW, (float)cardH };
    UI_DrawGlassPanel(r1, "PRIMARY KEYBOARD FLIGHT BINDINGS", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);

    int rowY = cardY + 34;
    DrawControlBindingRow(col1X, rowY, "W / S or UP / DOWN", "Pitch Axis (Climb / Dive)", UI_COLOR_AC4_CYAN);

    rowY += 28;
    DrawControlBindingRow(col1X, rowY, "A / D or LEFT / RIGHT", "Roll / 360* Horizontal Turn", UI_COLOR_AC4_CYAN);

    rowY += 28;
    DrawControlBindingRow(col1X, rowY, "Q / E", "Independent Airbrakes / Drift", UI_COLOR_AC4_CYAN);

    rowY += 28;
    DrawControlBindingRow(col1X, rowY, "SPACE or L-SHIFT", "Afterburner Supersonic Boost", UI_COLOR_AC4_CYAN);

    rowY += 28;
    DrawControlBindingRow(col1X, rowY, "L-CTRL or C", "Aerodynamic Airbrake Decel", UI_COLOR_AC4_CYAN);

    rowY += 28;
    DrawControlBindingRow(col1X, rowY, "F11 or ALT+ENTER", "Instant Fullscreen Toggle", UI_COLOR_AC4_CYAN);

    rowY += 28;
    DrawControlBindingRow(col1X, rowY, "ESC or P", "Pause Tactical Flight Core", UI_COLOR_AC4_CYAN);

    int kbBottomY = cardY + 248;
    DrawLine(col1X + 18, kbBottomY, col1X + colW - 18, kbBottomY, (Color){ 35, 75, 115, 140 });
    UI_DrawTextHud("AVIONICS CALIBRATION // 60 HZ LOW-LATENCY INPUT BUS", (float)(col1X + 20), (float)(kbBottomY + 12), 10.0f, UI_COLOR_MUTED_TEXT);
    UI_DrawTextHud("SUPPORTS FULL DIGITAL / ANALOG SIMULTANEOUS INPUTS", (float)(col1X + 20), (float)(kbBottomY + 28), 10.0f, UI_COLOR_MUTED_TEXT);

    // Columna 2: Mando / Gamepad
    Rectangle r2 = { (float)col2X, (float)cardY, (float)colW, (float)cardH };
    UI_DrawGlassPanel(r2, "GAMEPAD / XINPUT FLIGHT MAPPINGS", UI_COLOR_AC4_GREEN, UI_COLOR_PANEL_BG);

    rowY = cardY + 34;
    DrawControlBindingRow(col2X, rowY, "LEFT ANALOG STICK", "Pitch & Roll Flight Vector", UI_COLOR_AC4_GREEN);

    rowY += 28;
    DrawControlBindingRow(col2X, rowY, "LT / RT (TRIGGERS)", "Left / Right Airbrakes (Drift)", UI_COLOR_AC4_GREEN);

    rowY += 28;
    DrawControlBindingRow(col2X, rowY, "BUTTON (A) / CROSS", "Afterburner Supersonic Boost", UI_COLOR_AC4_GREEN);

    rowY += 28;
    DrawControlBindingRow(col2X, rowY, "BUTTON (B) / CIRCLE", "Airbrake Deceleration", UI_COLOR_AC4_GREEN);

    rowY += 28;
    DrawControlBindingRow(col2X, rowY, "START / MENU", "Pause Tactical Flight Core", UI_COLOR_AC4_GREEN);

    int gpBottomY = cardY + 248;
    DrawLine(col2X + 18, gpBottomY, col2X + colW - 18, gpBottomY, (Color){ 35, 75, 115, 140 });
    UI_DrawTextHud("SYSTEM CORE: C99 / RAYLIB 5.0 // SUPER SCALER ENGINE", (float)(col2X + 20), (float)(gpBottomY + 12), 10.0f, UI_COLOR_MUTED_TEXT);
    UI_DrawTextHud("PoC Build 0.1 // SUB-8MB FOOTPRINT // HCRB PROTOCOL", (float)(col2X + 20), (float)(gpBottomY + 28), 10.0f, UI_COLOR_MUTED_TEXT);

    UI_DrawNavHelp(screenWidth, screenHeight, "[ESC / ENTER / (B)]: RETURN TO MAIN MENU");
}

// ============================================================================
// 6. MENÚ DE PAUSA TÁCTICO SUSPENDIDO
// ============================================================================
MenuAction Menu_UpdatePauseMenu(void) {
    const int totalOpts = 4;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) return MENU_ACTION_RESUME_GAME;
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) return MENU_ACTION_RESUME_GAME;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) s_pauseSelection = (s_pauseSelection - 1 + totalOpts) % totalOpts;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) s_pauseSelection = (s_pauseSelection + 1) % totalOpts;

    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) s_pauseSelection = (s_pauseSelection - 1 + totalOpts) % totalOpts;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) s_pauseSelection = (s_pauseSelection + 1) % totalOpts;
    }

    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) confirm = true;

    if (confirm) {
        if (s_pauseSelection == 0) return MENU_ACTION_RESUME_GAME;
        if (s_pauseSelection == 1) return MENU_ACTION_RESTART_GAME;
        if (s_pauseSelection == 2) return MENU_ACTION_TO_MAIN_MENU;
        if (s_pauseSelection == 3) return MENU_ACTION_EXIT_APP;
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawPauseMenu(int screenWidth, int screenHeight) {
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 2, 6, 14, 215 });

    Vector2 mousePos = GetMousePosition();
    int cardW = 380;
    int cardH = 270;
    int cardX = (screenWidth - cardW) / 2;
    int cardY = (screenHeight - cardH) / 2;
    Rectangle pRec = { (float)cardX, (float)cardY, (float)cardW, (float)cardH };

    UI_DrawGlassPanel(pRec, "TACTICAL FLIGHT PAUSE", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);

    const char *opts[4] = {
        "RESUME SORTIE",
        "RESTART MISSION",
        "ABORT TO MAIN MENU",
        "EXIT APPLICATION"
    };

    int startY = cardY + 45;
    int spacing = 48;

    for (int i = 0; i < 4; i++) {
        bool isSel = (i == s_pauseSelection);
        Rectangle r = { (float)(cardX + 25), (float)(startY + i * spacing), (float)(cardW - 50), 38.0f };
        UI_DrawMenuButton(r, opts[i], isSel, mousePos);
    }
}

// ============================================================================
// 7. PANTALLA DE WRECK DEBRIEFING (MUERTE CÍNICA EN INGLÉS)
// ============================================================================

static int s_wreckSelection = 0;

static const char *s_cynicalDeathQuotes[] = {
    "TRANSONIC RACING IS NOT FOR EVERYONE. MOST END UP AS RECYCLABLE SCRAP.",
    "KINETIC ENERGY ALWAYS PREVAILS. YOU NEVER STOOD A CHANCE.",
    "ANOTHER PILOT CONTRACT LIQUIDATED. THE SYNDICATE SENDS ITS WARMEST REGARDS.",
    "GRAVITY AND CONCRETE: THE ONLY TRUE CONSTANTS IN THIS SECTOR.",
    "BLACK BOX TELEMETRY TERMINATED. CLEANUP DRONES DISPATCHED.",
    "SPEED NEVER KILLS ANYONE. SUDDENLY BECOMING STATIONARY DOES.",
    "YOUR LIFE INSURANCE POLICY DOES NOT COVER IMPACT VELOCITIES OVER MACH 1.",
    "TACTICAL DEBRIEF: HULL INTEGRITY INSUFFICIENT AGAINST IMMOVABLE GEOMETRY."
};
static const int s_cynicalDeathQuoteCount = sizeof(s_cynicalDeathQuotes) / sizeof(s_cynicalDeathQuotes[0]);

static void DrawWrappedQuote(const char *text, float posX, float posY, float maxW, float fontSize, Color color) {
    if (!text || text[0] == '\0') return;

    char lineBuf[256];
    lineBuf[0] = '\0';
    float currentY = posY;
    const char *p = text;

    while (*p != '\0') {
        char word[64];
        int wLen = 0;
        while (*p != '\0' && *p != ' ' && wLen < 60) {
            word[wLen++] = *p++;
        }
        word[wLen] = '\0';
        if (*p == ' ') p++;

        char testLine[256];
        if (lineBuf[0] == '\0') {
            snprintf(testLine, sizeof(testLine), "%s", word);
        } else {
            snprintf(testLine, sizeof(testLine), "%s %s", lineBuf, word);
        }

        Vector2 sz = UI_MeasureTextMenu(testLine, fontSize);
        if (sz.x > maxW && lineBuf[0] != '\0') {
            UI_DrawTextMenu(lineBuf, posX, currentY, fontSize, color);
            currentY += fontSize + 5.0f;
            snprintf(lineBuf, sizeof(lineBuf), "%s", word);
        } else {
            snprintf(lineBuf, sizeof(lineBuf), "%s", testLine);
        }
    }

    if (lineBuf[0] != '\0') {
        UI_DrawTextMenu(lineBuf, posX, currentY, fontSize, color);
    }
}

MenuAction Menu_UpdateWreckDebriefing(void) {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        s_wreckSelection = (s_wreckSelection == 0) ? 1 : 0;
    }
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
            s_wreckSelection = (s_wreckSelection == 0) ? 1 : 0;
        }
    }

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        return (s_wreckSelection == 0) ? MENU_ACTION_RESTART_GAME : MENU_ACTION_TO_MAIN_MENU;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        return MENU_ACTION_TO_MAIN_MENU;
    }
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            return (s_wreckSelection == 0) ? MENU_ACTION_RESTART_GAME : MENU_ACTION_TO_MAIN_MENU;
        }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            return MENU_ACTION_TO_MAIN_MENU;
        }
    }

    Vector2 mousePos = GetMousePosition();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int cardW = (int)fminf(720.0f, (float)screenW * 0.86f);
    int cardH = 340;
    int cardX = (screenW - cardW) / 2;
    int cardY = (screenH - cardH) / 2;

    int btnW = (cardW - 68) / 2;
    int btnH = 42;
    int btnY = cardY + cardH - 62;
    Rectangle rRestart = { (float)(cardX + 24), (float)btnY, (float)btnW, (float)btnH };
    Rectangle rMenu    = { (float)(cardX + 24 + btnW + 20), (float)btnY, (float)btnW, (float)btnH };

    if (CheckCollisionPointRec(mousePos, rRestart)) {
        s_wreckSelection = 0;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return MENU_ACTION_RESTART_GAME;
    }
    if (CheckCollisionPointRec(mousePos, rMenu)) {
        s_wreckSelection = 1;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return MENU_ACTION_TO_MAIN_MENU;
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawWreckDebriefing(int screenWidth, int screenHeight, const PlayerJet *player) {
    // Fondo oscuro de emergencia militar
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 8, 2, 4, 230 });

    Vector2 mousePos = GetMousePosition();
    int cardW = (int)fminf(720.0f, (float)screenWidth * 0.86f);
    int cardH = 340;
    int cardX = (screenWidth - cardW) / 2;
    int cardY = (screenHeight - cardH) / 2;
    Rectangle cardRec = { (float)cardX, (float)cardY, (float)cardW, (float)cardH };

    // Sombra suave de elevación
    DrawRectangleRounded((Rectangle){ cardRec.x, cardRec.y + 4, cardRec.width, cardRec.height }, 0.12f, 6, (Color){ 0, 0, 0, 180 });

    // Panel de cristal ahumado tDR con borde carmesí de alerta
    DrawRectangleRounded(cardRec, 0.12f, 6, (Color){ 16, 5, 8, 245 });
    DrawRectangleRoundedLinesEx(cardRec, 0.12f, 6, 1.2f, (Color){ 255, 45, 55, 200 });

    // Línea de brillo superior
    DrawLine(cardX + 16, cardY + 1, cardX + cardW - 16, cardY + 1, (Color){ 255, 120, 120, 180 });

    // Barra de cabecera militar
    Rectangle hdrRec = { (float)cardX + 2, (float)cardY + 2, (float)cardW - 4, 32.0f };
    DrawRectangleRec(hdrRec, (Color){ 38, 8, 12, 230 });
    DrawLine(cardX + 2, cardY + 34, cardX + cardW - 2, cardY + 34, (Color){ 255, 50, 60, 180 });
    UI_DrawTextHud("AEROSHEAR::NX // FLIGHT RECORDER TELEMETRY TERMINATED", (float)(cardX + 18), (float)(cardY + 10), 10.5f, (Color){ 255, 130, 130, 255 });

    unsigned int deathSeed = (player && player->runSeed > 0) ? player->runSeed : 0;
    UI_DrawTextHud(TextFormat("MISSION SEED : %06u", deathSeed), (float)(cardX + cardW - 200), (float)(cardY + 10), 10.5f, (Color){ 255, 205, 50, 255 });

    // Título de la catástrofe
    UI_DrawTextTitle("CRITICAL FAILURE // HULL DESTROYED", (float)(cardX + 24), (float)(cardY + 48), 16.0f, (Color){ 255, 60, 70, 255 });

    // Causa del impacto y seed code
    const char *reason = (player && player->fatalReason && player->fatalReason[0] != '\0') ? player->fatalReason : "HIGH VELOCITY GEOMETRY IMPACT";
    UI_DrawTextHud(TextFormat("FATAL EVENT : %s", reason), (float)(cardX + 24), (float)(cardY + 76), 11.0f, (Color){ 245, 200, 200, 210 });
    UI_DrawTextHud(TextFormat("SEED: %06u", deathSeed), (float)(cardX + cardW - 135), (float)(cardY + 76), 11.0f, (Color){ 255, 205, 50, 240 });

    // Caja de cita cínica (garantizada contra cualquier desbordamiento de texto)
    float qBoxX = (float)cardX + 24.0f;
    float qBoxY = (float)cardY + 104.0f;
    float qBoxW = (float)cardW - 48.0f;
    float qBoxH = 104.0f;
    Rectangle qRec = { qBoxX, qBoxY, qBoxW, qBoxH };

    DrawRectangleRounded(qRec, 0.15f, 4, (Color){ 26, 8, 12, 210 });
    DrawRectangleRoundedLinesEx(qRec, 0.15f, 4, 1.0f, (Color){ 160, 35, 45, 150 });

    UI_DrawTextHud("SYNDICATE AUTOMATED FLIGHT RECORDER LOG:", qBoxX + 14.0f, qBoxY + 10.0f, 9.5f, (Color){ 255, 110, 110, 200 });

    int qIdx = (player && player->deathQuoteIndex >= 0 && player->deathQuoteIndex < s_cynicalDeathQuoteCount)
               ? player->deathQuoteIndex
               : 0;
    const char *quote = s_cynicalDeathQuotes[qIdx];

    DrawWrappedQuote(quote, qBoxX + 14.0f, qBoxY + 34.0f, qBoxW - 28.0f, 11.5f, (Color){ 255, 230, 230, 240 });

    // Botones de acción inferiores
    int btnW = (cardW - 68) / 2;
    int btnH = 42;
    int btnY = cardY + cardH - 62;
    Rectangle rRestart = { (float)(cardX + 24), (float)btnY, (float)btnW, (float)btnH };
    Rectangle rMenu    = { (float)(cardX + 24 + btnW + 20), (float)btnY, (float)btnW, (float)btnH };

    UI_DrawMenuButton(rRestart, "[SPACE / (A)] RE-ENGAGE", (s_wreckSelection == 0), mousePos);
    UI_DrawMenuButton(rMenu, "[ESC / (B)] TO HANGAR", (s_wreckSelection == 1), mousePos);
}

void Menu_Unload(void) {
    // Recursos dinámicos
}
