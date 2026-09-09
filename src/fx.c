#include "fx.h"
#include "terrain.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>

#define MAX_AIR_MOTES       64
#define MAX_WAKE_PARTICLES  48
#define MAX_SPEED_STREAKS   48

typedef struct AirMote {
    Vector3 relPos;
    Vector3 worldPos;
    float length;
    float alpha;
    float speed;
} AirMote;

typedef struct Particle {
    Vector3 pos;
    Vector3 vel;
    float size;
    float alpha;
    float life;
    float maxLife;
    Color color;
    bool active;
} Particle;

typedef struct SpeedStreak {
    Vector2 start;
    Vector2 end;
    float speed;
    float length;
    float alpha;
    float thickness;
} SpeedStreak;

#define MAX_TRAIL_POINTS 22
typedef struct TrailPoint {
    Vector3 leftPos;
    Vector3 rightPos;
    float alpha;
    bool active;
} TrailPoint;

#define MAX_WALL_SPARKS 64
typedef struct SparkParticle {
    Vector3 pos;
    Vector3 vel;
    float life;
    float maxLife;
    float size;
    Color color;
    bool active;
} SparkParticle;

static AirMote airMotes[MAX_AIR_MOTES];
static Particle wakeParticles[MAX_WAKE_PARTICLES];
static SpeedStreak speedStreaks[MAX_SPEED_STREAKS];
static TrailPoint thrusterTrails[MAX_TRAIL_POINTS];
static SparkParticle wallSparks[MAX_WALL_SPARKS];
static float trailTimer = 0.0f;

void FX_Init(void) {
    // 1. Inicializar motas de aire
    for (int i = 0; i < MAX_AIR_MOTES; i++) {
        airMotes[i].relPos = (Vector3){
            ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 75.0f,
            ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 35.0f,
            ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 110.0f
        };
        airMotes[i].speed = 1.0f + ((float)rand() / (float)RAND_MAX) * 0.4f;
        airMotes[i].alpha = 0.20f + ((float)rand() / (float)RAND_MAX) * 0.45f;
        airMotes[i].length = 2.0f;
    }

    // 2. Partículas de estela de suelo
    for (int i = 0; i < MAX_WAKE_PARTICLES; i++) {
        wakeParticles[i].active = false;
    }

    // 2b. Estelas de toberas de plasma
    for (int i = 0; i < MAX_TRAIL_POINTS; i++) {
        thrusterTrails[i].active = false;
    }
    for (int i = 0; i < MAX_WALL_SPARKS; i++) {
        wallSparks[i].active = false;
    }
    trailTimer = 0.0f;

    // 3. Líneas radiales de velocidad 2D (Warp Streaks)
    for (int i = 0; i < MAX_SPEED_STREAKS; i++) {
        float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * PI;
        float dist = 60.0f + ((float)rand() / (float)RAND_MAX) * 320.0f;
        speedStreaks[i].start = (Vector2){ cosf(angle) * dist, sinf(angle) * dist };
        speedStreaks[i].end = speedStreaks[i].start;
        speedStreaks[i].speed = 450.0f + ((float)rand() / (float)RAND_MAX) * 650.0f;
        speedStreaks[i].length = 25.0f + ((float)rand() / (float)RAND_MAX) * 60.0f;
        speedStreaks[i].alpha = 0.0f;
        speedStreaks[i].thickness = 1.5f + ((float)rand() / (float)RAND_MAX) * 1.5f;
    }
}

