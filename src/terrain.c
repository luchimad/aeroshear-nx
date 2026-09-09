#include "terrain.h"
#include "config.h"
#include "biome.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>

static RenderDistance s_currentRenderDist = RENDER_DIST_LOW;

// ============================================================================
// GENERADOR DE RUIDO PERLIN PROCEDURAL Y MATEMÁTICAS DE RELIEVE
// ============================================================================

static const int P[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

static inline float PerlinFade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static inline float Grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

static float Perlin2D(float x, float y) {
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);

    float u = PerlinFade(xf);
    float v = PerlinFade(yf);

    int aa = P[P[xi] + yi];
    int ab = P[P[xi] + yi + 1];
    int ba = P[P[xi + 1] + yi];
    int bb = P[P[xi + 1] + yi + 1];

    float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1.0f, yf), u);
    float x2 = Lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);
    return Lerp(x1, x2, v) * 0.5f;
}

// ============================================================================
// GENERACIÓN MATEMÁTICA DE ELEVACIÓN POR BIOMA (DELTA & HOTSANDS)
// ============================================================================

// 2. HOT SANDS: Desierto interminable de dunas eólicas
static float GetHeight_Hotsands(float worldX, float worldZ) {
    float duneWave = sinf(worldX * 0.0022f + sinf(worldZ * 0.0015f) * 1.8f);
    float duneAsym = (duneWave > 0.0f) ? powf(duneWave, 1.35f) : -powf(-duneWave, 0.75f);
    float duneMajor = duneAsym * 42.0f;

    float nBarchan = Perlin2D(worldX * 0.0040f + 20.0f, worldZ * 0.0040f + 15.0f);
    float barchan = 1.0f - fabsf(nBarchan * 1.8f);
    if (barchan < 0.0f) barchan = 0.0f;
    barchan = barchan * barchan * 26.0f;

    float ripples = sinf(worldX * 0.010f + worldZ * 0.007f) * 4.5f + Perlin2D(worldX * 0.008f, worldZ * 0.008f) * 5.0f;
    float baseDesert = Perlin2D(worldX * 0.00035f, worldZ * 0.00035f) * 22.0f;

    return baseDesert + duneMajor + barchan + ripples + 60.0f;
}

// 3. DELTA STRAITS: Delta fluvial y archipiélagos (Agua plana a 45m)
static float GetHeight_Deltastraits(float worldX, float worldZ) {
    // Brazos de río y canales
    float river1 = fabsf(Perlin2D(worldX * 0.0014f + 5.0f, worldZ * 0.0014f + 12.0f));
    float river2 = fabsf(Perlin2D(worldX * 0.0028f + 30.0f, worldZ * 0.0028f + 40.0f));
    float trench = fminf(river1, river2);

    // Factor de isla: 0 = fondo de río submarino (32m), 1 = tierra firme emergida (>45m)
    float islandMask = (trench - 0.035f) / (0.15f - 0.035f);
    if (islandMask < 0.0f) islandMask = 0.0f;
    if (islandMask > 1.0f) islandMask = 1.0f;
    islandMask = islandMask * islandMask * (3.0f - 2.0f * islandMask);

    // Relieve de las islas
    float islandRelief = Perlin2D(worldX * 0.0025f + 10.0f, worldZ * 0.0025f + 25.0f) * 24.0f;
    if (islandRelief < 0.0f) islandRelief = 0.0f;

    float riverBed = 30.0f + Perlin2D(worldX * 0.0006f, worldZ * 0.0006f) * 6.0f;
    float islandLand = 45.0f + 2.5f + islandRelief; // Costas a 45-47.5m, colinas a 48-72m

    return Lerp(riverBed, islandLand, islandMask);
}

// Función principal de altura consultando el bioma activo
float Terrain_GetHeight(float worldX, float worldZ) {
    BiomeType activeBiome = Biome_GetActive();
    if (activeBiome == BIOME_HOTSANDS) {
        return GetHeight_Hotsands(worldX, worldZ);
    }
    return GetHeight_Deltastraits(worldX, worldZ);
}

