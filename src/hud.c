#include "hud.h"
#include "config.h"
#include "ui_theme.h"
#include "ui_core.h"
#include "fx.h"
#include "raymath.h"
#include <math.h>

// ============================================================================
// AEROSHEAR // THE DESIGNERS REPUBLIC (tDR) IN-FLIGHT HUD // WIPEOUT 3 SPEC
// ============================================================================

void HUD_Draw(const PlayerJet *player, int screenWidth, int screenHeight) {
    if (!player) return;

    // Ritmo de respiración sutil
    float pump = (sinf((float)GetTime() * 4.5f) * 0.5f + 0.5f) * 0.20f;
    UITheme *theme = UITheme_Get();

    // Paleta de color táctica activa
    Color hudCol = theme->hudPrimary;
    Color hudDim = theme->hudDim;
    Color alertCol = theme->hudAlert;

    // Ligera sobre-luminosidad por bombeo rítmico
    if (theme->enableBeatPumping && pump > 0.05f) {
        hudCol.r = (unsigned char)fminf(255.0f, (float)hudCol.r + pump * 20.0f);
        hudCol.g = (unsigned char)fminf(255.0f, (float)hudCol.g + pump * 12.0f);
        hudCol.b = (unsigned char)fminf(255.0f, (float)hudCol.b + pump * 20.0f);
    }

    Vector2 screenCenter = { (float)screenWidth * 0.5f, (float)screenHeight * 0.5f };

    // ========================================================================
    // 0. G-FORCE SCREEN PULL (ESTRÉS DE CAMPO VISUAL EN ALTAS G)
    // ========================================================================
    float gLoad = 1.0f + fabsf(player->roll) * 1.8f + fabsf(player->pitch) * 1.5f;
    if (player->isAfterburner) gLoad += 0.5f;
    if (player->isAirbrake) gLoad += 0.3f;
    FX_DrawGForcePull(gLoad, screenWidth, screenHeight);

    // ========================================================================
    // 1. MARCO PERIMÉTRICO CAD DE 1PX Y MARCAS TÉCNICAS tDR
    // ========================================================================
    UI_DrawTDRCadFrame(screenWidth, screenHeight, hudCol);

    // ========================================================================
    // 2. RETÍCULA CENTRAL Y DIRECTOR DE DERIVA (CENTRIFUGAL DRIFT & AIRBRAKES)
    // ========================================================================
    Vector2 datumPos = screenCenter;
    datumPos.x += player->screenOffset.x * 0.30f;
    datumPos.y += player->screenOffset.y * 0.30f;
    UI_DrawTDRFlightDirector(datumPos, player->roll, player->driftAngle, player->lateralSlip,
                             player->airbrakeLeft, player->airbrakeRight, hudCol);

    // ========================================================================
    // 3. VELOCÍMETRO DIGITAL tDR EN KM/H (FLANCO INFERIOR IZQUIERDO)
    // ========================================================================
    int spdX = (int)fmaxf(36.0f, (float)screenWidth * 0.035f);
    int spdY = screenHeight - 125;
    bool isOverdrive = player->isAfterburner && (player->kineticEnergy > 20.0f);

    UI_DrawTDRSpeedometer(spdX, spdY, player->speedKmh, player->machNumber,
                          player->boostEnergy, player->maxBoostEnergy,
                          player->isAfterburner, player->isAirbrake, player->isBoostDepleted, isOverdrive,
                          hudCol, alertCol, pump);

    // ========================================================================
    // 4. MÓDULO DE DINÁMICAS, ALTIMETRÍA RADAR AGL Y KE (FLANCO INFERIOR DERECHO)
    // ========================================================================
    int dynW = 168;
    int dynX = screenWidth - dynW - (int)fmaxf(36.0f, (float)screenWidth * 0.035f);
    int dynY = screenHeight - 125;

    UI_DrawTDRDynamics(dynX, dynY, player->altitudeAGL, player->altitudeMSL,
                       player->kineticEnergy, player->maxKineticEnergy,
                       player->inGroundShear, player->isStalling,
                       hudCol, alertCol, pump);

    // ========================================================================
    // 5. GUÍA DE TELEMETRÍA Y CONTROLES MINIMALISTA (BORDES INFERIORES)
    // ========================================================================
    int botY = screenHeight - 24;

    // Telemetría de cabeceo / alabeo / carga G (inferior izquierdo)
    UI_DrawTextHud(TextFormat("PITCH %+02.0f*  ROLL %+03.0f*  G-LOAD %.1fG",
                              player->pitch * RAD2DEG, player->roll * RAD2DEG, gLoad),
                   (float)spdX, (float)botY, 9.0f, (Color){ hudDim.r, hudDim.g, hudDim.b, 150 });

    // Controles tácticos minimalistas (inferior derecho)
    const char *ctrlHint = "[SPACE]: BOOST   [Q/E]: DRIFT   [CTRL]: BRAKE";
    Vector2 cSz = UI_MeasureTextHud(ctrlHint, 9.0f);
    UI_DrawTextHud(ctrlHint, (float)(dynX + dynW) - cSz.x, (float)botY, 9.0f, (Color){ hudDim.r, hudDim.g, hudDim.b, 130 });
}
