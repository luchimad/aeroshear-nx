#ifndef BIOME_H
#define BIOME_H

#include "raylib.h"
#include "game_types.h"
#include <stdbool.h>

// ============================================================================
// MÓDULO DE DEFINICIÓN Y GESTIÓN DE BIOMAS (ALPHA EDITION: DELTA & HOTSANDS)
// ============================================================================

typedef struct BiomeDefinition {
    BiomeType type;
    const char *id;
    const char *name;
    const char *subtitle;
    const char *description;
    const char *climate;

    Color skyZenithColor;
    Color skyHorizonColor;
    Color fogHorizonColor;
    Color themeAccentColor;
    Vector3 sunDirection;

    float maxHeight;
    bool hasWater;
    float waterLevel;
    float fogStart;
    float fogEnd;
} BiomeDefinition;

int Biome_GetCount(void);
const BiomeDefinition* Biome_Get(BiomeType type);
void Biome_SetActive(BiomeType type);
BiomeType Biome_GetActive(void);

#endif // BIOME_H
