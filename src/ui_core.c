#include "ui_core.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// AEROSHEAR // CORE VECTOR UI PRIMITIVES (Y2K / ACE COMBAT 4 PRECISION)
// ============================================================================

static Shader s_bgShader = { 0 };
static int s_locBgRes = -1;
static int s_locBgMouse = -1;
static int s_locBgTime = -1;
static int s_locBgBiomeTint = -1;
static int s_locBgPlanetAlpha = -1;
static bool s_bgShaderLoaded = false;
static bool s_bgShaderAttempted = false;

void UI_Core_Init(void) {
    if (!s_bgShaderLoaded && FileExists("shaders/menu_bg.vs") && FileExists("shaders/menu_bg.fs")) {
        s_bgShader = LoadShader("shaders/menu_bg.vs", "shaders/menu_bg.fs");
        if (s_bgShader.id != 0) {
            s_locBgRes = GetShaderLocation(s_bgShader, "uResolution");
            s_locBgMouse = GetShaderLocation(s_bgShader, "uMouse");
            s_locBgTime = GetShaderLocation(s_bgShader, "uTime");
            s_locBgBiomeTint = GetShaderLocation(s_bgShader, "uBiomeTint");
            s_locBgPlanetAlpha = GetShaderLocation(s_bgShader, "uPlanetAlpha");
            s_bgShaderLoaded = true;
            TraceLog(LOG_INFO, "[UI_CORE] Continuous Menu Background GPU Shader loaded successfully.");
        }
    }
    s_bgShaderAttempted = true;
}

void UI_Core_Unload(void) {
    if (s_bgShaderLoaded) {
        UnloadShader(s_bgShader);
        s_bgShaderLoaded = false;
    }
}

