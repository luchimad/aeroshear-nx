#ifndef SCENERY_H
#define SCENERY_H

#include "game_types.h"

// ============================================================================
// SISTEMA DE ESCENARIO Y VEGETACIÓN BILLBOARD (ÁRBOLES Y OBJETOS)
// ============================================================================

// Inicializa el sistema, carga las texturas de árboles y genera los props iniciales
void Scenery_Init(ScenerySystem *scenery, unsigned int seed);

// Carga o actualiza los props y vegetación según el bioma seleccionado
void Scenery_LoadBiome(ScenerySystem *scenery, BiomeType biome, unsigned int seed);

// Sincroniza y recicla chunks de árboles según la posición del jugador
void Scenery_Update(ScenerySystem *scenery, Vector3 playerPos);

// Fuerza la generación y centrado síncrono de props alrededor de la posición
void Scenery_ForceCenter(ScenerySystem *scenery, Vector3 playerPos);

// Ajusta dinámicamente la distancia de render y niebla de vegetación
void Scenery_SetRenderDistance(ScenerySystem *scenery, RenderDistance dist);

// Registra la posición de los checkpoints y el spawn para excluir props en sus inmediaciones
struct RaceTrack;
void Scenery_SetTrackClearance(ScenerySystem *scenery, const struct RaceTrack *race);

// Renderiza los árboles como billboards 3D orientados a la cámara con niebla de distancia
void Scenery_Draw(const ScenerySystem *scenery, const Camera3D *camera);

// Comprueba colisiones cilíndricas entre la nave y los props del escenario (árboles y edificios)
// Retorna true si ocurrió impacto fatal
bool Scenery_CheckCollisions(const ScenerySystem *scenery, PlayerJet *player, float dt);

// Libera texturas de árboles y recursos
void Scenery_Unload(ScenerySystem *scenery);

#endif // SCENERY_H
