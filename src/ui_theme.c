#include "ui_theme.h"
#include "config.h"
#include <stddef.h>

static UITheme s_activeTheme;

// Cuatro tipografías especializadas del sistema (cero fuentes genéricas / cero futuristic)
static Font s_fontLogo  = { 0 }; // Neuropolitical: Logo del juego (Aeroshear::NX)
static Font s_fontHud   = { 0 }; // WO3: In-Flight HUD, Telemetría, Altitudes, Velocidades
static Font s_fontTitle = { 0 }; // 2097: Alertas, Avisos y Títulos de Menú
static Font s_fontMenu  = { 0 }; // Fusion: Menús de Selección, Estadísticas, Descripciones

static bool s_isFontLogoLoaded  = false;
static bool s_isFontHudLoaded   = false;
static bool s_isFontTitleLoaded = false;
static bool s_isFontMenuLoaded  = false;

static void InitSingleFont(Font *targetFont, bool *loadedFlag, const char *path, int baseSize) {
    if (path && FileExists(path)) {
        *targetFont = LoadFontEx(path, baseSize, NULL, 0);
        if (targetFont->texture.id > 0) {
            SetTextureFilter(targetFont->texture, TEXTURE_FILTER_BILINEAR);
            *loadedFlag = true;
            TraceLog(LOG_INFO, "[FONT] Loaded: %s (%d px, bilinear filtered)", path, baseSize);
            return;
        }
    }
    *targetFont = GetFontDefault();
    *loadedFlag = false;
    TraceLog(LOG_WARNING, "[FONT] Fallback to default for: %s", path ? path : "NULL");
}

static HUDColorTheme s_currentHudTheme = HUD_THEME_GREEN;

void UITheme_SetHUDTheme(HUDColorTheme theme) {
    s_currentHudTheme = theme;
    switch (theme) {
        case HUD_THEME_GREEN:
            s_activeTheme.hudPrimary = UI_COLOR_AC4_GREEN;
            s_activeTheme.hudDim     = UI_COLOR_AC4_GREEN_DIM;
            s_activeTheme.hudWarning = UI_COLOR_AC4_AMBER;
            s_activeTheme.hudAlert   = UI_COLOR_AC4_ALERT;
            break;
        case HUD_THEME_CYAN:
            s_activeTheme.hudPrimary = UI_COLOR_AC4_CYAN;
            s_activeTheme.hudDim     = UI_COLOR_AC4_CYAN_DIM;
            s_activeTheme.hudWarning = UI_COLOR_AC4_AMBER;
            s_activeTheme.hudAlert   = UI_COLOR_AC4_ALERT;
            break;
        case HUD_THEME_AMBER:
            s_activeTheme.hudPrimary = UI_COLOR_AC4_AMBER;
            s_activeTheme.hudDim     = (Color){ 190, 125, 35, 160 };
            s_activeTheme.hudWarning = UI_COLOR_AC4_CYAN;
            s_activeTheme.hudAlert   = UI_COLOR_AC4_ALERT;
            break;
        default:
            break;
    }
}

HUDColorTheme UITheme_GetHUDTheme(void) {
    return s_currentHudTheme;
}

const char *UITheme_GetHUDThemeName(HUDColorTheme theme) {
    switch (theme) {
        case HUD_THEME_GREEN: return "PHOSPHOR GREEN";
        case HUD_THEME_CYAN:  return "ICE CYAN";
        case HUD_THEME_AMBER: return "TACTICAL AMBER";
        default:              return "UNKNOWN";
    }
}

void UITheme_Init(void) {
    s_activeTheme.bgVoid         = UI_COLOR_VOID_BLACK;
    s_activeTheme.panelBg        = UI_COLOR_PANEL_BG;
    s_activeTheme.borderActive   = UI_COLOR_PANEL_HIGHLIGHT;
    s_activeTheme.borderDim      = UI_COLOR_PANEL_BORDER;
    s_activeTheme.textPrimary    = UI_COLOR_STEEL_WHITE;
    s_activeTheme.textSecondary  = UI_COLOR_AC4_CYAN;
    s_activeTheme.textAccent     = UI_COLOR_AC4_AMBER;
    s_activeTheme.textMuted      = UI_COLOR_MUTED_TEXT;
    s_activeTheme.uiScale        = 1.0f;
    s_activeTheme.isMetric       = true;
    s_activeTheme.enableBeatPumping = true;

    UITheme_SetHUDTheme(HUD_THEME_GREEN);

    // 1. Neuropolitical: Logo del juego (Aeroshear::NX) a 64px
    InitSingleFont(&s_fontLogo, &s_isFontLogoLoaded, PATH_FONT_LOGO, 64);

    // 2. WO3: In-flight HUD, altitudes, velocidades, rumbos a 64px
    InitSingleFont(&s_fontHud, &s_isFontHudLoaded, PATH_FONT_HUD, 64);

    // 3. 2097: Alertas y títulos de menú a 64px
    InitSingleFont(&s_fontTitle, &s_isFontTitleLoaded, PATH_FONT_TITLE, 64);

    // 4. Fusion: Menús de elección, stats, specs, lore a 64px
    InitSingleFont(&s_fontMenu, &s_isFontMenuLoaded, PATH_FONT_MENU, 64);
}

