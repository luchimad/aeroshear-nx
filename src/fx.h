#ifndef FX_H
#define FX_H

#include "raylib.h"
#include "game_types.h"
#include <stdbool.h>

// Inicializa el sistema de partículas atmosféricas y de suelo
void FX_Init(void);

// Actualiza partículas en el aire, partículas de suelo, estelas y líneas de velocidad
void FX_Update(const PlayerJet *player, float dt, bool isWaterBiome, bool isDesertBiome, float waterLevel);

// Renderiza los efectos 3D (partículas atmosféricas de aire y micro-partículas de suelo)
void FX_Draw3D(const Camera3D *camera);

// Dispara lluvia de chispas incandescentes de fricción metálica contra paredes
void FX_SpawnWallSparks(Vector3 position, Vector3 normal, int count);

// Renderiza la sombra elíptica proyectada y el halo de sustentación AG en el terreno
void FX_DrawPlayerShadow(const PlayerJet *player);

// Renderiza las estelas gemelas de plasma de los motores en 3D
void FX_DrawThrusterTrails(void);

// Renderiza los efectos 2D de velocidad en pantalla (Speed Streaks en Afterburner)
void FX_Draw2D(const PlayerJet *player, int screenWidth, int screenHeight);

// Efecto de encendido estilo CRT retro (Power-On sequence)
void FX_DrawCRTPowerOn(float timer, int screenWidth, int screenHeight);

// Efecto de túnel y tensión visual por fuerza G extrema (G-Force Screen Pull)
void FX_DrawGForcePull(float gLoad, int screenWidth, int screenHeight);

// Libera los recursos del sistema de FX
void FX_Unload(void);

#endif // FX_H
