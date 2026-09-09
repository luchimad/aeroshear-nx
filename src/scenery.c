#include "scenery.h"
#include "terrain.h"
#include "config.h"
#include "biome.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>

// ============================================================================
// GENERADOR PROCEDURAL DE VEGETACIÓN Y PROPS POR BIOMA
// ============================================================================

static unsigned int SceneryHash(int x, int z, int seed) {
    unsigned int h = (unsigned int)(x * 374761393 + z * 668265263 + seed * 96245897);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

static float SceneryRandom(unsigned int *state) {
    *state = *state * 1664525u + 1013904223u;
    return (float)(*state & 0x00FFFFFF) / (float)0x00FFFFFF;
}

static void GenerateChunkProps(SceneryChunk *chunk, int chunkX, int chunkZ, BiomeType biome) {
    chunk->chunkX = chunkX;
    chunk->chunkZ = chunkZ;
    chunk->propCount = 0;
    chunk->isLoaded = true;

    unsigned int rng = SceneryHash(chunkX, chunkZ, 1337 + (int)biome * 7919);
    float chunkOriginX = (float)chunkX * TERRAIN_CHUNK_SIZE;
    float chunkOriginZ = (float)chunkZ * TERRAIN_CHUNK_SIZE;

    int candidates = MAX_PROPS_PER_CHUNK + 40;

    for (int i = 0; i < candidates && chunk->propCount < MAX_PROPS_PER_CHUNK; i++) {
        float localX = (SceneryRandom(&rng) - 0.5f) * (TERRAIN_CHUNK_SIZE - 20.0f);
        float localZ = (SceneryRandom(&rng) - 0.5f) * (TERRAIN_CHUNK_SIZE - 20.0f);
        float worldX = chunkOriginX + localX;
        float worldZ = chunkOriginZ + localZ;

        float groundY = Terrain_GetHeight(worldX, worldZ);
        Vector3 normal = Terrain_GetNormal(worldX, worldZ);
        float slope = 1.0f - normal.y;

        // 1. REGLAS PARA HOT SANDS (Arbustos del desierto en depresiones)
        if (biome == BIOME_HOTSANDS) {
            float desertCluster = sinf(worldX * 0.0025f) * cosf(worldZ * 0.0025f);
            float spawnChance = (desertCluster > 0.20f) ? 0.22f : 0.04f; // Vegetación árida muy dispersa
            if (SceneryRandom(&rng) > spawnChance) continue;

            float baseHeight = 4.5f + SceneryRandom(&rng) * 3.5f; // Arbusto de 4.5m a 8m
            float baseWidth  = baseHeight * (1.0f + SceneryRandom(&rng) * 0.4f);
            int texIdx = (SceneryRandom(&rng) > 0.5f) ? 1 : 0; // Bush 1 o Bush 2

            BillboardProp *prop = &chunk->props[chunk->propCount++];
            prop->width = baseWidth;
            prop->height = baseHeight;
            prop->textureIndex = texIdx;
            prop->position = (Vector3){ worldX, groundY + baseHeight * 0.5f, worldZ };
            prop->randomRotation = SceneryRandom(&rng) * 360.0f;
        }
        // ====================================================================
        // 3. REGLAS PARA DELTA STRAITS (Palmeras/Árboles SOLO en pasto de islas)
        // ====================================================================
        else if (biome == BIOME_DELTASTRAITS) {
            // Estrictamente por encima del agua (45m) y de la playa (48.5m)
            if (groundY < 49.0f) continue;
            if (slope > SCENERY_MAX_SLOPE) continue;

            float islandNoise = sinf(worldX * 0.004f + 3.1f) * cosf(worldZ * 0.004f + 1.2f);
            float spawnChance = (islandNoise > 0.0f) ? 0.70f : 0.15f;
            if (SceneryRandom(&rng) > spawnChance) continue;

            float baseHeight = 18.0f + SceneryRandom(&rng) * 12.0f;
            float baseWidth  = baseHeight * (0.65f + SceneryRandom(&rng) * 0.20f);

            float tChoice = SceneryRandom(&rng);
            int texIdx = (tChoice < 0.34f) ? 0 : ((tChoice < 0.67f) ? 1 : 2);

            BillboardProp *prop = &chunk->props[chunk->propCount++];
            prop->width = baseWidth;
            prop->height = baseHeight;
            prop->textureIndex = texIdx;
            prop->position = (Vector3){ worldX, groundY + baseHeight * 0.5f, worldZ };
            prop->randomRotation = SceneryRandom(&rng) * 360.0f;
        }
    }
}

// ============================================================================
// INICIALIZACIÓN Y GESTIÓN DE VEGETACIÓN
// ============================================================================

void Scenery_Init(ScenerySystem *scenery) {
    scenery->currentBiome = BIOME_DELTASTRAITS;

    // Cargar texturas de árboles y vegetación
    scenery->texTree1 = LoadTexture(PATH_TEX_TREE_1);
    scenery->texTree2 = LoadTexture(PATH_TEX_TREE_2);
    scenery->texTree3 = LoadTexture(PATH_TEX_TREE_3);
    scenery->texBush1 = LoadTexture(PATH_TEX_DESERT_BUSH_1);
    scenery->texBush2 = LoadTexture(PATH_TEX_DESERT_BUSH_2);

    SetTextureFilter(scenery->texTree1, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texTree2, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texTree3, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBush1, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBush2, TEXTURE_FILTER_POINT);

    scenery->centerChunkX = 0;
    scenery->centerChunkZ = 0;

    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int chunkIdx = 0;
    for (int gz = -halfGrid; gz <= halfGrid; gz++) {
        for (int gx = -halfGrid; gx <= halfGrid; gx++) {
            SceneryChunk *chunk = &scenery->chunks[chunkIdx++];
            GenerateChunkProps(chunk, gx, gz, scenery->currentBiome);
        }
    }
}

void Scenery_LoadBiome(ScenerySystem *scenery, BiomeType biome) {
    scenery->currentBiome = biome;
    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int chunkIdx = 0;

    for (int gz = -halfGrid; gz <= halfGrid; gz++) {
        for (int gx = -halfGrid; gx <= halfGrid; gx++) {
            SceneryChunk *chunk = &scenery->chunks[chunkIdx++];
            GenerateChunkProps(chunk, gx, gz, biome);
        }
    }
}

// ============================================================================
// STREAMING DE CHUNKS DE VEGETACIÓN
// ============================================================================

void Scenery_Update(ScenerySystem *scenery, Vector3 playerPos) {
    int currentCenterX = (int)roundf(playerPos.x / TERRAIN_CHUNK_SIZE);
    int currentCenterZ = (int)roundf(playerPos.z / TERRAIN_CHUNK_SIZE);

    if (currentCenterX == scenery->centerChunkX && currentCenterZ == scenery->centerChunkZ) {
        return;
    }

    scenery->centerChunkX = currentCenterX;
    scenery->centerChunkZ = currentCenterZ;

    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int minX = currentCenterX - halfGrid;
    int maxX = currentCenterX + halfGrid;
    int minZ = currentCenterZ - halfGrid;
    int maxZ = currentCenterZ + halfGrid;

    bool needed[TERRAIN_CHUNK_GRID][TERRAIN_CHUNK_GRID];
    for (int r = 0; r < TERRAIN_CHUNK_GRID; r++) {
        for (int c = 0; c < TERRAIN_CHUNK_GRID; c++) {
            needed[r][c] = true;
        }
    }

    for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
        SceneryChunk *chunk = &scenery->chunks[i];
        if (chunk->chunkX >= minX && chunk->chunkX <= maxX &&
            chunk->chunkZ >= minZ && chunk->chunkZ <= maxZ) {
            int gridRow = chunk->chunkZ - minZ;
            int gridCol = chunk->chunkX - minX;
            if (gridRow >= 0 && gridRow < TERRAIN_CHUNK_GRID && gridCol >= 0 && gridCol < TERRAIN_CHUNK_GRID) {
                needed[gridRow][gridCol] = false;
            }
        }
    }

    for (int r = 0; r < TERRAIN_CHUNK_GRID; r++) {
        for (int c = 0; c < TERRAIN_CHUNK_GRID; c++) {
            if (needed[r][c]) {
                int targetChunkX = minX + c;
                int targetChunkZ = minZ + r;

                for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
                    SceneryChunk *chunk = &scenery->chunks[i];
                    if (chunk->chunkX < minX || chunk->chunkX > maxX ||
                        chunk->chunkZ < minZ || chunk->chunkZ > maxZ) {
                        GenerateChunkProps(chunk, targetChunkX, targetChunkZ, scenery->currentBiome);
                        break;
                    }
                }
            }
        }
    }
}