void UITheme_Unload(void) {
    if (s_isFontLogoLoaded && s_fontLogo.texture.id > 0) {
        UnloadFont(s_fontLogo);
        s_isFontLogoLoaded = false;
    }
    if (s_isFontHudLoaded && s_fontHud.texture.id > 0) {
        UnloadFont(s_fontHud);
        s_isFontHudLoaded = false;
    }
    if (s_isFontTitleLoaded && s_fontTitle.texture.id > 0) {
        UnloadFont(s_fontTitle);
        s_isFontTitleLoaded = false;
    }
    if (s_isFontMenuLoaded && s_fontMenu.texture.id > 0) {
        UnloadFont(s_fontMenu);
        s_isFontMenuLoaded = false;
    }
}

UITheme* UITheme_Get(void) {
    return &s_activeTheme;
}

Font UI_GetTypedFont(UIFontType type) {
    switch (type) {
        case UI_FONT_LOGO:  return s_fontLogo;
        case UI_FONT_HUD:   return s_fontHud;
        case UI_FONT_TITLE: return s_fontTitle;
        case UI_FONT_MENU:
        default:            return s_fontMenu;
    }
}

Font UI_GetFont(void) {
    return s_fontMenu; // Fusion por defecto
}

// ============================================================================
// DIBUJADO GENERAL (FUSION POR DEFECTO: MENÚS, STATS, INFO VARIA)
// ============================================================================

void UI_DrawText(const char *text, float x, float y, float size, Color color) {
    if (!text || text[0] == '\0') return;
    float defaultSpacing = (size > 18.0f) ? 1.5f : 1.0f;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontMenu, text, pos, size, defaultSpacing, color);
}

void UI_DrawTextSpacing(const char *text, float x, float y, float size, float spacing, Color color) {
    if (!text || text[0] == '\0') return;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontMenu, text, pos, size, spacing, color);
}

Vector2 UI_MeasureText(const char *text, float size) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    float defaultSpacing = (size > 18.0f) ? 1.5f : 1.0f;
    return MeasureTextEx(s_fontMenu, text, size, defaultSpacing);
}

Vector2 UI_MeasureTextSpacing(const char *text, float size, float spacing) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    return MeasureTextEx(s_fontMenu, text, size, spacing);
}

// ============================================================================
// HUD E INFO EN VUELO (WO3)
// ============================================================================

void UI_DrawTextHud(const char *text, float x, float y, float size, Color color) {
    if (!text || text[0] == '\0') return;
    float defaultSpacing = (size > 18.0f) ? 1.5f : 1.0f;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontHud, text, pos, size, defaultSpacing, color);
}

void UI_DrawTextHudSpacing(const char *text, float x, float y, float size, float spacing, Color color) {
    if (!text || text[0] == '\0') return;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontHud, text, pos, size, spacing, color);
}

Vector2 UI_MeasureTextHud(const char *text, float size) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    float defaultSpacing = (size > 18.0f) ? 1.5f : 1.0f;
    return MeasureTextEx(s_fontHud, text, size, defaultSpacing);
}

Vector2 UI_MeasureTextHudSpacing(const char *text, float size, float spacing) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    return MeasureTextEx(s_fontHud, text, size, spacing);
}

// ============================================================================
// ALERTAS Y TÍTULOS DE MENÚ (2097)
// ============================================================================

void UI_DrawTextTitle(const char *text, float x, float y, float size, Color color) {
    if (!text || text[0] == '\0') return;
    float defaultSpacing = 1.5f;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontTitle, text, pos, size, defaultSpacing, color);
}

void UI_DrawTextTitleSpacing(const char *text, float x, float y, float size, float spacing, Color color) {
    if (!text || text[0] == '\0') return;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontTitle, text, pos, size, spacing, color);
}

Vector2 UI_MeasureTextTitle(const char *text, float size) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    return MeasureTextEx(s_fontTitle, text, size, 1.5f);
}

Vector2 UI_MeasureTextTitleSpacing(const char *text, float size, float spacing) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    return MeasureTextEx(s_fontTitle, text, size, spacing);
}

// ============================================================================
// MENÚS DE ELECCIÓN, STATS E INFO VARIA (FUSION)
// ============================================================================

void UI_DrawTextMenu(const char *text, float x, float y, float size, Color color) {
    UI_DrawText(text, x, y, size, color);
}

void UI_DrawTextMenuSpacing(const char *text, float x, float y, float size, float spacing, Color color) {
    UI_DrawTextSpacing(text, x, y, size, spacing, color);
}

Vector2 UI_MeasureTextMenu(const char *text, float size) {
    return UI_MeasureText(text, size);
}

Vector2 UI_MeasureTextMenuSpacing(const char *text, float size, float spacing) {
    return UI_MeasureTextSpacing(text, size, spacing);
}

// ============================================================================
// LOGO Y MARCA DEL JUEGO (NEUROPOLITICAL)
// ============================================================================

void UI_DrawTextLogo(const char *text, float x, float y, float size, Color color) {
    if (!text || text[0] == '\0') return;
    float defaultSpacing = 2.0f;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontLogo, text, pos, size, defaultSpacing, color);
}

void UI_DrawTextLogoSpacing(const char *text, float x, float y, float size, float spacing, Color color) {
    if (!text || text[0] == '\0') return;
    Vector2 pos = { x, y };
    DrawTextEx(s_fontLogo, text, pos, size, spacing, color);
}

Vector2 UI_MeasureTextLogo(const char *text, float size) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    return MeasureTextEx(s_fontLogo, text, size, 2.0f);
}

Vector2 UI_MeasureTextLogoSpacing(const char *text, float size, float spacing) {
    if (!text || text[0] == '\0') return (Vector2){ 0.0f, 0.0f };
    return MeasureTextEx(s_fontLogo, text, size, spacing);
}