// Normal aproximada mediante diferencias finitas
Vector3 Terrain_GetNormal(float worldX, float worldZ) {
    float eps = 2.5f;
    float hL = Terrain_GetHeight(worldX - eps, worldZ);
    float hR = Terrain_GetHeight(worldX + eps, worldZ);
    float hD = Terrain_GetHeight(worldX, worldZ - eps);
    float hU = Terrain_GetHeight(worldX, worldZ + eps);

    Vector3 n = { hL - hR, 2.0f * eps, hD - hU };
    return Vector3Normalize(n);
}

// ============================================================================
// CONSTRUCCIÓN Y ACTUALIZACIÓN DE MALLAS DE CHUNKS
// ============================================================================

static void BuildChunkMeshData(Mesh *mesh, int gridRes, float chunkSize, float worldOriginX, float worldOriginZ) {
    float step = chunkSize / (float)gridRes;
    int vIdx = 0;

    for (int z = 0; z <= gridRes; z++) {
        for (int x = 0; x <= gridRes; x++) {
            float localX = (float)x * step - chunkSize * 0.5f;
            float localZ = (float)z * step - chunkSize * 0.5f;
            float globalX = worldOriginX + localX;
            float globalZ = worldOriginZ + localZ;
            float globalY = Terrain_GetHeight(globalX, globalZ);

            mesh->vertices[vIdx * 3 + 0] = globalX;
            mesh->vertices[vIdx * 3 + 1] = globalY;
            mesh->vertices[vIdx * 3 + 2] = globalZ;

            mesh->texcoords[vIdx * 2 + 0] = (float)x / (float)gridRes;
            mesh->texcoords[vIdx * 2 + 1] = (float)z / (float)gridRes;

            Vector3 n = Terrain_GetNormal(globalX, globalZ);
            mesh->normals[vIdx * 3 + 0] = n.x;
            mesh->normals[vIdx * 3 + 1] = n.y;
            mesh->normals[vIdx * 3 + 2] = n.z;

            vIdx++;
        }
    }
}

static Mesh CreateInitialChunkMesh(int gridRes, float chunkSize, float worldOriginX, float worldOriginZ) {
    Mesh mesh = { 0 };
    mesh.vertexCount = (gridRes + 1) * (gridRes + 1);
    mesh.triangleCount = gridRes * gridRes * 2;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short *)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    BuildChunkMeshData(&mesh, gridRes, chunkSize, worldOriginX, worldOriginZ);

    int iIdx = 0;
    for (int z = 0; z < gridRes; z++) {
        for (int x = 0; x < gridRes; x++) {
            int row1 = z * (gridRes + 1);
            int row2 = (z + 1) * (gridRes + 1);

            mesh.indices[iIdx++] = (unsigned short)(row1 + x);
            mesh.indices[iIdx++] = (unsigned short)(row1 + x + 1);
            mesh.indices[iIdx++] = (unsigned short)(row2 + x);

            mesh.indices[iIdx++] = (unsigned short)(row1 + x + 1);
            mesh.indices[iIdx++] = (unsigned short)(row2 + x + 1);
            mesh.indices[iIdx++] = (unsigned short)(row2 + x);
        }
    }

    UploadMesh(&mesh, true);
    return mesh;
}

static void UpdateChunkPositionAndMesh(TerrainChunk *chunk, int gridRes, float chunkSize, int newChunkX, int newChunkZ, Shader terrainShader, Texture2D baseTex) {
    chunk->chunkX = newChunkX;
    chunk->chunkZ = newChunkZ;
    chunk->worldPosition = (Vector3){ (float)newChunkX * chunkSize, 0.0f, (float)newChunkZ * chunkSize };

    BuildChunkMeshData(&chunk->mesh, gridRes, chunkSize, chunk->worldPosition.x, chunk->worldPosition.z);

    UpdateMeshBuffer(chunk->mesh, 0, chunk->mesh.vertices, chunk->mesh.vertexCount * 3 * sizeof(float), 0);
    UpdateMeshBuffer(chunk->mesh, 2, chunk->mesh.normals, chunk->mesh.vertexCount * 3 * sizeof(float), 0);

    chunk->model.transform = MatrixIdentity();
    chunk->model.materials[0].shader = terrainShader;
    chunk->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = baseTex;
    chunk->isLoaded = true;
}

