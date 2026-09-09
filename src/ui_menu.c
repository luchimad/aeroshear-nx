#include "ui_menu.h"
#include "config.h"
#include "biome.h"
#include "audio.h"
#include "records.h"
#include "raymath.h"
#include <stdio.h>
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
// 1. MENÚ PRINCIPAL MINIMALISTA (ENGAGE_, OPTIONS_, ABOUT_, EXIT_)
// ============================================================================
MenuAction Menu_UpdateMainMenu(void) {
    const int totalOptions = 4;

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
    int btnH = 44;
    int startY = (int)((float)screenH * 0.44f);
    int spacing = 56;

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
        if (s_mainMenuSelection == 1) return MENU_ACTION_OPEN_SETTINGS;
        if (s_mainMenuSelection == 2) return MENU_ACTION_OPEN_CONTROLS;
        if (s_mainMenuSelection == 3) return MENU_ACTION_EXIT_APP;
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
    int titleY = (int)((float)screenHeight * 0.18f);

    // 3. Logo oficial en Neuropolitical y subtítulos con espaciado respirable
    UI_DrawTitleAeroshear(leftX, titleY, 1.0f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextTitle("FLIGHT CORE // PUBLIC ALPHA EDITION", (float)leftX, (float)(titleY + 56), 15.0f, UI_COLOR_AC4_CYAN);
    UI_DrawTextHud("STANDALONE BUILD v0.1.0 // 60 FPS DETERMINISTIC CORE", (float)leftX, (float)(titleY + 80), 11.0f, UI_COLOR_MUTED_TEXT);

    DrawLine(leftX, titleY + 104, leftX + 440, titleY + 104, (Color){ 45, 95, 145, 140 });

    const char *options[4] = { "ENGAGE_", "OPTIONS_", "CONTROLS_", "EXIT_" };

    int startY = (int)((float)screenHeight * 0.44f);
    int spacing = 58;
    int btnW = (int)fminf(320.0f, (float)screenWidth * 0.35f);
    int btnH = 46;

    for (int i = 0; i < 4; i++) {
        Rectangle btnRec = { (float)leftX, (float)(startY + i * spacing), (float)btnW, (float)btnH };
        bool isSelected = (i == s_mainMenuSelection);
        UI_DrawMenuButton(btnRec, options[i], isSelected, mousePos);
    }

    UI_DrawNavHelp(screenWidth, screenHeight, "[W/S / UP/DN / D-PAD]: NAVIGATE    [ENTER / SPACE / (A)]: SELECT    [F11 / ALT+ENTER]: FULLSCREEN");
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

    const CircuitRecord *rec = Records_Get(selectedBiome);
    if (rec && rec->hasRecord) {
        int rSec = (int)rec->bestTime;
        int rMin = rSec / 60;
        int rS   = rSec % 60;
        int rC   = (int)((rec->bestTime - (float)rSec) * 100.0f);
        UI_DrawTextMenu(TextFormat("CIRCUIT RECORD: %02d:%02d.%02d [%s]",
                                   rMin, rS, rC, rec->rank),
                       (float)(cardX + 22), (float)(cardY + 200), 11.0f, UI_COLOR_AC4_AMBER);
    } else {
        UI_DrawTextMenu("CIRCUIT RECORD: NO SORTIE LOGGED", (float)(cardX + 22), (float)(cardY + 200), 11.0f, UI_COLOR_MUTED_TEXT);
    }

    UI_DrawNavHelp(screenWidth, screenHeight, "[ENTER / SPACE / (A)]: PROCEED TO HANGAR    [ESC / (B)]: BACK TO MAIN MENU");
}

// ============================================================================
// 3. SELECCIÓN DE VEHÍCULO (AEROSHEAR RS-01 PHANTOM)
// ============================================================================
MenuAction Menu_UpdateAircraftSelect(int *selectedAircraftIdx) {
    if (IsKeyPressed(KEY_ESCAPE)) return MENU_ACTION_TO_MAP_SELECT;
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        return MENU_ACTION_TO_MAP_SELECT;
    }

    if (selectedAircraftIdx) *selectedAircraftIdx = 0; // Solo RS-01 para Alpha

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        return MENU_ACTION_START_GAME;
    }
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        return MENU_ACTION_START_GAME;
    }

    return MENU_ACTION_NONE;
}