static RenderDistance s_sceneryRenderDist = RENDER_DIST_LOW;

void Scenery_SetRenderDistance(ScenerySystem *scenery, RenderDistance dist) {
    (void)scenery;
    s_sceneryRenderDist = dist;
}

// ============================================================================
// RENDERIZADO 3D DE VEGETACIÓN (BILLBOARDS CON NIEBLA)
// ============================================================================

void Scenery_Draw(const ScenerySystem *scenery, const Camera3D *camera) {
    const BiomeDefinition *bDef = Biome_Get(scenery->currentBiome);
    Color fogCol = bDef->fogHorizonColor;
    float maxDist  = (s_sceneryRenderDist == RENDER_DIST_HIGH) ? SCENERY_RENDER_DIST_HIGH : SCENERY_RENDER_DIST_LOW;
    float fogStart = (s_sceneryRenderDist == RENDER_DIST_HIGH) ? TERRAIN_FOG_START_HIGH : bDef->fogStart;
    float fogEnd   = (s_sceneryRenderDist == RENDER_DIST_HIGH) ? TERRAIN_FOG_END_HIGH   : bDef->fogEnd;

    for (int c = 0; c < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; c++) {
        const SceneryChunk *chunk = &scenery->chunks[c];
        if (!chunk->isLoaded) continue;

        for (int p = 0; p < chunk->propCount; p++) {
            const BillboardProp *prop = &chunk->props[p];

            float dist = Vector3Distance(camera->position, prop->position);
            if (dist > maxDist) continue;

            // Niebla atmosférica suave hacia el horizonte
            float rawFog = (dist - fogStart) / (fogEnd - fogStart);
            if (rawFog < 0.0f) rawFog = 0.0f;
            if (rawFog > 1.0f) rawFog = 1.0f;
            float fogFactor = rawFog * rawFog * (3.0f - 2.0f * rawFog);

            Color propTint = WHITE;
            if (fogFactor > 0.01f) {
                propTint.r = (unsigned char)Lerp((float)WHITE.r, (float)fogCol.r, fogFactor);
                propTint.g = (unsigned char)Lerp((float)WHITE.g, (float)fogCol.g, fogFactor);
                propTint.b = (unsigned char)Lerp((float)WHITE.b, (float)fogCol.b, fogFactor);
                propTint.a = (unsigned char)Lerp(255.0f, (float)fogCol.a * 0.2f, fogFactor * 0.8f);
            }

            // Seleccionar textura según el bioma y el índice del prop
            Texture2D drawTex;
            if (scenery->currentBiome == BIOME_HOTSANDS) {
                drawTex = (prop->textureIndex == 0) ? scenery->texBush1 : scenery->texBush2;
            } else {
                if (prop->textureIndex == 0)      drawTex = scenery->texTree1;
                else if (prop->textureIndex == 1) drawTex = scenery->texTree2;
                else                              drawTex = scenery->texTree3;
            }

            if (drawTex.id != 0) {
                DrawBillboard(*camera, drawTex, prop->position, prop->height, propTint);
            }
        }
    }
}

// ============================================================================
// LIBERACIÓN DE RECURSOS
// ============================================================================

void Scenery_Unload(ScenerySystem *scenery) {
    if (scenery->texTree1.id != 0) UnloadTexture(scenery->texTree1);
    if (scenery->texTree2.id != 0) UnloadTexture(scenery->texTree2);
    if (scenery->texTree3.id != 0) UnloadTexture(scenery->texTree3);
    if (scenery->texBush1.id != 0) UnloadTexture(scenery->texBush1);
    if (scenery->texBush2.id != 0) UnloadTexture(scenery->texBush2);
}