// ============================================================================
// CARGA Y CAMBIO DE BIOMAS
// ============================================================================

static void UnloadCurrentTextures(TerrainSystem *terrain) {
    Texture2D *textures[6] = {
        &terrain->tex1, &terrain->tex2, &terrain->tex3,
        &terrain->tex4, &terrain->tex5, &terrain->texWater
    };
    for (int i = 0; i < 6; i++) {
        if (textures[i]->id != 0) {
            unsigned int id = textures[i]->id;
            UnloadTexture(*textures[i]);
            for (int j = i; j < 6; j++) {
                if (textures[j]->id == id) {
                    *textures[j] = (Texture2D){ 0 };
                }
            }
        }
    }
}

static void SetupTextureParams(Texture2D tex) {
    if (tex.id == 0) return;
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
}

void Terrain_LoadBiome(TerrainSystem *terrain, BiomeType biome) {
    terrain->currentBiome = biome;
    Biome_SetActive(biome);
    const BiomeDefinition *bDef = Biome_Get(biome);

    UnloadCurrentTextures(terrain);

    if (biome == BIOME_HOTSANDS) {
        terrain->tex1 = LoadTexture(PATH_TEX_HS_SAND_1);
        terrain->tex2 = LoadTexture(PATH_TEX_HS_SAND_2);
        terrain->tex3 = terrain->tex1;
        terrain->tex4 = terrain->tex2;
        terrain->tex5 = terrain->tex1;
        terrain->texWater = (Texture2D){ 0 };
    } else {
        terrain->tex1 = LoadTexture(PATH_TEX_DS_GRASS_1);
        terrain->tex2 = LoadTexture(PATH_TEX_DS_GRASS_2);
        terrain->tex3 = LoadTexture(PATH_TEX_DS_GRASS_3);
        terrain->tex4 = LoadTexture(PATH_TEX_DS_SAND_2);
        terrain->tex5 = LoadTexture(PATH_TEX_DS_WATER);
        terrain->texWater = terrain->tex5;
    }

    SetupTextureParams(terrain->tex1);
    SetupTextureParams(terrain->tex2);
    SetupTextureParams(terrain->tex3);
    SetupTextureParams(terrain->tex4);
    SetupTextureParams(terrain->tex5);
    SetupTextureParams(terrain->texWater);

    // Asignar textura al modelo del plano de agua
    terrain->waterPlaneModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = terrain->texWater;

    // Actualizar uniformes del shader de terreno
    int bType = (int)biome;
    float wLevel = bDef->waterLevel;
    float fStart = (s_currentRenderDist == RENDER_DIST_HIGH) ? TERRAIN_FOG_START_HIGH : bDef->fogStart;
    float fEnd = (s_currentRenderDist == RENDER_DIST_HIGH) ? TERRAIN_FOG_END_HIGH : bDef->fogEnd;

    SetShaderValue(terrain->terrainShader, terrain->locBiomeType, &bType, SHADER_UNIFORM_INT);
    SetShaderValue(terrain->terrainShader, terrain->locWaterLevel, &wLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrain->terrainShader, terrain->locFogStart, &fStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrain->terrainShader, terrain->locFogEnd, &fEnd, SHADER_UNIFORM_FLOAT);

    // Actualizar uniformes del shader de agua
    SetShaderValue(terrain->waterShader, terrain->locWaterFogStart, &fStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrain->waterShader, terrain->locWaterFogEnd, &fEnd, SHADER_UNIFORM_FLOAT);

    // Regenerar VBOs de chunks con la nueva topografía
    for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
        TerrainChunk *chunk = &terrain->chunks[i];
        if (chunk->isLoaded) {
            UpdateChunkPositionAndMesh(chunk, TERRAIN_GRID_RES, TERRAIN_CHUNK_SIZE, chunk->chunkX, chunk->chunkZ, terrain->terrainShader, terrain->tex1);
        }
    }
}