void Menu_DrawAircraftSelect(int screenWidth, int screenHeight, int selectedAircraftIdx, const SpriteSheet *previewSprite) {
    float time = (float)GetTime();

    // Fondo reactivo táctico en Dark Future Blue
    UI_DrawTacticalBackdropEx(screenWidth, screenHeight, time, (Color){ 70, 160, 230, 255 });
    UI_DrawScreenHeader(screenWidth, "AIRCRAFT HANGAR // FLIGHT REQUISITION", "PRIMARY TRANSONIC AIRFRAME FOR EVALUATION", "AVIONICS BAY // AIRFRAME: RS-01");

    int leftX = (int)fmaxf(48.0f, (float)screenWidth * 0.085f);
    int topY = (int)((float)screenHeight * 0.16f);

    const AircraftDefinition *jet = Aircraft_Get(selectedAircraftIdx);

    // Previsualización del Sprite en el centro-izquierda
    int previewBoxW = (int)fminf(430.0f, (float)screenWidth * 0.44f);
    int previewBoxH = 270;
    int previewX = leftX;
    int previewY = topY + 65;
    Rectangle pRec = { (float)previewX, (float)previewY, (float)previewBoxW, (float)previewBoxH };

    UI_DrawGlassPanel(pRec, "AIRFRAME HANGAR DOCK", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);

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

    UI_DrawNavHelp(screenWidth, screenHeight, "[ENTER / SPACE / (A)]: COMMENCE FLIGHT SORTIE    [ESC / (B)]: BACK TO MAP SELECTION");
}

// ============================================================================
// 4. MENÚ DE OPCIONES (FULLSCREEN, INVERT PITCH, CRT, AUDIO)
// ============================================================================
MenuAction Menu_UpdateSettings(GameSettings *settings) {
    const int totalOpts = 8;

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
            settings->masterVolume += 0.20f;
            if (settings->masterVolume > 1.05f) settings->masterVolume = 0.0f;
            Audio_SetMasterVolume(settings->masterVolume);
        } else if (s_settingsSelection == 7) {
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

    const char *labels[8] = {
        "DISPLAY MODE",
        "PITCH CONTROL AXIS",
        "CRT SCANLINES SHADER",
        "APERTURE PHOSPHOR MASK",
        "CINEMATIC BLUE GRADE",
        "HUD PALETTE THEME",
        "MASTER AUDIO VOLUME",
        "RETURN TO MAIN MENU"
    };

    const char *values[8];
    values[0] = IsWindowFullscreen() ? "FULLSCREEN" : "WINDOWED";
    values[1] = settings->invertPitch ? "INVERTED" : "NORMAL [W=CLIMB]";
    values[2] = settings->scanlinesEnabled ? "ON" : "OFF";
    values[3] = settings->pixelFilterEnabled ? "ON" : "OFF";
    values[4] = settings->blueFilterEnabled ? "ON" : "OFF";
    values[5] = UITheme_GetHUDThemeName(settings->hudTheme);
    char volBuffer[32];
    snprintf(volBuffer, sizeof(volBuffer), "%.0f%%", settings->masterVolume * 100.0f);
    values[6] = volBuffer;
    values[7] = "[ESC / (B)]";

    int startY = topY + 65;
    int spacing = 44;
    int btnW = (int)fminf(600.0f, (float)screenWidth * 0.62f);
    int rowH = 38;

    for (int i = 0; i < 8; i++) {
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
    UI_DrawTextHud("STANDALONE PUBLIC ALPHA v0.1.0 // SUB-8MB FOOTPRINT", (float)(col2X + 20), (float)(gpBottomY + 28), 10.0f, UI_COLOR_MUTED_TEXT);

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

void Menu_Unload(void) {
    // Recursos dinámicos
}