void FX_Update(const PlayerJet *player, float dt, bool isWaterBiome, bool isDesertBiome, float waterLevel) {
    Vector3 rightVec = Vector3Normalize(Vector3CrossProduct(player->forward, (Vector3){ 0, 1, 0 }));
    if (Vector3Length(rightVec) < 0.1f) rightVec = (Vector3){ 1, 0, 0 };
    Vector3 upVec = Vector3Normalize(Vector3CrossProduct(rightVec, player->forward));

    float speedRatio = player->forwardSpeed / SPEED_CRUISE;

    // ========================================================================
    // 1. MOTAS Y TRAZAS DE AIRE 3D (AIR MOTES)
    // ========================================================================
    float streamSpeed = player->forwardSpeed * dt;

    for (int i = 0; i < MAX_AIR_MOTES; i++) {
        airMotes[i].relPos.z -= streamSpeed * airMotes[i].speed;

        if (airMotes[i].relPos.z < -45.0f) {
            airMotes[i].relPos.z += 155.0f;
            airMotes[i].relPos.x = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 75.0f;
            airMotes[i].relPos.y = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 35.0f;
            airMotes[i].alpha = 0.20f + ((float)rand() / (float)RAND_MAX) * 0.45f;
        }

        airMotes[i].length = (player->isAfterburner ? 6.5f : 3.0f) * speedRatio;

        Vector3 wPos = player->position;
        wPos = Vector3Add(wPos, Vector3Scale(rightVec, airMotes[i].relPos.x));
        wPos = Vector3Add(wPos, Vector3Scale(upVec, airMotes[i].relPos.y));
        wPos = Vector3Add(wPos, Vector3Scale(player->forward, airMotes[i].relPos.z));
        airMotes[i].worldPos = wPos;
    }

    // ========================================================================
    // 2. ESTELAS DE SUELO (AGUA / POLVO)
    // ========================================================================
    if (player->forwardSpeed > SPEED_CRUISE * 0.60f && player->altitudeAGL < 20.0f) {
        if (isWaterBiome && player->position.y <= waterLevel + 18.0f) {
            for (int i = 0; i < MAX_WAKE_PARTICLES; i++) {
                if (!wakeParticles[i].active) {
                    wakeParticles[i].active = true;
                    float sideOffset = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 6.0f;
                    wakeParticles[i].pos = (Vector3){
                        player->position.x - player->forward.x * 12.0f + rightVec.x * sideOffset,
                        waterLevel + 0.2f,
                        player->position.z - player->forward.z * 12.0f + rightVec.z * sideOffset
                    };
                    wakeParticles[i].vel = (Vector3){
                        rightVec.x * sideOffset * 2.5f,
                        1.5f + ((float)rand() / (float)RAND_MAX) * 2.5f,
                        rightVec.z * sideOffset * 2.5f
                    };
                    wakeParticles[i].size = 3.5f + ((float)rand() / (float)RAND_MAX) * 4.0f;
                    wakeParticles[i].alpha = 0.75f;
                    wakeParticles[i].life = 0.0f;
                    wakeParticles[i].maxLife = 0.45f;
                    wakeParticles[i].color = (Color){ 215, 245, 255, 180 };
                    break;
                }
            }
        } else if (isDesertBiome) {
            for (int i = 0; i < MAX_WAKE_PARTICLES; i++) {
                if (!wakeParticles[i].active) {
                    wakeParticles[i].active = true;
                    float sideOffset = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 6.0f;
                    wakeParticles[i].pos = (Vector3){
                        player->position.x - player->forward.x * 12.0f + rightVec.x * sideOffset,
                        player->groundHeight + 0.4f,
                        player->position.z - player->forward.z * 12.0f + rightVec.z * sideOffset
                    };
                    wakeParticles[i].vel = (Vector3){
                        -player->forward.x * 5.0f,
                        1.2f + ((float)rand() / (float)RAND_MAX) * 2.0f,
                        -player->forward.z * 5.0f
                    };
                    wakeParticles[i].size = 4.0f + ((float)rand() / (float)RAND_MAX) * 4.5f;
                    wakeParticles[i].alpha = 0.60f;
                    wakeParticles[i].life = 0.0f;
                    wakeParticles[i].maxLife = 0.50f;
                    wakeParticles[i].color = (Color){ 235, 190, 120, 150 };
                    break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_WAKE_PARTICLES; i++) {
        if (wakeParticles[i].active) {
            wakeParticles[i].life += dt;
            if (wakeParticles[i].life >= wakeParticles[i].maxLife) {
                wakeParticles[i].active = false;
            } else {
                wakeParticles[i].pos = Vector3Add(wakeParticles[i].pos, Vector3Scale(wakeParticles[i].vel, dt));
                float prog = wakeParticles[i].life / wakeParticles[i].maxLife;
                wakeParticles[i].alpha = (1.0f - prog) * 0.75f;
                wakeParticles[i].size += dt * 6.0f;
            }
        }
    }

    // 2c. Actualizar chispas de fricción metálica
    for (int i = 0; i < MAX_WALL_SPARKS; i++) {
        if (wallSparks[i].active) {
            wallSparks[i].life += dt;
            if (wallSparks[i].life >= wallSparks[i].maxLife) {
                wallSparks[i].active = false;
            } else {
                wallSparks[i].pos = Vector3Add(wallSparks[i].pos, Vector3Scale(wallSparks[i].vel, dt));
                wallSparks[i].vel.y -= 22.0f * dt; // Gravedad
                wallSparks[i].vel = Vector3Scale(wallSparks[i].vel, 1.0f - dt * 2.8f);
            }
        }
    }

    // ========================================================================
    // 3. LÍNEAS RADIALES DE VELOCIDAD 2D (WARP STREAKS)
    // ========================================================================
    float targetStreakAlpha = player->isAfterburner ? 0.80f : (speedRatio > 0.98f ? 0.25f : 0.0f);

    for (int i = 0; i < MAX_SPEED_STREAKS; i++) {
        speedStreaks[i].alpha = Lerp(speedStreaks[i].alpha, targetStreakAlpha, dt * 5.0f);

        float currentDist = Vector2Length(speedStreaks[i].start);
        currentDist += (speedStreaks[i].speed * (player->isAfterburner ? 1.35f : 1.0f)) * dt;

        if (currentDist > 650.0f) {
            float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * PI;
            float dist = 70.0f + ((float)rand() / (float)RAND_MAX) * 160.0f;
            speedStreaks[i].start = (Vector2){ cosf(angle) * dist, sinf(angle) * dist };
            currentDist = dist;
            speedStreaks[i].thickness = 1.2f + ((float)rand() / (float)RAND_MAX) * (player->isAfterburner ? 2.0f : 1.0f);
        }

        Vector2 norm = Vector2Normalize(speedStreaks[i].start);
        speedStreaks[i].start = Vector2Scale(norm, currentDist);
        float dynamicLen = speedStreaks[i].length * (player->isAfterburner ? 1.5f : 1.0f);
        speedStreaks[i].end = Vector2Scale(norm, currentDist + dynamicLen);
    }

    // 4. Actualizar puntos de estela de toberas de plasma
    trailTimer += dt;
    if (trailTimer >= 0.022f) {
        trailTimer = 0.0f;
        for (int i = MAX_TRAIL_POINTS - 1; i > 0; i--) {
            thrusterTrails[i] = thrusterTrails[i - 1];
            thrusterTrails[i].alpha *= 0.86f;
        }

        float craftScale = (player->baseScale > 0.1f) ? player->baseScale : SPRITE_BASE_SCALE;
        float nozzleSpacing = 2.4f * (craftScale / SPRITE_BASE_SCALE);

        Vector3 offsetL = Vector3Add(Vector3Scale(rightVec, -nozzleSpacing), Vector3Scale(player->forward, -2.8f));
        Vector3 offsetR = Vector3Add(Vector3Scale(rightVec,  nozzleSpacing), Vector3Scale(player->forward, -2.8f));

        thrusterTrails[0].leftPos = Vector3Add(player->position, offsetL);
        thrusterTrails[0].rightPos = Vector3Add(player->position, offsetR);
        thrusterTrails[0].alpha = player->isAfterburner ? 1.0f : (player->forwardSpeed > player->cruiseSpeed * 0.4f ? 0.70f : 0.0f);
        thrusterTrails[0].active = true;
    }
}

void FX_DrawThrusterTrails(void) {
    for (int i = 0; i < MAX_TRAIL_POINTS - 1; i++) {
        if (thrusterTrails[i].active && thrusterTrails[i + 1].active) {
            float a = thrusterTrails[i].alpha;
            if (a < 0.05f) continue;

            Color col = (Color){ 75, 195, 255, (unsigned char)(a * 210.0f) };
            DrawLine3D(thrusterTrails[i].leftPos, thrusterTrails[i + 1].leftPos, col);
            DrawLine3D(thrusterTrails[i].rightPos, thrusterTrails[i + 1].rightPos, col);
        }
    }
}

void FX_DrawPlayerShadow(const PlayerJet *player) {
    // Sombra poligonal deshabilitada a solicitud del usuario para mantener la pureza visual del terreno
    (void)player;
}

void FX_Draw3D(const Camera3D *camera) {
    // 1. Trazos de Viento en el Aire (Air Motes)
    for (int i = 0; i < MAX_AIR_MOTES; i++) {
        if (airMotes[i].alpha > 0.05f) {
            Color mCol = (Color){ 220, 240, 255, (unsigned char)(airMotes[i].alpha * 170) };
            Vector3 p1 = airMotes[i].worldPos;
            Vector3 camFwd = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
            Vector3 p2 = Vector3Subtract(p1, Vector3Scale(camFwd, airMotes[i].length));

            DrawLine3D(p1, p2, mCol);
        }
    }

    // 2. Estelas de Agua / Polvo (Discos y trazos suaves sin esferas de alambre)
    rlDisableBackfaceCulling();
    for (int i = 0; i < MAX_WAKE_PARTICLES; i++) {
        if (wakeParticles[i].active && wakeParticles[i].alpha > 0.05f) {
            Color col = wakeParticles[i].color;
            col.a = (unsigned char)(wakeParticles[i].alpha * 160.0f);
            float r = wakeParticles[i].size * 0.45f;
            Vector3 pos = wakeParticles[i].pos;

            DrawLine3D((Vector3){ pos.x - r, pos.y, pos.z - r }, (Vector3){ pos.x + r, pos.y, pos.z + r }, col);
            DrawLine3D((Vector3){ pos.x - r, pos.y, pos.z + r }, (Vector3){ pos.x + r, pos.y, pos.z - r }, col);
            DrawCircle3D(pos, r * 0.75f, (Vector3){ 0, 1, 0 }, 90.0f, col);
        }
    }
    rlEnableBackfaceCulling();

    // 3. Estelas de plasma de las toberas
    FX_DrawThrusterTrails();

    // 4. Chispas incandescentes de colisión contra paredes (Mach 1 Titanium Shear)
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    for (int i = 0; i < MAX_WALL_SPARKS; i++) {
        if (wallSparks[i].active) {
            float progress = wallSparks[i].life / wallSparks[i].maxLife;
            float alpha = (1.0f - progress);
            Color sparkCol = wallSparks[i].color;
            sparkCol.a = (unsigned char)(alpha * 245.0f);
            Vector3 trail = Vector3Subtract(wallSparks[i].pos, Vector3Scale(wallSparks[i].vel, 0.026f));

            // Trazo incandescente alargado
            DrawLine3D(wallSparks[i].pos, trail, sparkCol);
            // Trazo gemelo ligeramente desplazado para dar grosor a 1080p
            Vector3 offsetTrail = Vector3Add(trail, (Vector3){ 0.08f, 0.08f, 0.0f });
            DrawLine3D(wallSparks[i].pos, offsetTrail, sparkCol);

            // Núcleo caliente incandescente (hot core glow)
            float coreSize = wallSparks[i].size * alpha * 1.5f;
            DrawCube(wallSparks[i].pos, coreSize, coreSize, coreSize, sparkCol);

            // Punto blanco central de calor extremo (titanio fundido)
            Color whiteHot = (Color){ 255, 255, 240, (unsigned char)(alpha * 255.0f) };
            float whiteSize = coreSize * 0.45f;
            DrawCube(wallSparks[i].pos, whiteSize, whiteSize, whiteSize, whiteHot);
        }
    }
    rlEnableDepthMask();
    EndBlendMode();
}

void FX_SpawnWallSparks(Vector3 position, Vector3 normal, int count) {
    if (count <= 0) return;
    int spawned = 0;
    for (int i = 0; i < MAX_WALL_SPARKS && spawned < count; i++) {
        if (!wallSparks[i].active) {
            wallSparks[i].active = true;
            wallSparks[i].pos = position;
            wallSparks[i].life = 0.0f;
            wallSparks[i].maxLife = 0.18f + ((float)rand() / (float)RAND_MAX) * 0.28f;
            wallSparks[i].size = 0.35f;

            Vector3 randDir = {
                normal.x + ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.85f,
                normal.y + ((float)rand() / (float)RAND_MAX) * 1.1f + 0.25f,
                normal.z + ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.85f
            };
            float speed = 30.0f + ((float)rand() / (float)RAND_MAX) * 75.0f;
            wallSparks[i].vel = Vector3Scale(Vector3Normalize(randDir), speed);

            float heat = (float)rand() / (float)RAND_MAX;
            if (heat > 0.65f) {
                wallSparks[i].color = (Color){ 255, 250, 190, 255 };
            } else if (heat > 0.30f) {
                wallSparks[i].color = (Color){ 255, 170, 50, 255 };
            } else {
                wallSparks[i].color = (Color){ 255, 80, 20, 255 };
            }
            spawned++;
        }
    }
}

void FX_Draw2D(const PlayerJet *player, int screenWidth, int screenHeight) {
    Vector2 center = { (float)screenWidth * 0.5f, (float)screenHeight * 0.5f };

    for (int i = 0; i < MAX_SPEED_STREAKS; i++) {
        if (speedStreaks[i].alpha > 0.04f) {
            Color col = player->isAfterburner ? 
                (Color){ 210, 245, 255, (unsigned char)(speedStreaks[i].alpha * 160) } :
                (Color){ 140, 220, 255, (unsigned char)(speedStreaks[i].alpha * 90) };

            Vector2 p1 = Vector2Add(center, speedStreaks[i].start);
            Vector2 p2 = Vector2Add(center, speedStreaks[i].end);
            DrawLineEx(p1, p2, speedStreaks[i].thickness, col);
        }
    }

    // Destello de impacto con pared (vignette perimetral roja táctica)
    if (player->wallImpactTimer > 0.0f) {
        float alpha = player->wallImpactTimer / 0.45f;
        Color redVignette = (Color){ 255, 30, 40, (unsigned char)(alpha * 125.0f) };
        int borderW = (int)((float)screenWidth * 0.08f);
        int borderH = (int)((float)screenHeight * 0.10f);
        DrawRectangleGradientH(0, 0, borderW, screenHeight, redVignette, BLANK);
        DrawRectangleGradientH(screenWidth - borderW, 0, borderW, screenHeight, BLANK, redVignette);
        DrawRectangleGradientV(0, 0, screenWidth, borderH, redVignette, BLANK);
        DrawRectangleGradientV(0, screenHeight - borderH, screenWidth, borderH, BLANK, redVignette);
    }
}

void FX_DrawCRTPowerOn(float timer, int screenWidth, int screenHeight) {
    if (timer >= 1.15f) return;

    // Phase 1: Horizontal white-hot slit expanding (0.00s .. 0.15s)
    if (timer < 0.15f) {
        DrawRectangle(0, 0, screenWidth, screenHeight, BLACK);

        float t = timer / 0.15f;
        float slitW = t * (float)screenWidth;
        float slitX = ((float)screenWidth - slitW) * 0.5f;
        float slitY = (float)screenHeight * 0.5f;
        float slitH = 3.0f + (1.0f - t) * 6.0f;

        // Intense center flare with cyan corona
        DrawRectangle((int)slitX, (int)(slitY - slitH * 0.5f), (int)slitW, (int)slitH, WHITE);
        DrawRectangle((int)slitX, (int)(slitY - slitH * 1.5f), (int)slitW, (int)(slitH * 3.0f), (Color){ 120, 230, 255, 120 });
        DrawCircle((int)(screenWidth * 0.5f), (int)slitY, slitH * 4.0f, (Color){ 200, 250, 255, 180 });
    }
    // Phase 2: Vertical cathode bloom and raster expansion (0.15s .. 0.45s)
    else if (timer < 0.45f) {
        float t = (timer - 0.15f) / 0.30f;
        float openH = (float)screenHeight * powf(t, 2.2f);
        float topBarH = ((float)screenHeight - openH) * 0.5f;

        if (topBarH > 0.0f) {
            DrawRectangle(0, 0, screenWidth, (int)topBarH, BLACK);
            DrawRectangle(0, screenHeight - (int)topBarH, screenWidth, (int)topBarH, BLACK);
        }

        // Horizontal bright scan line at the moving frontiers
        DrawLine(0, (int)topBarH, screenWidth, (int)topBarH, (Color){ 160, 240, 255, 230 });
        DrawLine(0, screenHeight - (int)topBarH, screenWidth, screenHeight - (int)topBarH, (Color){ 160, 240, 255, 230 });

        // Phosphor flash inside the blooming opening
        float flashAlpha = (1.0f - t * 0.75f) * 160.0f;
        DrawRectangle(0, (int)topBarH, screenWidth, (int)openH, (Color){ 180, 240, 255, (unsigned char)flashAlpha });
    }
    // Phase 3: CRT phosphor settle and glow fade (0.45s .. 1.15f)
    else {
        float t = (timer - 0.45f) / 0.70f;
        float fade = (1.0f - t);
        if (fade > 0.01f) {
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 140, 215, 245, (unsigned char)(fade * 70.0f) });
        }
    }
}

void FX_DrawGForcePull(float gLoad, int screenWidth, int screenHeight) {
    if (gLoad < 1.8f) return;

    float stress = Clamp((gLoad - 1.8f) / 3.2f, 0.0f, 1.0f);
    if (stress <= 0.01f) return;

    int depthX = (int)((float)screenWidth * (0.05f + 0.14f * stress));
    int depthY = (int)((float)screenHeight * (0.06f + 0.15f * stress));

    Color edgeCol = (Color){ 4, 1, 10, (unsigned char)(stress * 220.0f) };

    // 4 Directional edge gradients pulling inward
    DrawRectangleGradientH(0, 0, depthX, screenHeight, edgeCol, BLANK);
    DrawRectangleGradientH(screenWidth - depthX, 0, depthX, screenHeight, BLANK, edgeCol);
    DrawRectangleGradientV(0, 0, screenWidth, depthY, edgeCol, BLANK);
    DrawRectangleGradientV(0, screenHeight - depthY, screenWidth, depthY, BLANK, edgeCol);

    // High-G red peripheral stress alarm (> 3.5G)
    if (gLoad > 3.5f) {
        float redStress = Clamp((gLoad - 3.5f) / 1.8f, 0.0f, 1.0f);
        Color redEdge = (Color){ 220, 35, 45, (unsigned char)(redStress * 95.0f) };
        DrawRectangleGradientH(0, 0, depthX / 2, screenHeight, redEdge, BLANK);
        DrawRectangleGradientH(screenWidth - depthX / 2, 0, depthX / 2, screenHeight, BLANK, redEdge);
        DrawRectangleGradientV(0, 0, screenWidth, depthY / 2, redEdge, BLANK);
        DrawRectangleGradientV(0, screenHeight - depthY / 2, screenWidth, depthY / 2, BLANK, redEdge);
    }
}

void FX_Unload(void) {
}
