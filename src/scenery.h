#ifndef SCENERY_H
#define SCENERY_H

#include "game_types.h"

// ============================================================================
// SISTEMA DE ESCENARIO Y VEGETACIÓN BILLBOARD (ÁRBOLES Y OBJETOS)
// ============================================================================

// Inicializa el sistema, carga las texturas de árboles y genera los props iniciales
void Scenery_Init(ScenerySystem *scenery);

// Carga o actualiza los props y vegetación según el bioma seleccionado
void Scenery_LoadBiome(ScenerySystem *scenery, BiomeType biome);

// Sincroniza y recicla chunks de árboles según la posición del jugador
void Scenery_Update(ScenerySystem *scenery, Vector3 playerPos);

// Ajusta dinámicamente la distancia de render y niebla de vegetación
void Scenery_SetRenderDistance(ScenerySystem *scenery, RenderDistance dist);

// Renderiza los árboles como billboards 3D orientados a la cámara con niebla de distancia
void Scenery_Draw(const ScenerySystem *scenery, const Camera3D *camera);

// Libera texturas de árboles y recursos
void Scenery_Unload(ScenerySystem *scenery);

#endif // SCENERY_H