// 1. Limbo Orbital Atmosférico con Gradiente Continuo GPU y Reactivo
void UI_DrawOrbitalLimbEx(int screenWidth, int screenHeight, float time, Vector2 parallaxOffset, Color biomeTint) {
    if (!s_bgShaderAttempted) {
        UI_Core_Init();
    }

    if (s_bgShaderLoaded) {
        float res[2] = { (float)screenWidth, (float)screenHeight };
        Vector2 mPos = GetMousePosition();
        float mouse[2] = { mPos.x + parallaxOffset.x * 2.0f, mPos.y + parallaxOffset.y * 2.0f };
        float tint[3] = { (float)biomeTint.r / 255.0f, (float)biomeTint.g / 255.0f, (float)biomeTint.b / 255.0f };
        float planetAlpha = 1.0f;

        SetShaderValue(s_bgShader, s_locBgRes, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(s_bgShader, s_locBgMouse, mouse, SHADER_UNIFORM_VEC2);
        SetShaderValue(s_bgShader, s_locBgTime, &time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(s_bgShader, s_locBgBiomeTint, tint, SHADER_UNIFORM_VEC3);
        SetShaderValue(s_bgShader, s_locBgPlanetAlpha, &planetAlpha, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(s_bgShader);
            DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
        EndShaderMode();
    } else {
        // Fallback GPU de degradado continuo ultra suave sin escalonamiento
        DrawRectangleGradientEx((Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight },
                                (Color){ 6, 16, 28, 255 }, (Color){ 2, 5, 10, 255 },
                                (Color){ 1, 3, 7, 255 }, (Color){ 12, 32, 55, 255 });
    }

    // Cruces de referencia CAD sutiles
    Color gridCross = (Color){ 60, 110, 150, 42 };
    for (int gx = 90; gx < screenWidth; gx += 180) {
        for (int gy = 80; gy < screenHeight - 60; gy += 150) {
            DrawLine(gx - 3, gy, gx + 3, gy, gridCross);
            DrawLine(gx, gy - 3, gx, gy + 3, gridCross);
        }
    }
}

void UI_DrawOrbitalLimb(int screenWidth, int screenHeight, float time) {
    UI_DrawOrbitalLimbEx(screenWidth, screenHeight, time, (Vector2){ 0, 0 }, (Color){ 82, 175, 240, 255 });
}

// 2. Fondo Táctico Cinematográfico para Hangar y Briefing con Gradiente Reactivo Continuo
void UI_DrawTacticalBackdropEx(int screenWidth, int screenHeight, float time, Color biomeTint) {
    if (!s_bgShaderAttempted) {
        UI_Core_Init();
    }

    if (s_bgShaderLoaded) {
        float res[2] = { (float)screenWidth, (float)screenHeight };
        Vector2 mPos = GetMousePosition();
        float mouse[2] = { mPos.x, mPos.y };
        float tint[3] = { (float)biomeTint.r / 255.0f, (float)biomeTint.g / 255.0f, (float)biomeTint.b / 255.0f };
        float planetAlpha = 0.0f; // Fondo atmosférico profundo sin limbo planetario

        SetShaderValue(s_bgShader, s_locBgRes, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(s_bgShader, s_locBgMouse, mouse, SHADER_UNIFORM_VEC2);
        SetShaderValue(s_bgShader, s_locBgTime, &time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(s_bgShader, s_locBgBiomeTint, tint, SHADER_UNIFORM_VEC3);
        SetShaderValue(s_bgShader, s_locBgPlanetAlpha, &planetAlpha, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(s_bgShader);
            DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
        EndShaderMode();
    } else {
        DrawRectangleGradientEx((Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight },
                                (Color){ 8, 18, 30, 255 }, (Color){ 2, 4, 8, 255 },
                                (Color){ 2, 4, 8, 255 }, (Color){ 10, 24, 40, 255 });
    }

    // Rejilla de perspectiva técnica sutil (degradado suave de alfa, cero escalones)
    float startY = (float)screenHeight * 0.72f;
    for (int i = 0; i < 7; i++) {
        float y = startY + (float)(i * i) * 4.2f;
        if (y > (float)screenHeight) break;
        float alpha = (float)(7 - i) / 7.0f * 30.0f;
        DrawLine(0, (int)y, screenWidth, (int)y, (Color){ 50, 100, 150, (unsigned char)alpha });
    }

    // Cruces de referencia CAD
    Color gridCross = (Color){ 60, 110, 150, 36 };
    for (int gx = 90; gx < screenWidth; gx += 180) {
        for (int gy = 70; gy < screenHeight - 60; gy += 150) {
            DrawLine(gx - 3, gy, gx + 3, gy, gridCross);
            DrawLine(gx, gy - 3, gx, gy + 3, gridCross);
        }
    }
}

void UI_DrawTacticalBackdrop(int screenWidth, int screenHeight, float time) {
    UI_DrawTacticalBackdropEx(screenWidth, screenHeight, time, (Color){ 70, 160, 230, 255 });
}

// 2b. Cabecera Superior Líquida Y2K (Authentic Liquid Glass Header)
void UI_DrawLiquidHeader(int screenWidth, const char *title, const char *subtitle, Color accent) {
    int headerH = 50;

    // Cuerpo de vidrio translúcido
    DrawRectangle(0, 0, screenWidth, headerH, (Color){ 6, 14, 24, 215 });

    // Reflejo especular superior continuo de vidrio pulido
    DrawRectangleGradientV(0, 0, screenWidth, 22, (Color){ 255, 255, 255, 18 }, (Color){ 255, 255, 255, 0 });

    // Línea de bisel superior de vidrio
    DrawLine(0, 0, screenWidth, 0, (Color){ 255, 255, 255, 45 });

    // Borde inferior esmerilado con sutil refracción (CERO neón)
    DrawLine(0, headerH - 1, screenWidth, headerH - 1, (Color){ 255, 255, 255, 25 });
    if (accent.a > 0) {
        DrawRectangleGradientH(0, headerH - 1, (int)((float)screenWidth * 0.45f), 1,
                               (Color){ accent.r, accent.g, accent.b, 65 }, BLANK);
    }

    // Branding AEROSHEAR // NX a la izquierda
    float leftPad = (float)screenWidth * 0.04f;
    if (leftPad < 24.0f) leftPad = 24.0f;

    UI_DrawTextLogo("AEROSHEAR::NX", leftPad, 15.0f, 15.0f, UI_COLOR_STEEL_WHITE);

    // Separador vertical de vidrio esmerilado
    float titleX = leftPad + 180.0f;
    DrawLine((int)titleX - 16, 14, (int)titleX - 16, headerH - 14, (Color){ 255, 255, 255, 30 });

    // Título de la pantalla actual en fuente 2097
    if (title && title[0] != '\0') {
        UI_DrawTextTitle(title, titleX, 12.0f, 15.0f, (Color){ 220, 240, 255, 240 });
    }

    // Subtítulo descriptivo en Fusion
    if (subtitle && subtitle[0] != '\0') {
        UI_DrawTextMenu(subtitle, titleX, 29.0f, 11.0f, UI_COLOR_MUTED_TEXT);
    }

    // Estado táctico y de telemetría a la derecha
    float rightX = (float)screenWidth - 280.0f;
    UI_DrawTextHud("SYS // LINK: ONLINE", rightX, 14.0f, 11.0f, (Color){ 120, 165, 195, 190 });
    UI_DrawTextHud("FLIGHT OPS // SEC.04", rightX, 28.0f, 10.0f, (Color){ 75, 115, 145, 160 });
}

// 2b-2. Cabecera Minimalista Unificada (Estilo AEROSHEAR::NX)
void UI_DrawScreenHeader(int screenWidth, const char *title, const char *subtitle, const char *categoryTag) {
    int y = 44;
    Color lineCol = (Color){ 255, 255, 255, 26 };
    Color tickCol = (Color){ 130, 185, 225, 200 };

    // Línea de referencia y separación horizontal
    DrawLine(24, y + 24, screenWidth - 36, y + 24, lineCol);

    // Cruz de referencia técnica CAD
    int crossX = 36;
    DrawLine(crossX, y + 12 - 8, crossX, y + 12 + 8, tickCol);
    DrawLine(crossX - 8, y + 12, crossX + 8, y + 12, tickCol);

    // Título y subtítulo flotante a la izquierda
    float leftX = (float)(crossX + 22);
    if (title && title[0] != '\0') {
        UI_DrawTextTitle(title, leftX, (float)(y - 6), 18.0f, UI_COLOR_STEEL_WHITE);
    }
    if (subtitle && subtitle[0] != '\0') {
        Vector2 tSz = UI_MeasureTextTitle(title ? title : "", 18.0f);
        float subX = leftX + tSz.x + 18.0f;
        // Separador vertical sutil
        DrawLine((int)(subX - 9), y - 2, (int)(subX - 9), y + 14, (Color){ 255, 255, 255, 40 });
        UI_DrawTextMenu(subtitle, subX, (float)(y + 1), 11.0f, UI_COLOR_MUTED_TEXT);
    }

    // Tag táctico / telemetría a la derecha
    const char *tag = categoryTag ? categoryTag : "SYS // LINK: ONLINE   FLIGHT OPS // SEC.04";
    Vector2 tagSz = UI_MeasureTextHud(tag, 11.0f);
    float rightX = (float)screenWidth - 136.0f - tagSz.x;
    UI_DrawTextHud(tag, rightX, (float)(y + 1), 11.0f, (Color){ 90, 145, 180, 200 });
}

// 2c. Barra Inferior de Navegación y Ayuda
void UI_DrawNavHelp(int screenWidth, int screenHeight, const char *hints) {
    if (!hints || hints[0] == '\0') return;

    int y = screenHeight - 26;
    float startX = (float)screenWidth * 0.04f;
    float endX = (float)screenWidth * 0.96f;

    DrawLine((int)startX, y - 6, (int)endX, y - 6, (Color){ 255, 255, 255, 25 });

    Vector2 sz = UI_MeasureTextMenu(hints, 11.0f);
    float textX = ((float)screenWidth - sz.x) * 0.5f;
    UI_DrawTextMenu(hints, textX, (float)y, 11.0f, (Color){ 135, 175, 205, 210 });
}

// 2d. Banner de Alerta Táctico estilo Liquid Glass
void UI_DrawAlertBanner(float centerX, float y, const char *text, Color bannerColor, float alpha) {
    if (!text || text[0] == '\0' || alpha <= 0.01f) return;

    Vector2 sz = UI_MeasureTextTitle(text, 14.0f);
    int padX = 24;
    int padY = 7;
    int w = (int)sz.x + padX * 2;
    int h = (int)sz.y + padY * 2;
    int x = (int)(centerX - (float)w * 0.5f);

    Rectangle bounds = { (float)x, y, (float)w, (float)h };

    // Sombra suave
    DrawRectangleRounded((Rectangle){ bounds.x, bounds.y + 3, bounds.width, bounds.height }, 0.35f, 6, (Color){ 0, 0, 0, (unsigned char)(90.0f * alpha) });
    // Cuerpo de vidrio translúcido oscuro
    DrawRectangleRounded(bounds, 0.35f, 6, (Color){ 8, 16, 26, (unsigned char)(210.0f * alpha) });
    // Brillo superior
    DrawRectangleGradientV(x + 2, (int)y + 2, w - 4, (int)(h * 0.45f), (Color){ 255, 255, 255, (unsigned char)(28.0f * alpha) }, BLANK);
    DrawLine(x + 10, (int)y, x + w - 10, (int)y, (Color){ 255, 255, 255, (unsigned char)(80.0f * alpha) });
    // Borde esmerilado con sutil tinte
    DrawRectangleRoundedLinesEx(bounds, 0.35f, 6, 1.0f, (Color){ bannerColor.r, bannerColor.g, bannerColor.b, (unsigned char)(100.0f * alpha) });

    UI_DrawTextTitle(text, (float)(x + padX), y + (float)padY, 14.0f, (Color){ bannerColor.r, bannerColor.g, bannerColor.b, (unsigned char)(bannerColor.a * alpha) });
}

// 3. Barra de Registro Técnico Superior
void UI_DrawTopRegistrationBar(int screenWidth, int y) {
    Color lineCol = (Color){ 255, 255, 255, 26 };
    Color tickCol = (Color){ 130, 185, 225, 200 };

    DrawLine(24, y, screenWidth - 36, y, lineCol);

    int crossX = 42;
    DrawLine(crossX, y - 12, crossX, y + 12, tickCol);
    DrawLine(crossX - 12, y, crossX + 12, y, tickCol);

    UI_DrawText("SYS // NAV-REF: 04-A", crossX + 24, (float)(y - 9), 11.0f, (Color){ 90, 135, 165, 180 });
    UI_DrawText("FLIGHT-OPS // STATUS: READY", (float)(screenWidth - 230), (float)(y - 9), 11.0f, (Color){ 90, 135, 165, 180 });
}

// 4. Logo y Marca Oficial: "AEROSHEAR::NX" en Tipografía Neuropolitical
void UI_DrawTitleAeroshear(int x, int y, float scale, Color color) {
    float size = 42.0f * scale;
    float tracking = 3.0f * scale;
    const char *logoText = "AEROSHEAR::NX";

    // Sombra suave proyectada para volumen
    UI_DrawTextLogoSpacing(logoText, (float)(x + 2), (float)(y + 3), size, tracking, (Color){ 0, 12, 24, 210 });

    // Título vectorial principal en Neuropolitical
    UI_DrawTextLogoSpacing(logoText, (float)x, (float)y, size, tracking, color);

    // Marca de registro ™
    Vector2 sz = UI_MeasureTextLogoSpacing(logoText, size, tracking);
    float tmSize = 13.0f * scale;
    if (tmSize < 10.0f) tmSize = 10.0f;
    UI_DrawTextTitle("TM", (float)x + sz.x + 8.0f * scale, (float)y + 3.0f * scale, tmSize, (Color){ color.r, color.g, color.b, (unsigned char)(color.a * 0.85f) });
}

// 5. Botón de Menú Liquid Glass Dark Future Blue (Estilo Frutiger Aero Transparente Oscuro)
bool UI_DrawMenuButton(Rectangle bounds, const char *label, bool isSelected, Vector2 mousePos) {
    bool hovered = CheckCollisionPointRec(mousePos, bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    if (isSelected) {
        // 1. Sombra suave azulada de elevación
        DrawRectangleRounded((Rectangle){ bounds.x, bounds.y + 3, bounds.width, bounds.height }, 0.25f, 6, (Color){ 0, 15, 35, 140 });

        // 2. Cuerpo de vidrio translúcido dark future blue
        DrawRectangleRounded(bounds, 0.25f, 6, (Color){ 14, 32, 56, 220 });

        // 3. Degradado horizontal de energía activa
        DrawRectangleGradientH(x + 2, y + 2, (int)((float)w * 0.75f), h - 4, (Color){ 35, 110, 195, 95 }, (Color){ 35, 110, 195, 0 });

        // 4. Shine especular superior clásico Frutiger Aero (el "líquido")
        DrawRectangleGradientV(x + 4, y + 2, w - 8, (int)((float)h * 0.46f), (Color){ 255, 255, 255, 110 }, (Color){ 255, 255, 255, 0 });

        // 5. Línea de bisel superior de vidrio pulido
        DrawLine(x + 12, y, x + w - 12, y, (Color){ 255, 255, 255, 160 });

        // 6. Borde esmerilado de vidrio con tinte cyan
        DrawRectangleRoundedLinesEx(bounds, 0.25f, 6, 1.2f, (Color){ 110, 195, 255, 160 });

        // 7. Cursor lateral reactivo con halo brillante
        DrawRectangleRounded((Rectangle){ (float)x + 3, (float)y + 4, 4.0f, (float)h - 8 }, 0.5f, 4, (Color){ 90, 220, 255, 255 });

        float fontSize = 15.0f;
        float textY = (float)y + ((float)h - fontSize) * 0.5f;
        UI_DrawTextTitle(label, (float)(x + 23), textY + 1.0f, fontSize, (Color){ 0, 16, 40, 180 });
        UI_DrawTextTitle(label, (float)(x + 22), textY, fontSize, UI_COLOR_STEEL_WHITE);
    } else if (hovered) {
        DrawRectangleRounded((Rectangle){ bounds.x, bounds.y + 2, bounds.width, bounds.height }, 0.25f, 6, (Color){ 0, 10, 25, 100 });
        DrawRectangleRounded(bounds, 0.25f, 6, (Color){ 10, 24, 44, 165 });
        DrawRectangleGradientV(x + 4, y + 2, w - 8, (int)((float)h * 0.46f), (Color){ 255, 255, 255, 55 }, (Color){ 255, 255, 255, 0 });
        DrawLine(x + 12, y, x + w - 12, y, (Color){ 255, 255, 255, 80 });
        DrawRectangleRoundedLinesEx(bounds, 0.25f, 6, 1.0f, (Color){ 95, 170, 235, 95 });

        float fontSize = 15.0f;
        float textY = (float)y + ((float)h - fontSize) * 0.5f;
        UI_DrawTextTitle(label, (float)(x + 20), textY, fontSize, (Color){ 225, 240, 255, 245 });
    } else {
        DrawRectangleRounded(bounds, 0.25f, 6, (Color){ 6, 16, 30, 110 });
        DrawLine(x + 12, y, x + w - 12, y, (Color){ 255, 255, 255, 30 });
        DrawRectangleRoundedLinesEx(bounds, 0.25f, 6, 1.0f, (Color){ 45, 90, 140, 60 });

        float fontSize = 15.0f;
        float textY = (float)y + ((float)h - fontSize) * 0.5f;
        UI_DrawTextTitle(label, (float)(x + 18), textY, fontSize, (Color){ 135, 175, 210, 195 });
    }

    return clicked;
}

// 5b. Fila interactiva minimalista dark future blue liquid glass
bool UI_DrawMinimalOptionRow(Rectangle bounds, const char *label, const char *value, bool isSelected, Vector2 mousePos) {
    bool hovered = CheckCollisionPointRec(mousePos, bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    if (isSelected) {
        DrawRectangleRounded((Rectangle){ bounds.x, bounds.y + 2, bounds.width, bounds.height }, 0.22f, 6, (Color){ 0, 14, 32, 130 });
        DrawRectangleRounded(bounds, 0.22f, 6, (Color){ 14, 32, 56, 215 });
        DrawRectangleGradientH(x + 2, y + 2, (int)((float)w * 0.70f), h - 4, (Color){ 40, 115, 200, 85 }, (Color){ 40, 115, 200, 0 });
        DrawRectangleGradientV(x + 4, y + 2, w - 8, (int)((float)h * 0.46f), (Color){ 255, 255, 255, 95 }, (Color){ 255, 255, 255, 0 });
        DrawLine(x + 10, y, x + w - 10, y, (Color){ 255, 255, 255, 140 });
        DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, 1.1f, (Color){ 110, 195, 255, 150 });
        DrawRectangleRounded((Rectangle){ (float)x + 3, (float)y + 3, 3.5f, (float)h - 6 }, 0.5f, 4, (Color){ 90, 220, 255, 255 });

        float fontSize = 14.0f;
        float textY = (float)y + ((float)h - fontSize) * 0.5f;
        UI_DrawTextTitle(label, (float)(x + 20), textY, fontSize, UI_COLOR_STEEL_WHITE);

        if (value && value[0] != '\0') {
            Vector2 valSz = UI_MeasureTextHud(value, 12.0f);
            UI_DrawTextHud(value, (float)(x + w - 18) - valSz.x, (float)y + ((float)h - 12.0f) * 0.5f, 12.0f, UI_COLOR_AC4_CYAN);
        }
    } else if (hovered) {
        DrawRectangleRounded(bounds, 0.22f, 6, (Color){ 10, 24, 44, 150 });
        DrawRectangleGradientV(x + 4, y + 2, w - 8, (int)((float)h * 0.46f), (Color){ 255, 255, 255, 45 }, (Color){ 255, 255, 255, 0 });
        DrawLine(x + 10, y, x + w - 10, y, (Color){ 255, 255, 255, 60 });
        DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, 1.0f, (Color){ 80, 150, 220, 70 });

        float fontSize = 14.0f;
        float textY = (float)y + ((float)h - fontSize) * 0.5f;
        UI_DrawTextTitle(label, (float)(x + 18), textY, fontSize, (Color){ 220, 235, 248, 235 });

        if (value && value[0] != '\0') {
            Vector2 valSz = UI_MeasureTextHud(value, 12.0f);
            UI_DrawTextHud(value, (float)(x + w - 18) - valSz.x, (float)y + ((float)h - 12.0f) * 0.5f, 12.0f, (Color){ 180, 215, 240, 210 });
        }
    } else {
        DrawRectangleRounded(bounds, 0.22f, 6, (Color){ 6, 16, 28, 95 });
        DrawLine(x + 12, y, x + w - 12, y, (Color){ 255, 255, 255, 20 });
        DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, 1.0f, (Color){ 35, 75, 120, 50 });

        float fontSize = 14.0f;
        float textY = (float)y + ((float)h - fontSize) * 0.5f;
        UI_DrawTextTitle(label, (float)(x + 18), textY, fontSize, (Color){ 130, 170, 205, 190 });

        if (value && value[0] != '\0') {
            Vector2 valSz = UI_MeasureTextHud(value, 12.0f);
            UI_DrawTextHud(value, (float)(x + w - 18) - valSz.x, (float)y + ((float)h - 12.0f) * 0.5f, 12.0f, (Color){ 110, 155, 185, 160 });
        }
    }

    return clicked;
}

// 5d. Pod o Cápsula de Vidrio Translúcido Dark Future Blue con Brillo Líquido Especular y Borde Reactivo
void UI_DrawGlassPod(Rectangle bounds, Color borderGlow, Color bodyTint, float roundness) {
    if (roundness <= 0.0f) roundness = 0.12f;
    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    // 1. Sombra suave ambiental proyectada para sensación de flotabilidad y volumen
    Rectangle shadowRec = { bounds.x, bounds.y + 4.0f, bounds.width, bounds.height };
    DrawRectangleRounded(shadowRec, roundness, 6, (Color){ 0, 2, 6, 125 });

    // 2. Cuerpo de vidrio translúcido dark future blue
    Color body = (bodyTint.a > 0) ? bodyTint : (Color){ 5, 14, 26, 205 };
    DrawRectangleRounded(bounds, roundness, 6, body);

    // 3. Brillo especular superior líquido (Aero specular reflection on upper 46%)
    int sheenH = (int)((float)h * 0.46f);
    if (sheenH < 6) sheenH = 6;
    DrawRectangleGradientV(x + 3, y + 2, w - 6, sheenH, (Color){ 255, 255, 255, 36 }, (Color){ 255, 255, 255, 0 });

    // 4. Línea de bisel superior de vidrio pulido (Specular light catch)
    DrawLine(x + 12, y, x + w - 12, y, (Color){ 255, 255, 255, 95 });

    // 5. Borde de vidrio esmerilado suave con refracción reactiva
    Color rim = (borderGlow.a > 0) ? borderGlow : (Color){ 65, 145, 220, 80 };
    DrawRectangleRoundedLinesEx(bounds, roundness, 6, 1.0f, rim);
}

// 5c. Barra deslizadora limpia sin cajas toscas
float UI_DrawCleanSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal, bool isSelected, Vector2 mousePos) {
    bool hovered = CheckCollisionPointRec(mousePos, bounds);
    float clampedVal = Clamp(value, minVal, maxVal);

    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    if (isSelected) {
        DrawRectangleRounded(bounds, 0.20f, 6, (Color){ 16, 36, 58, 195 });
        DrawRectangleGradientH(x, y, (int)((float)w * 0.70f), h, (Color){ 70, 150, 220, 75 }, (Color){ 70, 150, 220, 0 });
        DrawLine(x + 10, y, x + w - 10, y, (Color){ 255, 255, 255, 80 });
        DrawRectangleRoundedLinesEx(bounds, 0.20f, 6, 1.0f, (Color){ 175, 220, 255, 80 });
        DrawRectangleRounded((Rectangle){ (float)x + 2, (float)y + 3, 3.5f, (float)h - 6 }, 0.5f, 4, (Color){ 135, 210, 255, 235 });
    } else if (hovered) {
        DrawRectangleRounded(bounds, 0.20f, 6, (Color){ 12, 26, 44, 140 });
        DrawRectangleRoundedLinesEx(bounds, 0.20f, 6, 1.0f, (Color){ 255, 255, 255, 30 });
    } else {
        DrawLine(x + 16, y + h, x + w - 16, y + h, (Color){ 255, 255, 255, 12 });
    }

    float fontSize = 14.0f;
    float textY = (float)y + ((float)h - fontSize) * 0.5f;
    Color labelCol = isSelected ? UI_COLOR_STEEL_WHITE : (hovered ? (Color){ 220, 235, 245, 230 } : (Color){ 125, 165, 195, 190 });
    UI_DrawTextTitle(label, (float)(x + 20), textY, fontSize, labelCol);

    float trackW = (float)w * 0.38f;
    float trackX = (float)(x + w) - trackW - 65.0f;
    float trackY = (float)y + (float)h * 0.5f;
    float trackH = 4.0f;

    Rectangle trackRec = { trackX - 10.0f, trackY - 12.0f, trackW + 20.0f, 24.0f };
    if (CheckCollisionPointRec(mousePos, trackRec) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float rel = (mousePos.x - trackX) / trackW;
        rel = Clamp(rel, 0.0f, 1.0f);
        clampedVal = minVal + rel * (maxVal - minVal);
    }

    float norm = (clampedVal - minVal) / (maxVal - minVal);
    norm = Clamp(norm, 0.0f, 1.0f);

    DrawRectangleRounded((Rectangle){ trackX, trackY - trackH * 0.5f, trackW, trackH }, 0.5f, 4, (Color){ 25, 50, 75, 190 });

    Color fillCol = isSelected ? UI_COLOR_AC4_CYAN : (Color){ 80, 155, 210, 200 };
    if (norm > 0.01f) {
        DrawRectangleRounded((Rectangle){ trackX, trackY - trackH * 0.5f, trackW * norm, trackH }, 0.5f, 4, fillCol);
    }

    float thumbX = trackX + trackW * norm;
    DrawRectangleRounded((Rectangle){ thumbX - 3.0f, trackY - 7.0f, 6.0f, 14.0f }, 0.4f, 4, UI_COLOR_STEEL_WHITE);
    if (isSelected) {
        DrawRectangleRoundedLinesEx((Rectangle){ thumbX - 3.0f, trackY - 7.0f, 6.0f, 14.0f }, 0.4f, 4, 1.0f, UI_COLOR_AC4_CYAN);
    }

    int pct = (int)(norm * 100.0f + 0.5f);
    char valStr[16];
    snprintf(valStr, sizeof(valStr), "%3d%%", pct);
    UI_DrawTextHud(valStr, (float)(x + w - 50), (float)y + ((float)h - 12.0f) * 0.5f, 12.0f, isSelected ? UI_COLOR_AC4_CYAN : UI_COLOR_MUTED_TEXT);

    return clampedVal;
}

// 6. Corchetes Vectoriales (Eliminados de bordes de ventanas para evitar estética neón)

void UI_DrawCornerBrackets(Rectangle bounds, int bracketSize, Color color) {
    // Intencionalmente vacío para eliminar los marcos neón de estilo cyber/wireframe
    (void)bounds;
    (void)bracketSize;
    (void)color;
}

// 7. Panel de Vidrio Líquido Auténtico (Liquid Glass Panel - CERO bordes neón)
void UI_DrawGlassPanel(Rectangle bounds, const char *titleBadge, Color borderColor, Color bgColor) {
    UI_DrawGlassPanelEx(bounds, titleBadge, borderColor, bgColor, 8.0f);
}

void UI_DrawGlassPanelEx(Rectangle bounds, const char *titleBadge, Color borderColor, Color bgColor, float cornerCutSize) {
    (void)cornerCutSize;
    (void)bgColor;
    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    // 1. Sombra suave de oclusión ambiental (Lifting glass off background)
    Rectangle shadowRec = { bounds.x, bounds.y + 4.0f, bounds.width, bounds.height };
    DrawRectangleRounded(shadowRec, 0.035f, 6, (Color){ 0, 2, 6, 110 });

    // 2. Cuerpo translúcido de acrílico/obsidiana esmerilada
    Color glassBody = (Color){ 8, 16, 28, 190 };
    DrawRectangleRounded(bounds, 0.035f, 6, glassBody);

    // 3. Brillo especular superior de vidrio líquido (Top 45% specular fade)
    int sheenH = (int)fminf(140.0f, (float)h * 0.45f);
    DrawRectangleGradientV(x + 2, y + 2, w - 4, sheenH, (Color){ 255, 255, 255, 24 }, (Color){ 255, 255, 255, 0 });

    // 4. Línea de bisel superior de vidrio pulido (Specular light-catcher)
    DrawLine(x + 12, y, x + w - 12, y, (Color){ 255, 255, 255, 80 });
    DrawLine(x + 1, y + 6, x + 6, y + 1, (Color){ 255, 255, 255, 50 });
    DrawLine(x + w - 6, y + 1, x + w - 1, y + 6, (Color){ 255, 255, 255, 50 });

    // 5. Borde esmerilado suave (CERO neón)
    DrawRectangleRoundedLinesEx(bounds, 0.035f, 6, 1.0f, (Color){ 255, 255, 255, 34 });

    // Sutil refracción óptica si el panel tiene color de acento asignado
    if (borderColor.a > 30) {
        Color subtleRefract = (Color){ borderColor.r, borderColor.g, borderColor.b, (unsigned char)fminf(65.0f, (float)borderColor.a * 0.32f) };
        DrawRectangleRoundedLinesEx(bounds, 0.035f, 6, 1.0f, subtleRefract);
    }

    // 6. Insignia o pestaña de título integrada de vidrio esmerilado (Sin cajas negras con marco neón)
    if (titleBadge && titleBadge[0] != '\0') {
        Vector2 sz = UI_MeasureTextTitle(titleBadge, 11.0f);
        int tabW = (int)sz.x + 24;
        int tabH = 18;
        int tabX = x + 16;
        int tabY = y - 9;
        Rectangle tabRec = { (float)tabX, (float)tabY, (float)tabW, (float)tabH };

        // Píldora de pestaña esmerilada
        DrawRectangleRounded(tabRec, 0.35f, 4, (Color){ 12, 24, 40, 230 });
        DrawRectangleRoundedLinesEx(tabRec, 0.35f, 4, 1.0f, (Color){ 255, 255, 255, 45 });
        DrawLine(tabX + 4, tabY, tabX + tabW - 4, tabY, (Color){ 255, 255, 255, 90 });

        // Texto del título grabado
        UI_DrawTextTitle(titleBadge, (float)(tabX + 12), (float)(tabY + 4), 11.0f, (Color){ 215, 235, 250, 240 });
    }
}

// 8. Línea de División Técnica con Muescas
void UI_DrawTechDivider(int x1, int y, int x2, Color color) {
    DrawLine(x1, y, x2, y, color);
    DrawLine(x1, y - 3, x1, y + 3, color);
    DrawLine(x2, y - 3, x2, y + 3, color);
}

// 9. Slider Táctico
float UI_DrawSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal, bool isSelected, Vector2 mousePos) {
    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    UI_DrawText(label, (float)x, (float)(y + 2), 13.0f, isSelected ? UI_COLOR_STEEL_WHITE : UI_COLOR_MUTED_TEXT);

    // Medir ancho de la etiqueta para garantizar separación absoluta (CERO solapamientos)
    Vector2 labelSz = UI_MeasureText(label, 13.0f);
    int labelMargin = 16;
    int minTrackX = x + (int)labelSz.x + labelMargin;

    int desiredTrackW = (int)fminf(190.0f, (float)w * 0.38f);
    if (desiredTrackW < 120) desiredTrackW = 120;
    int valW = 46;

    int trackX = x + w - desiredTrackW - valW - 8;
    if (trackX < minTrackX) trackX = minTrackX;
    int trackW = (x + w - valW - 8) - trackX;
    if (trackW < 60) trackW = 60;

    int trackY = y + 7;
    int trackH = 8;
    Rectangle trackRec = { (float)trackX, (float)trackY, (float)trackW, (float)trackH };

    // Pista de vidrio esmerilado translúcido
    DrawRectangleRounded(trackRec, 0.5f, 4, (Color){ 8, 16, 26, 180 });
    DrawRectangleRoundedLinesEx(trackRec, 0.5f, 4, 1.0f, (Color){ 255, 255, 255, 30 });

    float fraction = (value - minVal) / (maxVal - minVal);
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    int fillW = (int)((float)trackW * fraction);
    if (fillW > 4) {
        Rectangle fillRec = { (float)trackX + 1, (float)trackY + 1, (float)(fillW - 2), (float)(trackH - 2) };
        DrawRectangleRounded(fillRec, 0.5f, 4, (Color){ 70, 160, 230, 200 });
        DrawLine((int)fillRec.x + 2, (int)fillRec.y, (int)(fillRec.x + fillRec.width - 2), (int)fillRec.y, (Color){ 255, 255, 255, 90 });
    }

    UI_DrawText(TextFormat("%d%%", (int)(fraction * 100.0f)), (float)(trackX + trackW + 12), (float)(y + 2), 12.0f, UI_COLOR_STEEL_WHITE);

    Rectangle clickArea = { (float)trackX - 5, (float)y, (float)trackW + 10, (float)h };
    if (CheckCollisionPointRec(mousePos, clickArea) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float newFrac = (mousePos.x - (float)trackX) / (float)trackW;
        if (newFrac < 0.0f) newFrac = 0.0f;
        if (newFrac > 1.0f) newFrac = 1.0f;
        return minVal + newFrac * (maxVal - minVal);
    }

    return value;
}

// 10. Toggle Binario estilo Liquid Glass [ACTIVE / BYPASS]
bool UI_DrawToggle(Rectangle bounds, const char *label, bool state, bool isSelected, Vector2 mousePos) {
    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;

    UI_DrawText(label, (float)x, (float)(y + 2), 13.0f, isSelected ? UI_COLOR_STEEL_WHITE : UI_COLOR_MUTED_TEXT);

    int btnX = x + w - 90;
    int btnW = 80;
    int btnH = 22;
    Rectangle btnRec = { (float)btnX, (float)y, (float)btnW, (float)btnH };

    bool hovered = CheckCollisionPointRec(mousePos, btnRec);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color bgCol = state ? (Color){ 8, 36, 28, 200 } : (Color){ 22, 14, 18, 180 };
    DrawRectangleRounded(btnRec, 0.35f, 4, bgCol);
    DrawRectangleGradientV(btnX + 2, y + 2, btnW - 4, 10, (Color){ 255, 255, 255, 18 }, (Color){ 255, 255, 255, 0 });
    DrawLine(btnX + 6, y, btnX + btnW - 6, y, (Color){ 255, 255, 255, 65 });
    DrawRectangleRoundedLinesEx(btnRec, 0.35f, 4, 1.0f, state ? (Color){ 100, 220, 165, 75 } : (Color){ 255, 255, 255, 28 });

    const char *stateTxt = state ? "ACTIVE" : "BYPASS";
    Vector2 sz = UI_MeasureText(stateTxt, 11.0f);
    UI_DrawText(stateTxt, (float)btnX + ((float)btnW - sz.x) * 0.5f, (float)(y + 4), 11.0f, state ? (Color){ 165, 245, 205, 230 } : (Color){ 195, 145, 155, 190 });

    if (clicked) return !state;
    return state;
}

// 10b. Selector de Opciones Cíclico estilo Liquid Glass [ < OPTION > ]
int UI_DrawOptionSelector(Rectangle bounds, const char *label, const char *currentOption, bool isSelected, Vector2 mousePos) {
    int x = (int)bounds.x;
    int y = (int)bounds.y;
    int w = (int)bounds.width;
    int h = (int)bounds.height;

    // Etiqueta a la izquierda
    UI_DrawText(label, (float)x, (float)(y + 2), 13.0f, isSelected ? UI_COLOR_STEEL_WHITE : UI_COLOR_MUTED_TEXT);

    int optW = 190;
    int optX = x + w - optW;
    int optY = y;
    int optH = h > 22 ? h : 22;

    Rectangle optRec = { (float)optX, (float)optY, (float)optW, (float)optH };
    bool hovered = CheckCollisionPointRec(mousePos, optRec);

    Color bgCol = isSelected ? (Color){ 16, 36, 58, 210 } : (hovered ? (Color){ 12, 24, 38, 180 } : (Color){ 8, 16, 26, 150 });
    DrawRectangleRounded(optRec, 0.25f, 4, bgCol);
    DrawRectangleGradientV(optX + 2, optY + 2, optW - 4, (int)(optH * 0.45f), (Color){ 255, 255, 255, 20 }, (Color){ 255, 255, 255, 0 });
    DrawLine(optX + 8, optY, optX + optW - 8, optY, (Color){ 255, 255, 255, 70 });
    DrawRectangleRoundedLinesEx(optRec, 0.25f, 4, 1.0f, isSelected ? (Color){ 160, 215, 255, 80 } : (Color){ 255, 255, 255, 28 });

    // Flecha izquierda <
    Rectangle leftArrowRec = { (float)optX, (float)optY, 26.0f, (float)optH };
    bool hoverLeft = CheckCollisionPointRec(mousePos, leftArrowRec);
    UI_DrawTextTitle("<", (float)(optX + 8), (float)(optY + 3), 12.0f, hoverLeft ? UI_COLOR_STEEL_WHITE : (Color){ 160, 195, 220, 180 });

    // Flecha derecha >
    Rectangle rightArrowRec = { (float)(optX + optW - 26), (float)optY, 26.0f, (float)optH };
    bool hoverRight = CheckCollisionPointRec(mousePos, rightArrowRec);
    UI_DrawTextTitle(">", (float)(optX + optW - 16), (float)(optY + 3), 12.0f, hoverRight ? UI_COLOR_STEEL_WHITE : (Color){ 160, 195, 220, 180 });

    // Texto de la opción centrada
    Vector2 valSz = UI_MeasureText(currentOption, 11.0f);
    float textX = (float)optX + ((float)optW - valSz.x) * 0.5f;
    UI_DrawText(currentOption, textX, (float)(optY + 4), 11.0f, isSelected ? UI_COLOR_STEEL_WHITE : (Color){ 185, 225, 255, 220 });

    if (hoverLeft && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return -1;
    if (hoverRight && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 1;

    return 0;
}

// 11. Barra Segmentada de Precisión Aeroespacial (Células discretas con micro-gaps)
void UI_DrawSegmentedBar(int x, int y, int width, int height, float value, float maxValue, int totalSegments, Color activeCol, Color inactiveCol) {
    if (totalSegments <= 0) totalSegments = 10;
    float frac = value / maxValue;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    int activeCount = (int)roundf(frac * (float)totalSegments);
    int gap = 3;
    int segW = (width - (totalSegments - 1) * gap) / totalSegments;
    if (segW < 2) segW = 2;

    for (int i = 0; i < totalSegments; i++) {
        int sx = x + i * (segW + gap);
        bool isActive = (i < activeCount);
        if (isActive) {
            DrawRectangle(sx, y, segW, height, activeCol);
            DrawRectangle(sx, y, segW, 1, (Color){ 255, 255, 255, 180 }); // Borde brillante superior
        } else {
            DrawRectangle(sx, y, segW, height, (Color){ 8, 18, 28, 190 });
            DrawRectangleLines(sx, y, segW, height, inactiveCol);
        }
    }
}

// 12. Plataforma de Pedestal Holográfica Elíptica (Showroom Hangar)
void UI_DrawHoloPedestal(Vector2 center, float radiusX, float radiusY, float time, Color color) {
    // Sombra suave en el suelo directamente debajo del caza
    DrawEllipse((int)center.x, (int)center.y + 4, radiusX * 0.55f, radiusY * 0.45f, (Color){ 0, 0, 0, 160 });

    // Columna vertical sutil de luz holográfica
    Color beamBottom = (Color){ color.r, color.g, color.b, 25 };
    Color beamTop    = (Color){ color.r, color.g, color.b, 0 };
    Vector2 v1 = { center.x - radiusX * 0.7f, center.y };
    Vector2 v2 = { center.x + radiusX * 0.7f, center.y };
    Vector2 v3 = { center.x + radiusX * 0.4f, center.y - radiusY * 4.5f };
    Vector2 v4 = { center.x - radiusX * 0.4f, center.y - radiusY * 4.5f };
    DrawTriangle(v1, v2, v3, beamBottom);
    DrawTriangle(v1, v3, v4, beamTop);

    // Anillos elípticos concéntricos
    const int segments = 48;
    for (int i = 0; i < segments; i++) {
        float a1 = (float)i / (float)segments * 2.0f * PI;
        float a2 = (float)(i + 1) / (float)segments * 2.0f * PI;

        Vector2 p1 = { center.x + cosf(a1) * radiusX, center.y + sinf(a1) * radiusY };
        Vector2 p2 = { center.x + cosf(a2) * radiusX, center.y + sinf(a2) * radiusY };
        DrawLineEx(p1, p2, 1.2f, (Color){ color.r, color.g, color.b, 140 });

        // Anillo interior concéntrico
        Vector2 ip1 = { center.x + cosf(a1) * (radiusX * 0.68f), center.y + sinf(a1) * (radiusY * 0.68f) };
        Vector2 ip2 = { center.x + cosf(a2) * (radiusX * 0.68f), center.y + sinf(a2) * (radiusY * 0.68f) };
        DrawLineEx(ip1, ip2, 1.0f, (Color){ color.r, color.g, color.b, 70 });
    }

    // Marcas de rotación técnica en el pedestal
    float rotAngle = time * 0.45f;
    for (int t = 0; t < 8; t++) {
        float a = rotAngle + (float)t * (2.0f * PI / 8.0f);
        Vector2 pOuter = { center.x + cosf(a) * (radiusX + 6.0f), center.y + sinf(a) * (radiusY + 4.0f) };
        Vector2 pInner = { center.x + cosf(a) * (radiusX - 8.0f), center.y + sinf(a) * (radiusY - 5.0f) };
        DrawLineEx(pOuter, pInner, 1.5f, color);
    }
}

// 13. Insignia o Badge Táctico
void UI_DrawPillBadge(float x, float y, const char *text, Color borderCol, Color bgCol, Color textCol, float fontSize) {
    UI_DrawPillBadgeEx(x, y, text, borderCol, bgCol, textCol, fontSize);
}

float UI_DrawPillBadgeEx(float x, float y, const char *text, Color borderCol, Color bgCol, Color textCol, float fontSize) {
    if (!text || text[0] == '\0') return 0.0f;
    Vector2 sz = UI_MeasureTextTitle(text, fontSize);
    int padX = 8;
    int padY = 3;
    Rectangle rec = { x, y, sz.x + (float)(padX * 2), sz.y + (float)(padY * 2) };
    DrawRectangleRec(rec, bgCol);
    DrawRectangleLinesEx(rec, 1.0f, borderCol);
    UI_DrawTextTitle(text, x + (float)padX, y + (float)padY, fontSize, textCol);
    return rec.width;
}

// 14. Barra de Desplazamiento Vertical Modular (Scrollbar con arrastre y soporte de rueda)
void UI_DrawVerticalScrollbar(Rectangle bounds, float *scrollOffset, float contentHeight, float viewHeight, Vector2 mousePos) {
    if (!scrollOffset || contentHeight <= viewHeight || viewHeight <= 0.0f) return;

    float maxScroll = contentHeight - viewHeight;
    float thumbRatio = viewHeight / contentHeight;
    float thumbH = fmaxf(24.0f, bounds.height * thumbRatio);
    float trackH = bounds.height;

    // Rueda del ratón si el cursor está sobre la barra
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && CheckCollisionPointRec(mousePos, bounds)) {
        *scrollOffset -= wheel * 36.0f;
    }

    // Interacción por arrastre del mouse
    bool hovered = CheckCollisionPointRec(mousePos, bounds);
    if (hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float relY = mousePos.y - bounds.y - thumbH * 0.5f;
        float newRatio = relY / (trackH - thumbH);
        newRatio = Clamp(newRatio, 0.0f, 1.0f);
        *scrollOffset = newRatio * maxScroll;
    }

    if (*scrollOffset < 0.0f) *scrollOffset = 0.0f;
    if (*scrollOffset > maxScroll) *scrollOffset = maxScroll;

    // Dibujar pista de scrollbar
    DrawRectangleRounded(bounds, 0.5f, 4, (Color){ 8, 16, 26, 160 });
    DrawRectangleRoundedLinesEx(bounds, 0.5f, 4, 1.0f, (Color){ 255, 255, 255, 25 });

    // Posición del thumb
    float scrollRatio = *scrollOffset / maxScroll;
    scrollRatio = Clamp(scrollRatio, 0.0f, 1.0f);
    float thumbY = bounds.y + scrollRatio * (trackH - thumbH);
    Rectangle thumbRec = { bounds.x + 1.0f, thumbY, bounds.width - 2.0f, thumbH };
    bool thumbHover = CheckCollisionPointRec(mousePos, thumbRec);

    Color thumbCol = thumbHover ? (Color){ 100, 185, 240, 220 } : (Color){ 65, 130, 190, 180 };
    DrawRectangleRounded(thumbRec, 0.5f, 4, thumbCol);
    DrawRectangleRoundedLinesEx(thumbRec, 0.5f, 4, 1.0f, (Color){ 255, 255, 255, 60 });
}

// ============================================================================
// HUD DE VUELO TÁCTICO (CERO ARMAS / CERO HORIZONTE ARTIFICIAL)
// ============================================================================

// 11. Símbolo de Trayectoria de Vuelo (Aircraft Datum - CERO MIRAS DE ARMAS)
void UI_DrawFlightPathMarker(Vector2 screenPos, float rollRad, Color color) {
    float r = 5.0f;
    float wingLen = 11.0f;
    float finH = 6.0f;

    DrawCircleLines((int)screenPos.x, (int)screenPos.y, r, color);

    float cosR = cosf(rollRad);
    float sinR = sinf(rollRad);

    Vector2 l1 = { screenPos.x - cosR * r, screenPos.y - sinR * r };
    Vector2 l2 = { screenPos.x - cosR * (r + wingLen), screenPos.y - sinR * (r + wingLen) };
    DrawLineEx(l1, l2, 1.5f, color);

    Vector2 r1 = { screenPos.x + cosR * r, screenPos.y + sinR * r };
    Vector2 r2 = { screenPos.x + cosR * (r + wingLen), screenPos.y + sinR * (r + wingLen) };
    DrawLineEx(r1, r2, 1.5f, color);

    Vector2 f1 = { screenPos.x + sinR * r, screenPos.y - cosR * r };
    Vector2 f2 = { screenPos.x + sinR * (r + finH), screenPos.y - cosR * (r + finH) };
    DrawLineEx(f1, f2, 1.5f, color);
}

// 12. Cinta de Rumbo Horizontal Superior (Compass Tape)
void UI_DrawCompassTape(int centerX, int y, int width, float headingRad, Color color) {
    float headingDeg = headingRad * RAD2DEG;
    while (headingDeg < 0.0f) headingDeg += 360.0f;
    while (headingDeg >= 360.0f) headingDeg -= 360.0f;

    int halfW = width / 2;
    int x1 = centerX - halfW;
    int x2 = centerX + halfW;

    DrawLine(x1, y + 16, x2, y + 16, color);

    float pixelsPerDeg = (float)width / 60.0f;

    for (int deg = -40; deg <= 40; deg += 5) {
        float markDeg = headingDeg + (float)deg;
        float normDeg = fmodf(markDeg + 360.0f, 360.0f);
        int roundDeg = (int)roundf(normDeg);

        float px = (float)centerX + (float)deg * pixelsPerDeg;
        if (px < (float)x1 || px > (float)x2) continue;

        if (roundDeg % 30 == 0) {
            DrawLine((int)px, y + 6, (int)px, y + 16, color);

            const char *cardinal = "";
            if (roundDeg == 0 || roundDeg == 360) cardinal = "N";
            else if (roundDeg == 90)  cardinal = "E";
            else if (roundDeg == 180) cardinal = "S";
            else if (roundDeg == 270) cardinal = "W";
            else cardinal = TextFormat("%02d", roundDeg / 10);

            Vector2 sz = UI_MeasureTextHud(cardinal, 11.0f);
            UI_DrawTextHud(cardinal, px - sz.x * 0.5f, (float)(y - 6), 11.0f, color);
        } else if (roundDeg % 10 == 0) {
            DrawLine((int)px, y + 10, (int)px, y + 16, color);
        }
    }

    Vector2 pTop = { (float)centerX, (float)y + 19.0f };
    Vector2 pL   = { (float)centerX - 4.0f, (float)y + 25.0f };
    Vector2 pR   = { (float)centerX + 4.0f, (float)y + 25.0f };
    DrawTriangle(pTop, pL, pR, color);

    const char *hdgTxt = TextFormat("%03d*", (int)roundf(headingDeg));
    Vector2 hsz = UI_MeasureTextHud(hdgTxt, 12.0f);
    UI_DrawTextHud(hdgTxt, (float)centerX - hsz.x * 0.5f, (float)(y + 28), 12.0f, UI_COLOR_STEEL_WHITE);
}

// 13. Caja de Velocidad Táctica AC4
void UI_DrawSpeedBox(int x, int y, float speedKnots, float machNumber, Color color, float pump) {
    int w = 115 + (int)(pump * 4.0f);
    int h = 60;

    DrawLine(x, y, x + w, y, color);
    DrawLine(x, y + h, x + w, y + h, color);
    DrawLine(x + w, y, x + w, y + 14, color);
    DrawLine(x + w, y + h - 14, x + w, y + h, color);

    UI_DrawTextHud("CAS", (float)(x + 8), (float)(y + 6), 10.0f, (Color){ color.r, color.g, color.b, 170 });
    UI_DrawTextHud(TextFormat("%04d", (int)speedKnots), (float)(x + 8), (float)(y + 16), 24.0f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextHud("KTS", (float)(x + 82), (float)(y + 25), 11.0f, color);

    UI_DrawTextHud(TextFormat("M %.2f", machNumber), (float)(x + 8), (float)(y + 44), 11.0f, (Color){ color.r, color.g, color.b, 220 });
}

// 14. Caja de Altitud Táctica AC4 (WO3 + 2097 para Alerta)
void UI_DrawAltitudeBox(int x, int y, float altMSL, float altAGL, bool isWarning, Color color, Color alertColor, float pump) {
    int w = 120 + (int)(pump * 4.0f);
    int h = 60;

    Color curCol = isWarning ? alertColor : color;

    DrawLine(x, y, x + w, y, curCol);
    DrawLine(x, y + h, x + w, y + h, curCol);
    DrawLine(x, y, x, y + 14, curCol);
    DrawLine(x, y + h - 14, x, y + h, curCol);

    UI_DrawTextHud("ALT", (float)(x + 12), (float)(y + 6), 10.0f, (Color){ curCol.r, curCol.g, curCol.b, 170 });
    UI_DrawTextHud(TextFormat("%04d", (int)altMSL), (float)(x + 12), (float)(y + 16), 24.0f, UI_COLOR_STEEL_WHITE);
    UI_DrawTextHud("M", (float)(x + 88), (float)(y + 25), 11.0f, curCol);

    UI_DrawTextHud(TextFormat("R %03dm", (int)altAGL), (float)(x + 12), (float)(y + 44), 11.0f, (Color){ curCol.r, curCol.g, curCol.b, 220 });

    if (isWarning) {
        bool blink = ((int)(GetTime() * 8.0) % 2 == 0);
        if (blink) {
            DrawRectangle(x - 95, y + 18, 88, 24, (Color){ 180, 20, 30, 230 });
            DrawRectangleLines(x - 95, y + 18, 88, 24, alertColor);
            UI_DrawTextTitle("PULL UP", (float)(x - 85), (float)(y + 22), 12.0f, WHITE);
        }
    }
}

// 15. Barra de Empuje y Boost (WO3)
void UI_DrawThrustBar(int x, int y, int w, int h, float boostEnergy, float maxBoost, bool isAfterburner, bool isAirbrake, bool isDepleted, Color hudColor, Color warnColor) {
    DrawRectangleLines(x, y, w, h, (Color){ hudColor.r, hudColor.g, hudColor.b, 160 });

    float fraction = boostEnergy / maxBoost;
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    int totalSegments = 12;
    int filledSegments = (int)(fraction * (float)totalSegments);

    int segW = (w - 6) / totalSegments;
    int segH = h - 4;

    Color segColor = isDepleted ? warnColor : (isAfterburner ? UI_COLOR_AC4_AMBER : hudColor);

    for (int i = 0; i < filledSegments; i++) {
        DrawRectangle(x + 3 + i * segW, y + 2, segW - 1, segH, segColor);
    }

    const char *stateTxt = "MIL PWR";
    if (isAirbrake) stateTxt = "AIRBRAKE [RECHARGE]";
    else if (isDepleted) stateTxt = "BOOST EXHAUSTED";
    else if (isAfterburner) stateTxt = "AFTERBURNER";

    UI_DrawTextHud(stateTxt, (float)x, (float)(y + h + 4), 11.0f, segColor);
}

// 16. Barra de Energía Cinética (Kinetic Storage & Ground Effect)
void UI_DrawKineticEnergyBar(int x, int y, int w, int h, float kineticEnergy, float maxKinetic, float groundEffectRatio, bool isStalling, Color hudColor, Color warnColor) {
    DrawRectangleLines(x, y, w, h, (Color){ hudColor.r, hudColor.g, hudColor.b, 160 });

    float fraction = kineticEnergy / maxKinetic;
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    int totalSegments = 12;
    int filledSegments = (int)(fraction * (float)totalSegments);

    int segW = (w - 6) / totalSegments;
    int segH = h - 4;

    Color segColor = isStalling ? warnColor : (fraction < 0.25f ? UI_COLOR_AC4_AMBER : (groundEffectRatio > 0.8f ? UI_COLOR_AC4_CYAN : hudColor));

    for (int i = 0; i < filledSegments; i++) {
        DrawRectangle(x + 3 + i * segW, y + 2, segW - 1, segH, segColor);
    }

    const char *stateTxt = "KINETIC RESERVE";
    if (isStalling) stateTxt = "STALL // NO LIFT";
    else if (groundEffectRatio > 0.85f) stateTxt = "KE CHARGING [CUSHION]";
    else if (fraction < 0.25f) stateTxt = "KE CRITICAL [DIVE]";

    UI_DrawTextHud(TextFormat("KE %03d%%", (int)(fraction * 100.0f)), (float)x, (float)(y - 13), 11.0f, segColor);
    UI_DrawTextHud(stateTxt, (float)x, (float)(y + h + 4), 11.0f, segColor);
}

// 17. Badge de estado aerodinámico (Ground Effect Cushion & Stall Alert)
void UI_DrawGroundEffectBadge(int x, int y, float groundEffectRatio, bool isStalling, Color hudColor, Color alertColor) {
    if (isStalling) {
        bool blink = ((int)(GetTime() * 7.0) % 2 == 0);
        if (blink) {
            DrawRectangle(x - 90, y, 180, 24, (Color){ 180, 20, 30, 220 });
            DrawRectangleLines(x - 90, y, 180, 24, alertColor);
            UI_DrawTextTitle("STALL // RECOVER DECK", (float)(x - 80), (float)(y + 4), 11.0f, WHITE);
        }
    } else if (groundEffectRatio > 0.6f) {
        DrawRectangle(x - 90, y, 180, 20, (Color){ 6, 18, 30, 190 });
        DrawRectangleLines(x - 90, y, 180, 20, (Color){ hudColor.r, hudColor.g, hudColor.b, 180 });
        UI_DrawTextHud("GROUND EFFECT // ACTIVE CUSHION", (float)(x - 88), (float)(y + 3), 11.0f, hudColor);
    }
}

// ============================================================================
// HUD RADICAL THE DESIGNERS REPUBLIC // WIPEOUT 3 MINIMALIST CAD (SPRINT 4)
// ============================================================================

// 18. Marco Perimétrico CAD tDR con Cruces de Registro de 1px
void UI_DrawTDRCadFrame(int screenWidth, int screenHeight, Color color) {
    Color frameDim = (Color){ color.r, color.g, color.b, 75 };
    Color frameBright = (Color){ color.r, color.g, color.b, 160 };

    int marginX = (int)fmaxf(28.0f, (float)screenWidth * 0.025f);
    int marginY = (int)fmaxf(24.0f, (float)screenHeight * 0.035f);
    int tickLen = 14;

    // Corner 1: Top-Left [┌]
    DrawLine(marginX, marginY, marginX + tickLen, marginY, frameBright);
    DrawLine(marginX, marginY, marginX, marginY + tickLen, frameBright);

    // Corner 2: Top-Right [┐]
    DrawLine(screenWidth - marginX, marginY, screenWidth - marginX - tickLen, marginY, frameBright);
    DrawLine(screenWidth - marginX, marginY, screenWidth - marginX, marginY + tickLen, frameBright);

    // Corner 3: Bottom-Left [└]
    DrawLine(marginX, screenHeight - marginY, marginX + tickLen, screenHeight - marginY, frameBright);
    DrawLine(marginX, screenHeight - marginY, marginX, screenHeight - marginY - tickLen, frameBright);

    // Corner 4: Bottom-Right [┘]
    DrawLine(screenWidth - marginX, screenHeight - marginY, screenWidth - marginX - tickLen, screenHeight - marginY, frameBright);
    DrawLine(screenWidth - marginX, screenHeight - marginY, screenWidth - marginX, screenHeight - marginY - tickLen, frameBright);

    // Micro Cruces de Registro CAD [+]
    int midX = screenWidth / 2;
    int midY = screenHeight / 2;
    Color crossCol = (Color){ color.r, color.g, color.b, 55 };

    // Cruz superior central
    DrawLine(midX - 4, marginY, midX + 4, marginY, crossCol);
    DrawLine(midX, marginY - 4, midX, marginY + 4, crossCol);

    // Cruces de bordes laterales
    DrawLine(marginX - 4, midY, marginX + 4, midY, crossCol);
    DrawLine(marginX, midY - 4, marginX, midY + 4, crossCol);
    DrawLine(screenWidth - marginX - 4, midY, screenWidth - marginX + 4, midY, crossCol);
    DrawLine(screenWidth - marginX, midY - 4, screenWidth - marginX, midY + 4, crossCol);

    // Branding micro-vectorial tDR (sutil y limpio)
    UI_DrawTextHud("AEROSHEAR::NX // SYSTEM ACTIVE", (float)(marginX + 20), (float)(marginY - 4), 9.0f, frameDim);
    UI_DrawTextHud("TDR//VECTOR.CAL", (float)(midX - 44), (float)(marginY - 4), 9.0f, crossCol);
}

// 19. Velocímetro Digital tDR en KM/H con Barra Segmentada y Badges
void UI_DrawTDRSpeedometer(int x, int y, float speedKmh, float machNumber, float boostEnergy, float maxBoost,
                           bool isAfterburner, bool isAirbrake, bool isDepleted, bool isOverdrive,
                           Color hudColor, Color alertColor, float pump) {
    // Cápsula Glass Pod Reactiva Dark Future Blue
    Rectangle podRec = { (float)(x - 10), (float)(y - 7), 188.0f, 96.0f };
    Color podBody = (Color){ 5, 14, 26, 185 };
    Color podGlow = (Color){ hudColor.r, hudColor.g, hudColor.b, 80 };

    if (isDepleted) {
        podGlow = (Color){ alertColor.r, alertColor.g, alertColor.b, 190 };
        podBody = (Color){ 20, 8, 14, 195 };
    } else if (isOverdrive) {
        podGlow = (Color){ 160, 245, 255, 220 };
        podBody = (Color){ 8, 24, 46, 210 };
    } else if (isAfterburner) {
        podGlow = (Color){ UI_COLOR_AC4_AMBER.r, UI_COLOR_AC4_AMBER.g, UI_COLOR_AC4_AMBER.b, 175 };
    } else if (isAirbrake) {
        podGlow = (Color){ UI_COLOR_AC4_CYAN.r, UI_COLOR_AC4_CYAN.g, UI_COLOR_AC4_CYAN.b, 165 };
    }
    UI_DrawGlassPod(podRec, podGlow, podBody, 0.12f);

    // Encabezado técnico
    UI_DrawTextHud("VELOCITY // AIRSPEED", (float)x, (float)y, 9.0f, (Color){ hudColor.r, hudColor.g, hudColor.b, 140 });

    // Display gigante de velocidad en KM/H (WO3.ttf)
    int spd = (int)fmaxf(0.0f, speedKmh);
    const char *spdStr = TextFormat("%04d", spd);
    UI_DrawTextHud(spdStr, (float)x, (float)(y + 12), 36.0f, UI_COLOR_STEEL_WHITE);

    // Etiqueta de unidad y Mach a la derecha
    UI_DrawTextHud("KM/H", (float)(x + 118), (float)(y + 18), 13.0f, hudColor);
    UI_DrawTextHud(TextFormat("M %.2f", machNumber), (float)(x + 118), (float)(y + 34), 11.0f, (Color){ hudColor.r, hudColor.g, hudColor.b, 180 });

    // Barra Segmentada de Potencia (16 segmentos ultrafinos con micro-gaps)
    int barW = 168;
    int barH = 8;
    int barY = y + 54;
    DrawRectangle(x, barY, barW, barH, (Color){ 4, 10, 18, 180 });
    DrawRectangleLines(x, barY, barW, barH, (Color){ hudColor.r, hudColor.g, hudColor.b, 110 });

    float boostRatio = (maxBoost > 0.0f) ? Clamp(boostEnergy / maxBoost, 0.0f, 1.0f) : 0.0f;
    int numSegments = 16;
    int filled = (int)(boostRatio * (float)numSegments);
    int segW = (barW - 4) / numSegments;

    Color segCol = hudColor;
    if (isDepleted) segCol = alertColor;
    else if (isOverdrive) segCol = (Color){ 160, 245, 255, 255 };
    else if (isAfterburner) segCol = UI_COLOR_AC4_AMBER;
    else if (isAirbrake) segCol = UI_COLOR_AC4_CYAN;

    for (int i = 0; i < filled; i++) {
        int sx = x + 2 + i * segW;
        DrawRectangle(sx, barY + 1, segW - 1, barH - 2, segCol);
    }

    // Badge Dinámico de Régimen de Motor
    int badgeY = barY + 14;
    DrawRectangle(x, badgeY, barW, 18, (Color){ 5, 12, 20, 190 });
    DrawRectangleLines(x, badgeY, barW, 18, (Color){ hudColor.r, hudColor.g, hudColor.b, 85 });

    const char *modeText = "[THRUST // NOMINAL]";
    Color modeCol = (Color){ hudColor.r, hudColor.g, hudColor.b, 200 };

    if (isDepleted) {
        modeText = "[BOOST EXHAUSTED]";
        modeCol = alertColor;
    } else if (isOverdrive) {
        modeText = "[OVERDRIVE // SLNG]";
        modeCol = (Color){ 160, 245, 255, 255 };
    } else if (isAfterburner) {
        modeText = "[AFTERBURNER]";
        modeCol = UI_COLOR_AC4_AMBER;
    } else if (isAirbrake) {
        modeText = "[AIRBRAKE // VENT]";
        modeCol = UI_COLOR_AC4_CYAN;
    }

    Vector2 modeSz = UI_MeasureTextHud(modeText, 10.0f);
    UI_DrawTextHud(modeText, (float)x + ((float)barW - modeSz.x) * 0.5f, (float)(badgeY + 4), 10.0f, modeCol);
}

// 20. Módulo de Dinámica de Vuelo, Altimetría de Radar AGL y Reserva de KE
void UI_DrawTDRDynamics(int x, int y, float altitudeAGL, float altitudeMSL, float kineticEnergy, float maxKE,
                        bool inGroundShear, bool isStalling, Color hudColor, Color alertColor, float pump) {
    // Cápsula Glass Pod Reactiva Dark Future Blue
    Rectangle podRec = { (float)(x - 10), (float)(y - 7), 188.0f, 96.0f };
    Color podBody = (Color){ 5, 14, 26, 185 };
    Color podGlow = (Color){ hudColor.r, hudColor.g, hudColor.b, 80 };

    if (isStalling) {
        podGlow = (Color){ alertColor.r, alertColor.g, alertColor.b, 210 };
        podBody = (Color){ 22, 6, 12, 205 };
    } else if (inGroundShear) {
        podGlow = (Color){ UI_COLOR_AC4_CYAN.r, UI_COLOR_AC4_CYAN.g, UI_COLOR_AC4_CYAN.b, 200 };
        podBody = (Color){ 6, 22, 38, 205 };
    }
    UI_DrawGlassPod(podRec, podGlow, podBody, 0.12f);

    // Encabezado técnico
    UI_DrawTextHud("TERRAIN // DYNAMICS", (float)x, (float)y, 9.0f, (Color){ hudColor.r, hudColor.g, hudColor.b, 140 });

    // Display gigante de AGL en Metros (WO3.ttf)
    Color altValCol = inGroundShear ? UI_COLOR_AC4_CYAN : (isStalling ? alertColor : UI_COLOR_STEEL_WHITE);
    float aglSafe = fmaxf(0.0f, altitudeAGL);
    const char *aglStr = TextFormat("%04.1f", aglSafe);
    UI_DrawTextHud(aglStr, (float)x, (float)(y + 12), 36.0f, altValCol);

    // Etiqueta de unidad y MSL a la derecha
    UI_DrawTextHud("AGL", (float)(x + 118), (float)(y + 18), 13.0f, inGroundShear ? UI_COLOR_AC4_CYAN : hudColor);
    UI_DrawTextHud(TextFormat("MSL %03dm", (int)altitudeMSL), (float)(x + 118), (float)(y + 34), 11.0f, (Color){ hudColor.r, hudColor.g, hudColor.b, 180 });

    // Barra Segmentada de Energía Cinética (KE)
    int barW = 168;
    int barH = 8;
    int barY = y + 54;
    DrawRectangle(x, barY, barW, barH, (Color){ 4, 10, 18, 180 });
    DrawRectangleLines(x, barY, barW, barH, (Color){ hudColor.r, hudColor.g, hudColor.b, 110 });

    float keRatio = (maxKE > 0.0f) ? Clamp(kineticEnergy / maxKE, 0.0f, 1.0f) : 0.0f;
    int numSegments = 16;
    int filled = (int)(keRatio * (float)numSegments);
    int segW = (barW - 4) / numSegments;

    Color keSegCol = inGroundShear ? UI_COLOR_AC4_CYAN : (isStalling ? alertColor : (keRatio < 0.25f ? UI_COLOR_AC4_AMBER : hudColor));

    for (int i = 0; i < filled; i++) {
        int sx = x + 2 + i * segW;
        DrawRectangle(sx, barY + 1, segW - 1, barH - 2, keSegCol);
    }

    // Badge Dinámico de Ground-Shear / Reserva KE
    int badgeY = barY + 14;
    DrawRectangle(x, badgeY, barW, 18, (Color){ 5, 12, 20, 190 });
    DrawRectangleLines(x, badgeY, barW, 18, (Color){ hudColor.r, hudColor.g, hudColor.b, 85 });

    const char *keText = TextFormat("[KE // %03d%%]", (int)(keRatio * 100.0f));
    Color keTextCol = (Color){ hudColor.r, hudColor.g, hudColor.b, 200 };

    if (isStalling) {
        keText = "[STALL // RECOVER]";
        keTextCol = alertColor;
    } else if (inGroundShear) {
        keText = "[SHEAR // KE BOOST]";
        keTextCol = UI_COLOR_AC4_CYAN;
    }

    Vector2 keSz = UI_MeasureTextHud(keText, 10.0f);
    UI_DrawTextHud(keText, (float)x + ((float)barW - keSz.x) * 0.5f, (float)(badgeY + 4), 10.0f, keTextCol);
}

// 21. Director de Vuelo y Retícula Central tDR con Indicador de Derrape
void UI_DrawTDRFlightDirector(Vector2 center, float rollRad, float driftAngle, float lateralSlip,
                              bool airbrakeLeft, bool airbrakeRight, Color hudColor) {
    Color lineDim = (Color){ hudColor.r, hudColor.g, hudColor.b, 130 };
    Color lineSubtle = (Color){ hudColor.r, hudColor.g, hudColor.b, 70 };

    // 1. Retícula Central de 1px (-- . --)
    float gap = 7.0f;
    float lineLen = 14.0f;

    DrawLine((int)(center.x - gap - lineLen), (int)center.y, (int)(center.x - gap), (int)center.y, lineDim);
    DrawLine((int)(center.x + gap), (int)center.y, (int)(center.x + gap + lineLen), (int)center.y, lineDim);
    DrawRectangle((int)(center.x - 1.0f), (int)(center.y - 1.0f), 2, 2, UI_COLOR_STEEL_WHITE);

    // 2. Guía Vectorial de Alabeo sutil de 1px
    float cosR = cosf(rollRad * 0.35f);
    float sinR = sinf(rollRad * 0.35f);
    float hSpan = 28.0f;
    Vector2 hl1 = { center.x - cosR * hSpan, center.y - sinR * hSpan };
    Vector2 hl2 = { center.x + cosR * hSpan, center.y + sinR * hSpan };
    DrawLineEx(hl1, hl2, 1.0f, lineSubtle);

    // 3. Indicador de Deriva Lateral (Drift Slip Pip)
    float slipTrackY = center.y + 22.0f;
    float trackHalf = 36.0f;
    DrawLine((int)(center.x - trackHalf), (int)slipTrackY, (int)(center.x + trackHalf), (int)slipTrackY, lineSubtle);
    DrawLine((int)center.x, (int)(slipTrackY - 3.0f), (int)center.x, (int)(slipTrackY + 3.0f), lineSubtle);

    float slipOffset = Clamp(lateralSlip * 0.32f, -trackHalf, trackHalf);
    Color pipCol = (driftAngle > 5.0f) ? UI_COLOR_AC4_CYAN : lineDim;
    float pipX = center.x + slipOffset;

    // Pip en forma de diamante de precisión
    Vector2 pTop = { pipX, slipTrackY - 3.0f };
    Vector2 pBot = { pipX, slipTrackY + 3.0f };
    Vector2 pLeft = { pipX - 3.0f, slipTrackY };
    Vector2 pRight = { pipX + 3.0f, slipTrackY };
    DrawTriangle(pTop, pLeft, pBot, pipCol);
    DrawTriangle(pTop, pBot, pRight, pipCol);

    if (driftAngle > 6.0f) {
        const char *driftTxt = TextFormat("SLIP %.0f*", driftAngle);
        Vector2 dSz = UI_MeasureTextHud(driftTxt, 9.0f);
        UI_DrawTextHud(driftTxt, center.x - dSz.x * 0.5f, slipTrackY + 6.0f, 9.0f, UI_COLOR_AC4_CYAN);
    }

    // 4. Badges de Aerofreno Lateral Activo [L-AIR] / [R-AIR]
    int badgeW = 32;
    int badgeH = 14;
    int leftBadgeX = (int)(center.x - 72.0f);
    int rightBadgeX = (int)(center.x + 40.0f);
    int badgeY = (int)(center.y - 7.0f);

    // Aerofreno Izquierdo (Q)
    if (airbrakeLeft) {
        DrawRectangle(leftBadgeX, badgeY, badgeW, badgeH, (Color){ 0, 190, 240, 220 });
        DrawRectangleLines(leftBadgeX, badgeY, badgeW, badgeH, WHITE);
        UI_DrawTextHud("L-AIR", (float)(leftBadgeX + 3), (float)(badgeY + 2), 9.0f, UI_COLOR_VOID_BLACK);
    } else {
        DrawRectangleLines(leftBadgeX, badgeY, badgeW, badgeH, (Color){ hudColor.r, hudColor.g, hudColor.b, 60 });
        UI_DrawTextHud("L-AIR", (float)(leftBadgeX + 3), (float)(badgeY + 2), 9.0f, (Color){ hudColor.r, hudColor.g, hudColor.b, 90 });
    }

    // Aerofreno Derecho (E)
    if (airbrakeRight) {
        DrawRectangle(rightBadgeX, badgeY, badgeW, badgeH, (Color){ 0, 190, 240, 220 });
        DrawRectangleLines(rightBadgeX, badgeY, badgeW, badgeH, WHITE);
        UI_DrawTextHud("R-AIR", (float)(rightBadgeX + 3), (float)(badgeY + 2), 9.0f, UI_COLOR_VOID_BLACK);
    } else {
        DrawRectangleLines(rightBadgeX, badgeY, badgeW, badgeH, (Color){ hudColor.r, hudColor.g, hudColor.b, 60 });
        UI_DrawTextHud("R-AIR", (float)(rightBadgeX + 3), (float)(badgeY + 2), 9.0f, (Color){ hudColor.r, hudColor.g, hudColor.b, 90 });
    }
}

