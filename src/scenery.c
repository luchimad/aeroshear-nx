#include "scenery.h"
#include "terrain.h"
#include "config.h"
#include "biome.h"
#include "race.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// GENERADOR PROCEDURAL DE VEGETACIÓN Y PROPS POR BIOMA
// ============================================================================

static const char *s_alphaDiscardFs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "void main() {\n"
    "    vec4 texelColor = texture(texture0, fragTexCoord);\n"
    "    if (texelColor.a < 0.15) discard;\n"
    "    finalColor = texelColor * fragColor;\n"
    "}\n";

static unsigned int SceneryHash(int x, int z, int seed) {
    unsigned int h = (unsigned int)(x * 374761393 + z * 668265263 + seed * 96245897);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

static float SceneryRandom(unsigned int *state) {
    *state = *state * 1664525u + 1013904223u;
    return (float)(*state & 0x00FFFFFF) / (float)0x00FFFFFF;
}

static void GenerateChunkProps(SceneryChunk *chunk, int chunkX, int chunkZ, const ScenerySystem *scenery) {
    chunk->chunkX = chunkX;
    chunk->chunkZ = chunkZ;
    chunk->propCount = 0;
    chunk->isLoaded = true;

    BiomeType biome = scenery->currentBiome;
    unsigned int seed = scenery->currentSeed;

    unsigned int rng = SceneryHash(chunkX, chunkZ, (int)seed + (int)biome * 7919);
    float chunkOriginX = (float)chunkX * TERRAIN_CHUNK_SIZE;
    float chunkOriginZ = (float)chunkZ * TERRAIN_CHUNK_SIZE;

    int candidates = MAX_PROPS_PER_CHUNK + 20;

    for (int i = 0; i < candidates && chunk->propCount < MAX_PROPS_PER_CHUNK; i++) {
        float localX = (SceneryRandom(&rng) - 0.5f) * (TERRAIN_CHUNK_SIZE - 20.0f);
        float localZ = (SceneryRandom(&rng) - 0.5f) * (TERRAIN_CHUNK_SIZE - 20.0f);
        float worldX = chunkOriginX + localX;
        float worldZ = chunkOriginZ + localZ;

        // 0. EXCLUSIÓN DE CIRCUITOS: Impedir que aparezcan edificios o árboles en puertas o spawn
        if (scenery->hasTrackClearance) {
            // Radio de seguridad circular amplio alrededor del spawn (500m)
            float dxSpawn = worldX - scenery->spawnPosition.x;
            float dzSpawn = worldZ - scenery->spawnPosition.z;
            if (dxSpawn * dxSpawn + dzSpawn * dzSpawn < 500.0f * 500.0f) {
                continue;
            }

            // Exclusión de corredor entre Spawn y la Primer Gate (Checkpoint 0)
            if (scenery->checkpointCount > 0) {
                Vector3 pA = scenery->spawnPosition;
                Vector3 pB = scenery->checkpointPositions[0];
                float segX = pB.x - pA.x;
                float segZ = pB.z - pA.z;
                float segLenSq = segX * segX + segZ * segZ;
                if (segLenSq > 1.0f) {
                    float t = ((worldX - pA.x) * segX + (worldZ - pA.z) * segZ) / segLenSq;
                    if (t >= -0.05f && t <= 1.05f) {
                        float projX = pA.x + t * segX;
                        float projZ = pA.z + t * segZ;
                        float dX = worldX - projX;
                        float dZ = worldZ - projZ;
                        // Pasillo libre de 400m de ancho lateral entre el spawn y la primer gate
                        if (dX * dX + dZ * dZ < 400.0f * 400.0f) {
                            continue;
                        }
                    }
                }
            }

            // Exclusión total de 1 km (1000m) a la redonda de CUALQUIER gate
            bool nearGate = false;
            for (int k = 0; k < scenery->checkpointCount; k++) {
                Vector3 cpPos = scenery->checkpointPositions[k];
                float dx = worldX - cpPos.x;
                float dz = worldZ - cpPos.z;
                float distSq = dx * dx + dz * dz;

                // Radio de seguridad estricto de 1000m (1km)
                if (distSq < 1000.0f * 1000.0f) {
                    nearGate = true;
                    break;
                }
            }
            if (nearGate) continue;
        }

        float groundY = Terrain_GetHeight(worldX, worldZ);
        Vector3 normal = Terrain_GetNormal(worldX, worldZ);
        float slope = 1.0f - normal.y;

        // 1. REGLAS PARA HOT SANDS (Ciudad en ruinas al 25% de densidad y más desperdigada + arbustos)
        if (biome == BIOME_HOTSANDS) {
            float cityNoise = sinf(worldX * 0.0015f + 1.3f) * cosf(worldZ * 0.0015f + 0.9f);

            // Si estamos en un sector de ruinas urbanas
            if (cityNoise > 0.22f) {
                // Densidad reducida a un 25% de la previa (0.75f / 0.48f -> 0.18f / 0.11f)
                float spawnChance = (cityNoise > 0.45f) ? 0.18f : 0.11f;
                if (SceneryRandom(&rng) > spawnChance) continue;

                // Edificios más desperdigados: verificación de distancia mínima entre props
                const float MIN_BUILDING_DIST = 38.0f;
                bool tooClose = false;
                for (int p = 0; p < chunk->propCount; p++) {
                    float pdx = chunk->props[p].position.x - worldX;
                    float pdz = chunk->props[p].position.z - worldZ;
                    if (pdx * pdx + pdz * pdz < MIN_BUILDING_DIST * MIN_BUILDING_DIST) {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose) continue;

                bool isTall = (SceneryRandom(&rng) < 0.38f);
                float baseHeight;
                float baseWidth;
                int texIdx;

                if (isTall) {
                    baseHeight = 110.0f + SceneryRandom(&rng) * 70.0f; // 110m a 180m de altura
                    texIdx = (SceneryRandom(&rng) > 0.5f) ? 5 : 4; // building_tall_1 o 2
                    float texAspect = (texIdx == 4) ? (295.0f / 505.0f) : (262.0f / 512.0f);
                    baseWidth = baseHeight * texAspect;
                } else {
                    baseHeight = 48.0f + SceneryRandom(&rng) * 32.0f;  // 48m a 80m de altura
                    texIdx = (SceneryRandom(&rng) > 0.5f) ? 3 : 2; // building_small_1 o 2
                    float texAspect = (texIdx == 2) ? 1.0f : (350.0f / 340.0f);
                    baseWidth = baseHeight * texAspect;
                }

                BillboardProp *prop = &chunk->props[chunk->propCount++];
                prop->width = baseWidth;
                prop->height = baseHeight;
                prop->textureIndex = texIdx;
                prop->position = (Vector3){ worldX, groundY + baseHeight * 0.5f, worldZ };
                prop->randomRotation = SceneryRandom(&rng) * 360.0f;
                prop->category = PROP_CATEGORY_BUILDING;
                // Semi-ancho frontal sólido del edificio (76% del ancho visible del sprite)
                prop->collisionRadius = baseWidth * 0.38f;
            } else {
                // Arbustos dispersos en las dunas
                float desertCluster = sinf(worldX * 0.0025f) * cosf(worldZ * 0.0025f);
                float spawnChance = (desertCluster > 0.20f) ? 0.16f : 0.02f;
                if (SceneryRandom(&rng) > spawnChance) continue;

                float baseHeight = 4.5f + SceneryRandom(&rng) * 3.5f;
                float baseWidth  = baseHeight * (1.0f + SceneryRandom(&rng) * 0.4f);
                int texIdx = (SceneryRandom(&rng) > 0.5f) ? 1 : 0;

                BillboardProp *prop = &chunk->props[chunk->propCount++];
                prop->width = baseWidth;
                prop->height = baseHeight;
                prop->textureIndex = texIdx;
                prop->position = (Vector3){ worldX, groundY + baseHeight * 0.5f, worldZ };
                prop->randomRotation = SceneryRandom(&rng) * 360.0f;
                prop->category = PROP_CATEGORY_BUSH;
                prop->collisionRadius = 0.0f;
            }
        }
        // ====================================================================
        // 2. REGLAS PARA DELTA STRAITS (Palmeras/Árboles SOLO en pasto de islas)
        // ====================================================================
        else if (biome == BIOME_DELTASTRAITS) {
            // Estrictamente por encima del agua (45m) y de la playa con buen margen (50m)
            if (groundY < 50.0f) continue;
            if (slope > SCENERY_MAX_SLOPE) continue;

            float islandNoise = sinf(worldX * 0.004f + 3.1f) * cosf(worldZ * 0.004f + 1.2f);
            float spawnChance = (islandNoise > 0.0f) ? 0.55f : 0.12f;
            if (SceneryRandom(&rng) > spawnChance) continue;

            // Distancia mínima entre árboles para evitar amontonamiento
            const float MIN_TREE_DIST = 18.0f;
            bool tooClose = false;
            for (int p = 0; p < chunk->propCount; p++) {
                float pdx = chunk->props[p].position.x - worldX;
                float pdz = chunk->props[p].position.z - worldZ;
                if (pdx * pdx + pdz * pdz < MIN_TREE_DIST * MIN_TREE_DIST) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

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
            prop->category = PROP_CATEGORY_TREE;
            prop->collisionRadius = baseWidth * 0.18f; // Hitbox ajustada al tronco
        }
    }
}

// ============================================================================
// INICIALIZACIÓN Y GESTIÓN DE VEGETACIÓN
// ============================================================================

void Scenery_Init(ScenerySystem *scenery, unsigned int seed) {
    scenery->currentBiome = BIOME_DELTASTRAITS;
    scenery->currentSeed = seed;
    scenery->hasTrackClearance = false;
    scenery->checkpointCount = 0;
    scenery->spawnPosition = (Vector3){ 0 };

    // Cargar shader de recorte alpha para evitar oclusión Z de fondo en zonas transparentes
    scenery->billboardShader = LoadShaderFromMemory(NULL, s_alphaDiscardFs);

    // Cargar texturas de árboles y vegetación
    scenery->texTree1 = LoadTexture(PATH_TEX_TREE_1);
    scenery->texTree2 = LoadTexture(PATH_TEX_TREE_2);
    scenery->texTree3 = LoadTexture(PATH_TEX_TREE_3);
    scenery->texBush1 = LoadTexture(PATH_TEX_DESERT_BUSH_1);
    scenery->texBush2 = LoadTexture(PATH_TEX_DESERT_BUSH_2);

    // Cargar nuevas texturas de edificios para bioma desierto (ruinas urbanas)
    scenery->texBuildSmall1 = LoadTexture(PATH_TEX_BUILD_SMALL_1);
    scenery->texBuildSmall2 = LoadTexture(PATH_TEX_BUILD_SMALL_2);
    scenery->texBuildTall1  = LoadTexture(PATH_TEX_BUILD_TALL_1);
    scenery->texBuildTall2  = LoadTexture(PATH_TEX_BUILD_TALL_2);

    SetTextureFilter(scenery->texTree1, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texTree2, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texTree3, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBush1, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBush2, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBuildSmall1, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBuildSmall2, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBuildTall1, TEXTURE_FILTER_POINT);
    SetTextureFilter(scenery->texBuildTall2, TEXTURE_FILTER_POINT);

    scenery->centerChunkX = 0;
    scenery->centerChunkZ = 0;

    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int chunkIdx = 0;
    for (int gz = -halfGrid; gz <= halfGrid; gz++) {
        for (int gx = -halfGrid; gx <= halfGrid; gx++) {
            SceneryChunk *chunk = &scenery->chunks[chunkIdx++];
            GenerateChunkProps(chunk, gx, gz, scenery);
        }
    }
}

void Scenery_LoadBiome(ScenerySystem *scenery, BiomeType biome, unsigned int seed) {
    scenery->currentBiome = biome;
    scenery->currentSeed = seed;
    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int chunkIdx = 0;

    for (int gz = -halfGrid; gz <= halfGrid; gz++) {
        for (int gx = -halfGrid; gx <= halfGrid; gx++) {
            SceneryChunk *chunk = &scenery->chunks[chunkIdx++];
            GenerateChunkProps(chunk, gx, gz, scenery);
        }
    }
}

void Scenery_SetTrackClearance(ScenerySystem *scenery, const struct RaceTrack *race) {
    if (!scenery || !race) return;
    scenery->checkpointCount = (race->totalCheckpoints > RACE_TOTAL_CHECKPOINTS) ? RACE_TOTAL_CHECKPOINTS : race->totalCheckpoints;
    for (int i = 0; i < scenery->checkpointCount; i++) {
        scenery->checkpointPositions[i] = race->checkpoints[i].position;
        scenery->checkpointDirections[i] = race->checkpoints[i].direction;
    }
    scenery->spawnPosition = race->spawnPosition;
    scenery->hasTrackClearance = true;

    // Regenerar chunks cargados para limpiar cualquier prop que hubiese quedado cerca de una puerta
    for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
        SceneryChunk *chunk = &scenery->chunks[i];
        if (chunk->isLoaded) {
            GenerateChunkProps(chunk, chunk->chunkX, chunk->chunkZ, scenery);
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
                        GenerateChunkProps(chunk, targetChunkX, targetChunkZ, scenery);
                        break;
                    }
                }
            }
        }
    }
}