void Terrain_SetRenderDistance(TerrainSystem *terrain, RenderDistance dist) {
    s_currentRenderDist = dist;
    float fStart = (dist == RENDER_DIST_HIGH) ? TERRAIN_FOG_START_HIGH : TERRAIN_FOG_START_LOW;
    float fEnd   = (dist == RENDER_DIST_HIGH) ? TERRAIN_FOG_END_HIGH   : TERRAIN_FOG_END_LOW;

    SetShaderValue(terrain->terrainShader, terrain->locFogStart, &fStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrain->terrainShader, terrain->locFogEnd, &fEnd, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrain->waterShader, terrain->locWaterFogStart, &fStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrain->waterShader, terrain->locWaterFogEnd, &fEnd, SHADER_UNIFORM_FLOAT);
}

// ============================================================================
// INICIALIZACIÓN DEL SISTEMA DE TERRENO Y PLANO DE AGUA
// ============================================================================

void Terrain_Init(TerrainSystem *terrain) {
    terrain->currentBiome = BIOME_DELTASTRAITS;
    Biome_SetActive(BIOME_DELTASTRAITS);

    // 1. Compilación del Shader de Terreno
    terrain->terrainShader = LoadShader("shaders/terrain.vs", "shaders/terrain.fs");

    int locTex0 = GetShaderLocation(terrain->terrainShader, "texture0");
    int locTex1 = GetShaderLocation(terrain->terrainShader, "texture1");
    int locTex2 = GetShaderLocation(terrain->terrainShader, "texture2");
    int locTex3 = GetShaderLocation(terrain->terrainShader, "texture3");
    int locTex4 = GetShaderLocation(terrain->terrainShader, "texture4");

    int s0 = 0, s1 = 1, s2 = 2, s3 = 3, s4 = 4;
    SetShaderValue(terrain->terrainShader, locTex0, &s0, SHADER_UNIFORM_INT);
    SetShaderValue(terrain->terrainShader, locTex1, &s1, SHADER_UNIFORM_INT);
    SetShaderValue(terrain->terrainShader, locTex2, &s2, SHADER_UNIFORM_INT);
    SetShaderValue(terrain->terrainShader, locTex3, &s3, SHADER_UNIFORM_INT);
    SetShaderValue(terrain->terrainShader, locTex4, &s4, SHADER_UNIFORM_INT);

    terrain->locCameraPos    = GetShaderLocation(terrain->terrainShader, "uCameraPos");
    terrain->locSunDirection = GetShaderLocation(terrain->terrainShader, "uSunDirection");
    terrain->locFogColor     = GetShaderLocation(terrain->terrainShader, "uFogColor");
    terrain->locFogStart     = GetShaderLocation(terrain->terrainShader, "uFogStart");
    terrain->locFogEnd       = GetShaderLocation(terrain->terrainShader, "uFogEnd");
    terrain->locBiomeType    = GetShaderLocation(terrain->terrainShader, "uBiomeType");
    terrain->locWaterLevel   = GetShaderLocation(terrain->terrainShader, "uWaterLevel");
    terrain->locTime         = GetShaderLocation(terrain->terrainShader, "uTime");

    // 2. Compilación del Shader y Malla del Plano Horizontal de Agua
    terrain->waterShader = LoadShader("shaders/water.vs", "shaders/water.fs");
    terrain->locWaterTime     = GetShaderLocation(terrain->waterShader, "uTime");
    terrain->locWaterCamPos   = GetShaderLocation(terrain->waterShader, "uCameraPos");
    terrain->locWaterSunDir   = GetShaderLocation(terrain->waterShader, "uSunDirection");
    terrain->locWaterFogColor = GetShaderLocation(terrain->waterShader, "uFogColor");
    terrain->locWaterFogStart = GetShaderLocation(terrain->waterShader, "uFogStart");
    terrain->locWaterFogEnd   = GetShaderLocation(terrain->waterShader, "uFogEnd");

    // Crear plano horizontal amplio de agua
    float waterPlaneSpan = TERRAIN_CHUNK_SIZE * (float)TERRAIN_CHUNK_GRID * 1.8f;
    Mesh waterMesh = GenMeshPlane(waterPlaneSpan, waterPlaneSpan, 1, 1);
    terrain->waterPlaneModel = LoadModelFromMesh(waterMesh);
    terrain->waterPlaneModel.materials[0].shader = terrain->waterShader;

    terrain->centerChunkX = 0;
    terrain->centerChunkZ = 0;

    // 3. Creación inicial de las 81 mallas de chunks (9x9)
    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int chunkIdx = 0;

    for (int gz = -halfGrid; gz <= halfGrid; gz++) {
        for (int gx = -halfGrid; gx <= halfGrid; gx++) {
            TerrainChunk *chunk = &terrain->chunks[chunkIdx++];
            chunk->chunkX = gx;
            chunk->chunkZ = gz;
            chunk->worldPosition = (Vector3){ (float)gx * TERRAIN_CHUNK_SIZE, 0.0f, (float)gz * TERRAIN_CHUNK_SIZE };

            chunk->mesh = CreateInitialChunkMesh(TERRAIN_GRID_RES, TERRAIN_CHUNK_SIZE, chunk->worldPosition.x, chunk->worldPosition.z);
            chunk->model = LoadModelFromMesh(chunk->mesh);
            chunk->model.transform = MatrixIdentity();
            chunk->model.materials[0].shader = terrain->terrainShader;
            chunk->isLoaded = true;
        }
    }

    // 4. Cargar texturas y parámetros del bioma inicial
    Terrain_LoadBiome(terrain, BIOME_DELTASTRAITS);
}

