#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "game_types.h"
#include "aircraft.h"

// ============================================================================
// AEROSHEAR // FLIGHT CORE - PLAYER & AIRCRAFT CONTROLLER (ALPHA EDITION)
// ============================================================================

void Player_Init(PlayerJet *player);
void Player_ApplyAircraft(PlayerJet *player, const AircraftDefinition *aircraft);

// Actualiza controles (Teclado + Mando unificado), efecto suelo, KE y físicas de picada
void Player_Update(PlayerJet *player, const GameSettings *settings, float dt);

// Dibuja el sprite 2D Super Scaler con inclinación reactiva a viraje y picada por caída
void DrawPlayerSprite(const PlayerJet *player, const SpriteSheet *sheet, const Camera3D *camera, int viewWidth, int viewHeight);

#endif // PLAYER_H