void Scenery_ForceCenter(ScenerySystem *scenery, Vector3 playerPos) {
    int currentCenterX = (int)roundf(playerPos.x / TERRAIN_CHUNK_SIZE);
    int currentCenterZ = (int)roundf(playerPos.z / TERRAIN_CHUNK_SIZE);

    scenery->centerChunkX = currentCenterX;
    scenery->centerChunkZ = currentCenterZ;

    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int chunkIdx = 0;

    for (int gz = -halfGrid; gz <= halfGrid; gz++) {
        for (int gx = -halfGrid; gx <= halfGrid; gx++) {
            if (chunkIdx < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID) {
                SceneryChunk *chunk = &scenery->chunks[chunkIdx++];
                int targetX = currentCenterX + gx;
                int targetZ = currentCenterZ + gz;
                GenerateChunkProps(chunk, targetX, targetZ, scenery);
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
// RENDERIZADO 3D DE VEGETACIÓN (BILLBOARDS CON NIEBLA Y ALPHA DISCARD)
// ============================================================================

void Scenery_Draw(const ScenerySystem *scenery, const Camera3D *camera) {
    const BiomeDefinition *bDef = Biome_Get(scenery->currentBiome);
    Color fogCol = bDef->fogHorizonColor;
    float maxDist  = (s_sceneryRenderDist == RENDER_DIST_HIGH) ? SCENERY_RENDER_DIST_HIGH : SCENERY_RENDER_DIST_LOW;
    float fogStart = (s_sceneryRenderDist == RENDER_DIST_HIGH) ? TERRAIN_FOG_START_HIGH : bDef->fogStart;
    float fogEnd   = (s_sceneryRenderDist == RENDER_DIST_HIGH) ? TERRAIN_FOG_END_HIGH   : bDef->fogEnd;

    if (scenery->billboardShader.id != 0) {
        BeginShaderMode(scenery->billboardShader);
    }

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
            Texture2D drawTex = { 0 };
            if (scenery->currentBiome == BIOME_HOTSANDS) {
                if (prop->textureIndex == 0)      drawTex = scenery->texBush1;
                else if (prop->textureIndex == 1) drawTex = scenery->texBush2;
                else if (prop->textureIndex == 2) drawTex = scenery->texBuildSmall1;
                else if (prop->textureIndex == 3) drawTex = scenery->texBuildSmall2;
                else if (prop->textureIndex == 4) drawTex = scenery->texBuildTall1;
                else if (prop->textureIndex == 5) drawTex = scenery->texBuildTall2;
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

    if (scenery->billboardShader.id != 0) {
        EndShaderMode();
    }
}

// ============================================================================
// DETECCIÓN DE COLISIONES DE PROPS (ÁRBOLES Y EDIFICIOS)
// SISTEMA CONTINUO (SWEPT-SEGMENT CCD) + HITBOX ORIENTADA A BILLBOARD
// ============================================================================

bool Scenery_CheckCollisions(const ScenerySystem *scenery, PlayerJet *player, float dt) {
    (void)dt;
    if (player->isDead) return true;

    Vector3 pPrev = player->prevPosition;
    Vector3 pCurr = player->position;

    // Envolvente de chunks que cubre todo el segmento de vuelo entre t-1 y t
    int minChunkX = (int)roundf(fminf(pPrev.x, pCurr.x) / TERRAIN_CHUNK_SIZE) - 1;
    int maxChunkX = (int)roundf(fmaxf(pPrev.x, pCurr.x) / TERRAIN_CHUNK_SIZE) + 1;
    int minChunkZ = (int)roundf(fminf(pPrev.z, pCurr.z) / TERRAIN_CHUNK_SIZE) - 1;
    int maxChunkZ = (int)roundf(fmaxf(pPrev.z, pCurr.z) / TERRAIN_CHUNK_SIZE) + 1;

    const float shipRadius = 3.2f;
    const float shipHalfH  = 2.6f;

    for (int c = 0; c < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; c++) {
        const SceneryChunk *chunk = &scenery->chunks[c];
        if (!chunk->isLoaded) continue;

        // Comprobar únicamente chunks dentro del corredor de paso del caza
        if (chunk->chunkX < minChunkX || chunk->chunkX > maxChunkX ||
            chunk->chunkZ < minChunkZ || chunk->chunkZ > maxChunkZ) {
            continue;
        }

        for (int p = 0; p < chunk->propCount; p++) {
            const BillboardProp *prop = &chunk->props[p];
            if (prop->category == PROP_CATEGORY_BUSH || prop->collisionRadius <= 0.0f) {
                continue;
            }

            // En Delta Straits no existen edificios (salvaguarda ante colisiones fantasma)
            if (prop->category == PROP_CATEGORY_BUILDING && scenery->currentBiome == BIOME_DELTASTRAITS) {
                continue;
            }

            // Comprobación vertical rápida de envolvente
            float propBottom = prop->position.y - prop->height * 0.5f;
            float propTop    = prop->position.y + prop->height * 0.5f;
            float pathMinY   = fminf(pPrev.y, pCurr.y) - shipHalfH;
            float pathMaxY   = fmaxf(pPrev.y, pCurr.y) + shipHalfH;

            if (pathMinY > propTop || pathMaxY < propBottom) {
                continue;
            }

            Vector3 propC = prop->position;

            // ----------------------------------------------------------------
            // CASO A: MONOLITOS URBANOS / EDIFICIOS (PROP_CATEGORY_BUILDING)
            // Hitbox plana orientada a la fachada visual con espesor delgado
            // y detección continua por barrido (Swept Slab Intersection)
            // ----------------------------------------------------------------
            if (prop->category == PROP_CATEGORY_BUILDING) {
                // Vector de aproximación hacia el centro del edificio
                Vector2 toProp = { propC.x - pPrev.x, propC.z - pPrev.z };
                float distToProp = sqrtf(toProp.x * toProp.x + toProp.y * toProp.y);

                Vector2 N; // Normal de aproximación / profundidad
                if (distToProp > 0.001f) {
                    N = (Vector2){ toProp.x / distToProp, toProp.y / distToProp };
                } else {
                    N = (Vector2){ player->forward.x, player->forward.z };
                    float lenN = sqrtf(N.x * N.x + N.y * N.y);
                    if (lenN > 0.001f) { N.x /= lenN; N.y /= lenN; }
                    else N = (Vector2){ 0.0f, 1.0f };
                }

                // Tangente lateral a lo largo de la fachada del edificio
                Vector2 T = { -N.y, N.x };

                // Dimensiones del volumen sólido del edificio:
                // Ancho sólido = semi-ancho frontal (76% del sprite visual) + radio de nave
                float solidHalfW = prop->collisionRadius + shipRadius;
                // Grosor delgado de la fachada = 6.0m + radio nave (elimina chocar 30m en el aire)
                float solidHalfD = 6.0f + shipRadius;

                // Proyección de posiciones previa y actual en el marco local (u=lateral, v=profundidad)
                float relPrevX = pPrev.x - propC.x;
                float relPrevZ = pPrev.z - propC.z;
                float uPrev = relPrevX * T.x + relPrevZ * T.y;
                float vPrev = relPrevX * N.x + relPrevZ * N.y; // < 0 cuando está frente al edificio

                float relCurrX = pCurr.x - propC.x;
                float relCurrZ = pCurr.z - propC.z;
                float uCurr = relCurrX * T.x + relCurrZ * T.y;
                float vCurr = relCurrX * N.x + relCurrZ * N.y;

                bool impactConfirmed = false;
                float tImpact = 1.0f;

                // 1. ¿Está actualmente dentro del volumen sólido de la fachada?
                if (fabsf(uCurr) <= solidHalfW && fabsf(vCurr) <= solidHalfD) {
                    impactConfirmed = true;
                    tImpact = 1.0f;
                }
                // 2. ¿Cruzó la cara frontal del volumen durante este frame? (Anti-Tunneling)
                else if (vPrev < -solidHalfD && vCurr >= -solidHalfD) {
                    float denom = vCurr - vPrev;
                    if (denom > 0.0001f) {
                        float tFront = (-solidHalfD - vPrev) / denom;
                        tFront = Clamp(tFront, 0.0f, 1.0f);
                        float uAtFront = uPrev + tFront * (uCurr - uPrev);
                        if (fabsf(uAtFront) <= solidHalfW) {
                            impactConfirmed = true;
                            tImpact = tFront;
                        }
                    }
                }
                // 3. ¿Cruzó el plano visual cero durante este frame? (Anti-Tunneling)
                else if (vPrev <= 0.0f && vCurr >= 0.0f) {
                    float denom = vCurr - vPrev;
                    if (denom > 0.0001f) {
                        float tPlane = -vPrev / denom;
                        tPlane = Clamp(tPlane, 0.0f, 1.0f);
                        float uAtPlane = uPrev + tPlane * (uCurr - uPrev);
                        if (fabsf(uAtPlane) <= solidHalfW) {
                            impactConfirmed = true;
                            tImpact = tPlane;
                        }
                    }
                }

                if (impactConfirmed) {
                    float yAtImpact = pPrev.y + tImpact * (pCurr.y - pPrev.y);
                    if (yAtImpact + shipHalfH >= propBottom && yAtImpact - shipHalfH <= propTop) {
                        // Impacto catastrófico contra monolito confirmado
                        player->hullIntegrity = 0.0f;
                        player->isDead = true;
                        player->deathTimer = 0.0f;
                        player->fatalReason = "URBAN MONOLITH COLLAPSE";
                        player->damageFlashTimer = 0.40f;

                        // Fijar la posición exacta del impacto en el morro del avión
                        player->position.x = pPrev.x + tImpact * (pCurr.x - pPrev.x);
                        player->position.y = yAtImpact;
                        player->position.z = pPrev.z + tImpact * (pCurr.z - pPrev.z);
                        player->forwardSpeed = 0.0f;
                        player->velocity = (Vector3){ 0 };
                        return true;
                    }
                }
            }
            // ----------------------------------------------------------------
            // CASO B: ÁRBOLES / PALMERAS (PROP_CATEGORY_TREE)
            // Tronco cilíndrico vertical con proyección swept-segment continua
            // ----------------------------------------------------------------
            else if (prop->category == PROP_CATEGORY_TREE) {
                float segDx = pCurr.x - pPrev.x;
                float segDz = pCurr.z - pPrev.z;
                float segLenSq = segDx * segDx + segDz * segDz;

                float t = 0.0f;
                if (segLenSq > 0.0001f) {
                    t = ((propC.x - pPrev.x) * segDx + (propC.z - pPrev.z) * segDz) / segLenSq;
                    t = Clamp(t, 0.0f, 1.0f);
                }

                float closeX = pPrev.x + t * segDx;
                float closeZ = pPrev.z + t * segDz;
                float closeY = pPrev.y + t * (pCurr.y - pPrev.y);

                float trunkRadius = prop->collisionRadius;
                float maxTreeDist = trunkRadius + shipRadius;
                float dX = closeX - propC.x;
                float dZ = closeZ - propC.z;

                if (dX * dX + dZ * dZ <= maxTreeDist * maxTreeDist) {
                    if (closeY + shipHalfH >= propBottom && closeY - shipHalfH <= propTop) {
                        if (player->damageFlashTimer <= 0.0f) {
                            player->hullIntegrity -= DAMAGE_TREE_STRIKE;
                            player->damageFlashTimer = COLLISION_INVULN_TIME;
                            player->forwardSpeed *= 0.72f;
                            player->alertTimer = 1.8f;
                            snprintf(player->lastAlertText, sizeof(player->lastAlertText), "ALERT: TREE STRIKE // HULL COMPROMISED -20%%");

                            if (player->hullIntegrity <= 0.0f) {
                                player->hullIntegrity = 0.0f;
                                player->isDead = true;
                                player->deathTimer = 0.0f;
                                player->fatalReason = "CRITICAL HULL INTEGRITY FAILURE";
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return player->isDead;
}

// ============================================================================
// LIBERACIÓN DE RECURSOS
// ============================================================================

void Scenery_Unload(ScenerySystem *scenery) {
    if (scenery->billboardShader.id != 0) UnloadShader(scenery->billboardShader);
    if (scenery->texTree1.id != 0) UnloadTexture(scenery->texTree1);
    if (scenery->texTree2.id != 0) UnloadTexture(scenery->texTree2);
    if (scenery->texTree3.id != 0) UnloadTexture(scenery->texTree3);
    if (scenery->texBush1.id != 0) UnloadTexture(scenery->texBush1);
    if (scenery->texBush2.id != 0) UnloadTexture(scenery->texBush2);
    if (scenery->texBuildSmall1.id != 0) UnloadTexture(scenery->texBuildSmall1);
    if (scenery->texBuildSmall2.id != 0) UnloadTexture(scenery->texBuildSmall2);
    if (scenery->texBuildTall1.id != 0)  UnloadTexture(scenery->texBuildTall1);
    if (scenery->texBuildTall2.id != 0)  UnloadTexture(scenery->texBuildTall2);
}