// ============================================================================
// STREAMING DINÁMICO DE CHUNKS
// ============================================================================

void Terrain_Update(TerrainSystem *terrain, Vector3 playerPos) {
    int currentCenterX = (int)roundf(playerPos.x / TERRAIN_CHUNK_SIZE);
    int currentCenterZ = (int)roundf(playerPos.z / TERRAIN_CHUNK_SIZE);

    terrain->centerChunkX = currentCenterX;
    terrain->centerChunkZ = currentCenterZ;

    int halfGrid = TERRAIN_CHUNK_GRID / 2;
    int minX = currentCenterX - halfGrid;
    int maxX = currentCenterX + halfGrid;
    int minZ = currentCenterZ - halfGrid;
    int maxZ = currentCenterZ + halfGrid;

    bool needed[TERRAIN_CHUNK_GRID][TERRAIN_CHUNK_GRID];
    bool hasMissingChunks = false;
    for (int r = 0; r < TERRAIN_CHUNK_GRID; r++) {
        for (int c = 0; c < TERRAIN_CHUNK_GRID; c++) {
            needed[r][c] = true;
        }
    }

    for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
        TerrainChunk *chunk = &terrain->chunks[i];
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
                hasMissingChunks = true;
                break;
            }
        }
        if (hasMissingChunks) break;
    }

    if (!hasMissingChunks) return;

    // Streaming por presupuesto de tiempo: máximo 3 mallas por fotograma para garantizar 60 FPS estables
    const int MAX_CHUNK_UPDATES_PER_FRAME = 3;
    int updatesThisFrame = 0;

    for (int r = 0; r < TERRAIN_CHUNK_GRID && updatesThisFrame < MAX_CHUNK_UPDATES_PER_FRAME; r++) {
        for (int c = 0; c < TERRAIN_CHUNK_GRID && updatesThisFrame < MAX_CHUNK_UPDATES_PER_FRAME; c++) {
            if (needed[r][c]) {
                int targetChunkX = minX + c;
                int targetChunkZ = minZ + r;

                for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
                    TerrainChunk *chunk = &terrain->chunks[i];
                    if (chunk->chunkX < minX || chunk->chunkX > maxX ||
                        chunk->chunkZ < minZ || chunk->chunkZ > maxZ) {
                        UpdateChunkPositionAndMesh(chunk, TERRAIN_GRID_RES, TERRAIN_CHUNK_SIZE, targetChunkX, targetChunkZ, terrain->terrainShader, terrain->tex1);
                        needed[r][c] = false;
                        updatesThisFrame++;
                        break;
                    }
                }
            }
        }
    }
}

