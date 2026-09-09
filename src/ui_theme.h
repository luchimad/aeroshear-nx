#ifndef UI_THEME_H
#define UI_THEME_H

#include "raylib.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // DESIGN TOKENS, FUTURISTIC TYPOGRAPHY & THEME SYSTEM
// ============================================================================

// --- Paleta de Color: Espacio Profundo / Orbital Atmosférico ---
#define UI_COLOR_VOID_BLACK       ((Color){ 2, 4, 8, 255 })
#define UI_COLOR_DEEP_NAVY        ((Color){ 5, 12, 22, 255 })
#define UI_COLOR_PANEL_BG         ((Color){ 4, 10, 18, 220 })
#define UI_COLOR_PANEL_GLASS      ((Color){ 8, 18, 32, 190 })
#define UI_COLOR_PANEL_BORDER     ((Color){ 24, 60, 90, 160 })
#define UI_COLOR_PANEL_HIGHLIGHT  ((Color){ 75, 180, 235, 255 })

// --- Brillo Atmosférico Orbital (Curva Planeta) ---
#define UI_COLOR_ORBITAL_WHITE    ((Color){ 235, 245, 255, 255 })
#define UI_COLOR_ORBITAL_CYAN     ((Color){ 82, 175, 240, 255 })
#define UI_COLOR_ORBITAL_BLUE     ((Color){ 24, 68, 130, 255 })
#define UI_COLOR_ORBITAL_DARK     ((Color){ 8, 22, 48, 255 })

// --- Paleta Táctica Futurista (HUD & Telemetría) ---
#define UI_COLOR_AC4_GREEN        ((Color){ 105, 242, 180, 240 }) // Fósforo verde quirúrgico
#define UI_COLOR_AC4_GREEN_DIM    ((Color){ 48, 145, 100, 160 })
#define UI_COLOR_AC4_CYAN         ((Color){ 130, 228, 240, 245 }) // Cyan hielo digital nítido
#define UI_COLOR_AC4_CYAN_DIM     ((Color){ 55, 135, 155, 150 })
#define UI_COLOR_AC4_AMBER        ((Color){ 255, 182, 54, 245 })  // Ámbar alerta
#define UI_COLOR_AC4_ALERT        ((Color){ 255, 55, 68, 255 })   // Alarma roja
#define UI_COLOR_STEEL_WHITE      ((Color){ 242, 246, 252, 255 }) // Blanco titanio nítido
#define UI_COLOR_MUTED_TEXT       ((Color){ 120, 155, 180, 190 }) // Texto secundario
#define UI_COLOR_DARK_TEXT        ((Color){ 65, 95, 120, 180 })

// --- Temas de Color de HUD Táctico ---
typedef enum HUDColorTheme {
    HUD_THEME_GREEN = 0,    // Ace Combat 4 phosphor green
    HUD_THEME_CYAN  = 1,    // WipEout icy cyan
    HUD_THEME_AMBER = 2,    // Tactical amber caution
    HUD_THEME_COUNT = 3
} HUDColorTheme;

// --- Estructura Moddeable de Tema ---
typedef struct UITheme {
    Color bgVoid;
    Color panelBg;
    Color borderActive;
    Color borderDim;
    Color textPrimary;
    Color textSecondary;
    Color textAccent;
    Color textMuted;
    Color hudPrimary;
    Color hudDim;
    Color hudWarning;
    Color hudAlert;

    float uiScale;
    bool isMetric;
    bool enableBeatPumping;
} UITheme;

// --- Clasificación de Tipografías del Sistema ---
typedef enum UIFontType {
    UI_FONT_LOGO = 0,     // Neuropolitical: Logo del juego (Aeroshear::NX)
    UI_FONT_HUD,          // WO3: Todo el HUD e info en vuelo
    UI_FONT_TITLE,        // 2097: Alertas y títulos de menú
    UI_FONT_MENU          // Fusion: Menús de elección, stats, e info varia
} UIFontType;

// Inicializa el sistema de temas y carga las 4 tipografías vectoriales a alta resolución
void UITheme_Init(void);

// Libera recursos del sistema de temas (fuentes, texturas)
void UITheme_Unload(void);

// Devuelve una referencia al tema activo
UITheme* UITheme_Get(void);

// Configura y aplica el tema de color del HUD
void UITheme_SetHUDTheme(HUDColorTheme theme);

// Devuelve el tema de color actual del HUD
HUDColorTheme UITheme_GetHUDTheme(void);

// Devuelve el nombre legible en pantalla del tema
const char *UITheme_GetHUDThemeName(HUDColorTheme theme);

// Devuelve la fuente según su categoría
Font UI_GetTypedFont(UIFontType type);

// Devuelve la fuente de menú/info por defecto (Fusion)
Font UI_GetFont(void);

// --- Funciones de Dibujado General (Fusion: Menús, Stats, Info Varia) ---
void UI_DrawText(const char *text, float x, float y, float size, Color color);
void UI_DrawTextSpacing(const char *text, float x, float y, float size, float spacing, Color color);
Vector2 UI_MeasureText(const char *text, float size);
Vector2 UI_MeasureTextSpacing(const char *text, float size, float spacing);

// --- Funciones para HUD e Info en Vuelo (WO3) ---
void UI_DrawTextHud(const char *text, float x, float y, float size, Color color);
void UI_DrawTextHudSpacing(const char *text, float x, float y, float size, float spacing, Color color);
Vector2 UI_MeasureTextHud(const char *text, float size);
Vector2 UI_MeasureTextHudSpacing(const char *text, float size, float spacing);

// --- Funciones para Alertas y Títulos de Menú (2097) ---
void UI_DrawTextTitle(const char *text, float x, float y, float size, Color color);
void UI_DrawTextTitleSpacing(const char *text, float x, float y, float size, float spacing, Color color);
Vector2 UI_MeasureTextTitle(const char *text, float size);
Vector2 UI_MeasureTextTitleSpacing(const char *text, float size, float spacing);

// --- Funciones Explícitas para Menús, Stats e Info (Fusion) ---
void UI_DrawTextMenu(const char *text, float x, float y, float size, Color color);
void UI_DrawTextMenuSpacing(const char *text, float x, float y, float size, float spacing, Color color);
Vector2 UI_MeasureTextMenu(const char *text, float size);
Vector2 UI_MeasureTextMenuSpacing(const char *text, float size, float spacing);

// --- Funciones para Logo y Marca del Juego (Neuropolitical) ---
void UI_DrawTextLogo(const char *text, float x, float y, float size, Color color);
void UI_DrawTextLogoSpacing(const char *text, float x, float y, float size, float spacing, Color color);
Vector2 UI_MeasureTextLogo(const char *text, float size);
Vector2 UI_MeasureTextLogoSpacing(const char *text, float size, float spacing);

#endif // UI_THEME_H
