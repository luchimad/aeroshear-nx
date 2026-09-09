#ifndef TERRAIN_H
#define TERRAIN_H

#include "game_types.h"

// ============================================================================
// MÓDULO DE TERRENO PROCEDURAL // GENERACIÓN INFINITA Y SHADER MULTI-TEXTURA
// ============================================================================

// Calcula la altura matemática del terreno en cualquier punto del mundo (X, Z)
float Terrain_GetHeight(float worldX, float worldZ);

// Calcula la normal matemática aproximada del terreno en (X, Z)
Vector3 Terrain_GetNormal(float worldX, float worldZ);

// Inicializa el sistema de terreno, carga texturas del bioma activo y genera la grilla inicial
void Terrain_Init(TerrainSystem *terrain);

// Carga o cambia las texturas, shaders y parámetros del bioma seleccionado
void Terrain_LoadBiome(TerrainSystem *terrain, BiomeType biome);

// Ajusta dinámicamente los rangos de niebla y horizonte según la distancia de render elegida
void Terrain_SetRenderDistance(TerrainSystem *terrain, RenderDistance dist);

// Actualiza la posición de la grilla de chunks alrededor del jugador (streaming dinámico)
void Terrain_Update(TerrainSystem *terrain, Vector3 playerPos);

// Renderiza todos los chunks visibles con el shader de terreno en espacio 3D
void Terrain_Draw(TerrainSystem *terrain, const Camera3D *camera);

// Libera mallas, modelos, texturas y shaders del sistema de terreno
void Terrain_Unload(TerrainSystem *terrain);

#endif // TERRAIN_H