// ============================================================================
// RENDERIZADO DEL TERRENO Y PLANO DE AGUA
// ============================================================================

void Terrain_Draw(TerrainSystem *terrain, const Camera3D *camera) {
    const BiomeDefinition *bDef = Biome_Get(terrain->currentBiome);

    Vector3 camPos = camera->position;
    Vector3 sunDir = bDef->sunDirection;
    Color fogCol = bDef->fogHorizonColor;
    Vector4 fogColorVec = { (float)fogCol.r / 255.0f, (float)fogCol.g / 255.0f, (float)fogCol.b / 255.0f, 1.0f };
    float time = (float)GetTime();

    // 1. Actualizar uniformes del shader de terreno
    SetShaderValue(terrain->terrainShader, terrain->locCameraPos, &camPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrain->terrainShader, terrain->locSunDirection, &sunDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrain->terrainShader, terrain->locFogColor, &fogColorVec, SHADER_UNIFORM_VEC4);
    SetShaderValue(terrain->terrainShader, terrain->locTime, &time, SHADER_UNIFORM_FLOAT);

    // Enlazar los 5 slots de textura en OpenGL para el terreno
    rlActiveTextureSlot(0);
    rlEnableTexture(terrain->tex1.id);
    rlActiveTextureSlot(1);
    rlEnableTexture(terrain->tex2.id);
    rlActiveTextureSlot(2);
    rlEnableTexture(terrain->tex3.id);
    rlActiveTextureSlot(3);
    rlEnableTexture(terrain->tex4.id);
    rlActiveTextureSlot(4);
    rlEnableTexture(terrain->tex5.id);

    rlDisableBackfaceCulling();

    // Dibujar los 81 chunks de terreno
    for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
        TerrainChunk *chunk = &terrain->chunks[i];
        if (chunk->isLoaded) {
            DrawModel(chunk->model, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        }
    }

    rlEnableBackfaceCulling();

    rlActiveTextureSlot(4);
    rlDisableTexture();
    rlActiveTextureSlot(3);
    rlDisableTexture();
    rlActiveTextureSlot(2);
    rlDisableTexture();
    rlActiveTextureSlot(1);
    rlDisableTexture();
    rlActiveTextureSlot(0);

    // 2. Renderizado del Plano Horizontal de Agua si el bioma tiene agua
    if (bDef->hasWater && terrain->texWater.id != 0) {
        SetShaderValue(terrain->waterShader, terrain->locWaterCamPos, &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(terrain->waterShader, terrain->locWaterSunDir, &sunDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(terrain->waterShader, terrain->locWaterFogColor, &fogColorVec, SHADER_UNIFORM_VEC4);
        SetShaderValue(terrain->waterShader, terrain->locWaterTime, &time, SHADER_UNIFORM_FLOAT);

        terrain->waterPlaneModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = terrain->texWater;

        // El plano de agua sigue al jugador en X y Z a una altura Y fija (nivel del mar)
        Vector3 waterPos = { camera->position.x, bDef->waterLevel, camera->position.z };
        DrawModel(terrain->waterPlaneModel, waterPos, 1.0f, WHITE);
    }
}

// ============================================================================
// LIBERACIÓN DE RECURSOS
// ============================================================================

void Terrain_Unload(TerrainSystem *terrain) {
    for (int i = 0; i < TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID; i++) {
        TerrainChunk *chunk = &terrain->chunks[i];
        if (chunk->isLoaded) {
            UnloadModel(chunk->model);
            chunk->isLoaded = false;
        }
    }

    UnloadModel(terrain->waterPlaneModel);
    UnloadShader(terrain->waterShader);

    UnloadCurrentTextures(terrain);
    UnloadShader(terrain->terrainShader);
}
