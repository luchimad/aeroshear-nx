#include "biome.h"
#include "raymath.h"

static BiomeType s_activeBiome = BIOME_DELTASTRAITS;

static const BiomeDefinition s_biomes[BIOME_COUNT] = {
    [BIOME_DELTASTRAITS] = {
        .type = BIOME_DELTASTRAITS,
        .id = "deltastraits",
        .name = "DELTA STRAITS",
        .subtitle = "BRAIDED RIVERS, ISLETS & COASTLINES",
        .description = "Intricate fluvial river network and coastal archipelago with crystal shallows, sandy banks, and lush islet flora.",
        .climate = "TROPICAL MARITIME / HIGH HUMIDITY",
        .skyZenithColor = (Color){ 35, 150, 230, 255 },
        .skyHorizonColor = (Color){ 170, 220, 240, 255 },
        .fogHorizonColor = (Color){ 170, 220, 240, 255 },
        .themeAccentColor = (Color){ 68, 224, 195, 255 },
        .sunDirection = (Vector3){ 0.50f, 0.80f, 0.32f },
        .maxHeight = 90.0f,
        .hasWater = true,
        .waterLevel = 45.0f,
        .fogStart = 650.0f,
        .fogEnd = 2100.0f
    },
    [BIOME_HOTSANDS] = {
        .type = BIOME_HOTSANDS,
        .id = "hotsands",
        .name = "HOT SANDS",
        .subtitle = "ENDLESS DUNES & DESERT HEAT",
        .description = "Vast arid desert expanse featuring wind-sculpted transverse dunes, golden heat haze, and sparse scrub.",
        .climate = "ARID / EXTREME HEAT 48*C",
        .skyZenithColor = (Color){ 225, 160, 85, 255 },
        .skyHorizonColor = (Color){ 245, 210, 165, 255 },
        .fogHorizonColor = (Color){ 245, 210, 165, 255 },
        .themeAccentColor = (Color){ 248, 172, 70, 255 },
        .sunDirection = (Vector3){ 0.30f, 0.92f, 0.25f },
        .maxHeight = 160.0f,
        .hasWater = false,
        .waterLevel = 0.0f,
        .fogStart = 550.0f,
        .fogEnd = 1900.0f
    }
};

int Biome_GetCount(void) {
    return BIOME_COUNT;
}

const BiomeDefinition* Biome_Get(BiomeType type) {
    if (type < 0 || type >= BIOME_COUNT) return &s_biomes[0];
    return &s_biomes[type];
}

void Biome_SetActive(BiomeType type) {
    if (type >= 0 && type < BIOME_COUNT) {
        s_activeBiome = type;
    }
}

BiomeType Biome_GetActive(void) {
    return s_activeBiome;
}
