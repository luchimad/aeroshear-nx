#include "race.h"
#include "terrain.h"
#include "config.h"
#include "hud.h"
#include "ui_theme.h"
#include "ui_core.h"
#include "audio.h"
#include "music.h"
#include "records.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void Race_Init(RaceTrack *race, BiomeType biome, Vector3 *startPlayerPos, float *startYaw) {
    race->totalCheckpoints = RACE_TOTAL_CHECKPOINTS; // 18
    race->currentCheckpoint = 0;
    race->raceTimer = 0.0f;
    race->countdownTimer = RACE_COUNTDOWN_SEC;
    race->isCountdown = true;
    race->isStarted = false;
    race->isFinished = false;
    race->finishTotalTime = 0.0f;
    race->bestSplitDelta = 0.0f;
    race->feedbackTimer = 0.0f;
    race->lastPassedGate = 0;
    race->isMissedWarning = false;
    race->halfwayAnnounced = false;
    race->outOfBoundsTimer = 0.0f;
    race->outOfBoundsGraceTimer = 0.0f;
    race->isOutOfBoundsWarning = false;
    race->cameraSnapRequested = false;
    race->currentBiome = biome;
    race->isNewRecord = false;
    race->qualifyingRank = -1;
    race->nameEntered = false;
    strcpy(race->pilotTag, "AAA");
    race->tagCursor = 0;

    const BiomeDefinition *bDef = Biome_Get(biome);
    snprintf(race->trackName, sizeof(race->trackName), "%s CIRCUIT", bDef->name);

    if (!race->isTexturesLoaded) {
        race->texPylon = LoadTexture(PATH_TEX_PYLON);
        race->texRing  = LoadTexture(PATH_TEX_RING);
        SetTextureFilter(race->texPylon, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(race->texRing,  TEXTURE_FILTER_BILINEAR);
        race->isTexturesLoaded = true;
    }

    float circuitRadius = 5200.0f;
    float parTimeStep = 5.2f;

    // 1. Coordenadas horizontales de los 18 checkpoints
    for (int i = 0; i < race->totalCheckpoints; i++) {
        RaceCheckpoint *cp = &race->checkpoints[i];
        cp->index = i;
        cp->isPassed = false;
        cp->splitTime = 0.0f;
        cp->parTime = (float)(i + 1) * parTimeStep;

        float angle = ((float)i / (float)race->totalCheckpoints) * 2.0f * PI;
        float rOffset = sinf(angle * 2.0f) * 600.0f + cosf(angle * 3.0f) * 300.0f;
        float currentRadius = circuitRadius + rOffset;

        float gx = cosf(angle) * currentRadius;
        float gz = sinf(angle) * currentRadius;

        float groundH = Terrain_GetHeight(gx, gz);
        if (bDef->hasWater && groundH < bDef->waterLevel) groundH = bDef->waterLevel;

        if (i == race->totalCheckpoints - 1) {
            cp->type = CHECKPOINT_FINISH_LINE;
            cp->width = RACE_PYLON_GATE_WIDTH * 1.15f;
            cp->height = RACE_PYLON_HEIGHT;
            cp->position = (Vector3){ gx, groundH, gz };
            cp->targetSpeed = SPEED_CRUISE * 1.3f;
        } else if (i == 0) {
            cp->type = CHECKPOINT_GROUND_PYLON;
            cp->width = RACE_PYLON_GATE_WIDTH;
            cp->height = RACE_PYLON_HEIGHT;
            cp->position = (Vector3){ gx, groundH, gz };
            cp->targetSpeed = SPEED_CRUISE;
        } else {
            bool prevWasAir = (i > 1 && race->checkpoints[i - 1].type == CHECKPOINT_AIR_RING);
            bool isGroundGate = prevWasAir ? true : ((i % 3) != 2);

            if (isGroundGate) {
                cp->type = CHECKPOINT_GROUND_PYLON;
                cp->width = RACE_PYLON_GATE_WIDTH;
                cp->height = RACE_PYLON_HEIGHT;
                cp->position = (Vector3){ gx, groundH, gz };
                cp->targetSpeed = SPEED_CRUISE * 1.1f;
            } else {
                cp->type = CHECKPOINT_AIR_RING;
                cp->width = RACE_RING_DIAMETER;
                cp->height = RACE_RING_DIAMETER;
                float airAlt = groundH + 185.0f + (float)((i * 19) % 45);
                cp->position = (Vector3){ gx, airAlt, gz };
                cp->targetSpeed = SPEED_CRUISE * 1.25f;
            }
        }
    }

    // 2. CÃLCULO PRECISO DE LA NORMAL (SOLUCIÃ“N BUG DETECCIÃ“N ANTICIPADA)
    // Se utiliza la tangente real de la curva: Normalize(cp[i+1] - cp[i-1])
    // para que el plano de cruce sea exactamente ortogonal a la trayectoria de vuelo
    for (int i = 0; i < race->totalCheckpoints; i++) {
        RaceCheckpoint *cp = &race->checkpoints[i];
        int prevIdx = (i - 1 + race->totalCheckpoints) % race->totalCheckpoints;
        int nextIdx = (i + 1) % race->totalCheckpoints;

        Vector3 prevPos = race->checkpoints[prevIdx].position;
        Vector3 nextPos = race->checkpoints[nextIdx].position;

        Vector3 curveTangent = Vector3Subtract(nextPos, prevPos);
        curveTangent.y = 0.0f; // Horizontal estricto para pilones terrestres
        if (Vector3Length(curveTangent) < 0.1f) curveTangent = (Vector3){ 0.0f, 0.0f, 1.0f };

        cp->direction = Vector3Normalize(curveTangent);
        cp->rightVec = (Vector3){ cp->direction.z, 0.0f, -cp->direction.x };

        if (cp->type == CHECKPOINT_AIR_RING) {
            Vector3 fullTangent = Vector3Normalize(Vector3Subtract(nextPos, prevPos));
            cp->direction = fullTangent;
            Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
            cp->rightVec = Vector3Normalize(Vector3CrossProduct(fullTangent, worldUp));
        }
    }

    // 3. Posicionar al jugador alineado frente al primer checkpoint
    if (startPlayerPos && startYaw) {
        RaceCheckpoint *cp0 = &race->checkpoints[0];
        Vector3 startDir = cp0->direction;
        float startDist = 480.0f;

        startPlayerPos->x = cp0->position.x - startDir.x * startDist;
        startPlayerPos->z = cp0->position.z - startDir.z * startDist;

        float startGround = Terrain_GetHeight(startPlayerPos->x, startPlayerPos->z);
        if (bDef->hasWater && startGround < bDef->waterLevel) startGround = bDef->waterLevel;
        startPlayerPos->y = startGround + 13.5f;
        race->spawnPosition = *startPlayerPos;

        Vector3 toGate = Vector3Normalize(Vector3Subtract(cp0->position, *startPlayerPos));
        *startYaw = atan2f(-toGate.x, toGate.z);
    }
}

static void Race_OnFinish(RaceTrack *race, const PlayerJet *player) {
    if (race->isFinished) return;
    race->isFinished = true;
    race->finishTotalTime = race->raceTimer;

    race->qualifyingRank = Records_CheckQualify(race->currentBiome, race->finishTotalTime);
    race->isNewRecord = (race->qualifyingRank == 0);
    if (race->qualifyingRank >= 0) {
        race->nameEntered = false;
        race->tagCursor = 0;
        strcpy(race->pilotTag, "AAA");
    } else {
        race->nameEntered = true;
    }
}

void Race_Update(RaceTrack *race, PlayerJet *player, float dt) {
    if (race->feedbackTimer > 0.0f) race->feedbackTimer -= dt;

    // ========================================================================
    // 1. CUENTA REGRESIVA CON VOCES REALES DEL LOCUTOR (ANNOUNCER)
    // ========================================================================
    if (race->isCountdown) {
        int prevStage = (int)(race->countdownTimer + 0.99f);
        race->countdownTimer -= dt;
        int newStage = (int)(race->countdownTimer + 0.99f);

        if (newStage < prevStage && newStage > 0) {
            Audio_PlayCountdownStage(newStage); // Voice_three, Voice_two, Voice_one
            Music_TriggerDucking(1.2f);
        }

        if (race->countdownTimer <= 0.0f) {
            race->isCountdown = false;
            race->isStarted = true;
            Audio_PlayCountdownStage(0); // Voice_go!
            Music_TriggerDucking(1.2f);
        }
        return;
    }

    if (race->isFinished) {
        if (!race->nameEntered && race->qualifyingRank >= 0) {
            int key = GetKeyPressed();
            while (key > 0) {
                if (key >= KEY_A && key <= KEY_Z) {
                    race->pilotTag[race->tagCursor] = (char)('A' + (key - KEY_A));
                    if (race->tagCursor < 2) race->tagCursor++;
                }
                key = GetKeyPressed();
            }

            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                race->pilotTag[race->tagCursor]++;
                if (race->pilotTag[race->tagCursor] > 'Z') race->pilotTag[race->tagCursor] = 'A';
            }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                race->pilotTag[race->tagCursor]--;
                if (race->pilotTag[race->tagCursor] < 'A') race->pilotTag[race->tagCursor] = 'Z';
            }

            if (IsGamepadAvailable(0)) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                    race->pilotTag[race->tagCursor]++;
                    if (race->pilotTag[race->tagCursor] > 'Z') race->pilotTag[race->tagCursor] = 'A';
                }
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                    race->pilotTag[race->tagCursor]--;
                    if (race->pilotTag[race->tagCursor] < 'A') race->pilotTag[race->tagCursor] = 'Z';
                }
            }

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_LEFT)) {
                if (race->tagCursor > 0) race->tagCursor--;
            }

            bool confirmChar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_RIGHT);
            if (IsGamepadAvailable(0) && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))) {
                confirmChar = true;
            }

            if (confirmChar) {
                if (race->tagCursor < 2) {
                    race->tagCursor++;
                } else {
                    const char *rank = "RANK S [ACE AVIATOR]";
                    if (race->finishTotalTime > 120.0f) rank = "RANK B [QUALIFIED]";
                    else if (race->finishTotalTime > 100.0f) rank = "RANK A [VETERAN]";
                    float spd = player ? player->speedKmh : 0.0f;
                    Records_InsertScore(race->currentBiome, race->qualifyingRank, race->pilotTag, race->finishTotalTime, spd, rank);
                    race->nameEntered = true;
                }
            }
        }
        return;
    }

    race->raceTimer += dt;

    if (race->currentCheckpoint >= race->totalCheckpoints) {
        Race_OnFinish(race, player);
        return;
    }

    // Locutor anuncia mitad de carrera
    if (!race->halfwayAnnounced && race->currentCheckpoint == (race->totalCheckpoints / 2)) {
        race->halfwayAnnounced = true;
        Audio_PlayHalfway(); // Voice_halfway!
        Music_TriggerDucking(1.5f);
    }

    // ========================================================================
    // 2. DETECCIÃ“N ANALÃTICA EXACTA DE CRUCE DE PLANO (SOLUCIÃ“N TIMING & HITBOX)
    // ========================================================================
    RaceCheckpoint *cp = &race->checkpoints[race->currentCheckpoint];

    Vector3 vPrev = Vector3Subtract(player->prevPosition, cp->position);
    Vector3 vCurr = Vector3Subtract(player->position, cp->position);

    float dPrev = Vector3DotProduct(vPrev, cp->direction);
    float dCurr = Vector3DotProduct(vCurr, cp->direction);
    float distCurr = Vector3Length(vCurr);

    // Cruce exacto en este frame: antes del plano en t-1 y cruzando o sobrepasando en t
    bool planeCrossed = (dPrev <= 0.0f && dCurr >= 0.0f);
    Vector3 crossPos = player->position;

    if (planeCrossed) {
        float denom = dCurr - dPrev;
        if (denom > 0.0001f) {
            float tCross = Clamp(-dPrev / denom, 0.0f, 1.0f);
            crossPos = Vector3Add(player->prevPosition, Vector3Scale(Vector3Subtract(player->position, player->prevPosition), tCross));
        }
    }

    // Tolerancia frontal estricta: solo se evalÃºa si el jugador estÃ¡ realmente atravesando la puerta
    float gateDetectionRadius = cp->width * 1.15f;

    if (planeCrossed && distCurr < gateDetectionRadius) {
        Vector3 toCross = Vector3Subtract(crossPos, cp->position);
        float lateralOffset = fabsf(Vector3DotProduct(toCross, cp->rightVec));

        bool isValidPass = false;
        bool isPerfectPass = false;

        if (cp->type == CHECKPOINT_GROUND_PYLON || cp->type == CHECKPOINT_FINISH_LINE) {
            // SOLUCIÃ“N HITBOX FLOTANTE:
            // La cota vertical se calcula con la altura del terreno REAL EN EL CORREDOR DE PASO DEL JUGADOR
            float localGroundH = Terrain_GetHeight(crossPos.x, crossPos.z);
            const BiomeDefinition *bDef = Biome_Get(Biome_GetActive());
            if (bDef && bDef->hasWater && localGroundH < bDef->waterLevel) localGroundH = bDef->waterLevel;

            float crossAGL = crossPos.y - localGroundH;

            // Ancho del portal: 65% del espacio entre pilones
            bool insideWidth = (lateralOffset <= (cp->width * 0.65f));
            // Altura vÃ¡lida: desde ras de suelo (+2m) hasta 15m por encima de la cima del pilÃ³n
            bool insideHeight = (crossAGL >= 2.0f && crossAGL <= (cp->height + 15.0f));

            if (insideWidth && insideHeight) {
                isValidPass = true;
                if (lateralOffset <= (cp->width * 0.28f) && (crossAGL >= 8.0f && crossAGL <= 45.0f)) {
                    isPerfectPass = true;
                }
            }
        } else if (cp->type == CHECKPOINT_AIR_RING) {
            float relY = crossPos.y - cp->position.y;
            float radialDist = sqrtf(lateralOffset * lateralOffset + relY * relY);
            if (radialDist <= (cp->width * 0.58f)) {
                isValidPass = true;
                if (radialDist <= (cp->width * 0.26f)) {
                    isPerfectPass = true;
                }
            }
        }

        if (isValidPass) {
            cp->isPassed = true;
            cp->splitTime = race->raceTimer;
            race->bestSplitDelta = race->raceTimer - cp->parTime;
            race->feedbackTimer = 2.2f;
            race->lastPassedGate = race->currentCheckpoint + 1;
            race->currentCheckpoint++;

            // Recarga instantÃ¡nea por paso perfecto (+25% Boost, +35 KE)
            if (isPerfectPass) {
                player->boostEnergy = Clamp(player->boostEnergy + BOOST_PERFECT_GATE_BONUS, 0.0f, player->maxBoostEnergy);
                player->kineticEnergy = Clamp(player->kineticEnergy + 35.0f, 0.0f, player->maxKineticEnergy);
                player->perfectGateTimer = 2.0f;
                Audio_PlayPerfectGate();
            } else {
                Audio_PlayCheckpoint();
            }

            if (race->currentCheckpoint >= race->totalCheckpoints) {
                Race_OnFinish(race, player);
            }
        } else {
            // Pasó fuera del marco: Missed Gate con penalización de +5s
            cp->isPassed = true;
            cp->splitTime = race->raceTimer;
            race->raceTimer += 5.0f;
            race->bestSplitDelta = race->raceTimer - cp->parTime;
            race->feedbackTimer = 2.5f;
            race->lastPassedGate = -(race->currentCheckpoint + 1);
            race->currentCheckpoint++;
            Audio_PlayWarning();

            if (race->currentCheckpoint >= race->totalCheckpoints) {
                Race_OnFinish(race, player);
            }
        }
    }

    // OOB TELEPORT: Si el jugador se desvía más de 1200m del trazado entre gates por más de 5s => TP a la última gate cruzada mirando a la próxima
    if (!race->isCountdown && !race->isFinished && race->totalCheckpoints > 0) {
        int nextIdx = race->currentCheckpoint;
        if (nextIdx >= race->totalCheckpoints) nextIdx = race->totalCheckpoints - 1;

        // Determinar el segmento activo de carrera (desde la última gate pasada hasta la próxima)
        Vector3 pStart = (nextIdx > 0) ? race->checkpoints[nextIdx - 1].position : race->spawnPosition;
        Vector3 pEnd = race->checkpoints[nextIdx].position;

        // Distancia horizontal (XZ) al segmento del trazado
        float segX = pEnd.x - pStart.x;
        float segZ = pEnd.z - pStart.z;
        float segLenSq = segX * segX + segZ * segZ;

        float t = 0.0f;
        if (segLenSq > 1.0f) {
            t = ((player->position.x - pStart.x) * segX + (player->position.z - pStart.z) * segZ) / segLenSq;
            t = Clamp(t, 0.0f, 1.0f);
        }

        float closeX = pStart.x + segX * t;
        float closeZ = pStart.z + segZ * t;
        float dx = player->position.x - closeX;
        float dz = player->position.z - closeZ;
        float distFromTrack = sqrtf(dx * dx + dz * dz);

        const float OOB_RADIUS = 1200.0f;
        const float OOB_GRACE = 5.0f;

        if (distFromTrack > OOB_RADIUS) {
            race->outOfBoundsTimer += dt;
            race->isOutOfBoundsWarning = true;

            if (race->outOfBoundsTimer >= OOB_GRACE) {
                // Penalización de tiempo
                race->raceTimer += 5.0f;

                Vector3 rp;
                float newHeading;

                if (nextIdx > 0) {
                    // Posicionar exactamente en la ÚLTIMA GATE que cruzó
                    int lastIdx = nextIdx - 1;
                    RaceCheckpoint *cpLast = &race->checkpoints[lastIdx];
                    RaceCheckpoint *cpNext = &race->checkpoints[nextIdx];

                    rp = cpLast->position;

                    // Ajuste de altitud según tipo de gate
                    float gh = Terrain_GetHeight(rp.x, rp.z);
                    const BiomeDefinition *bOob = Biome_Get(Biome_GetActive());
                    if (bOob && bOob->hasWater && gh < bOob->waterLevel) gh = bOob->waterLevel;

                    if (cpLast->type == CHECKPOINT_AIR_RING) {
                        rp.y = cpLast->position.y;
                        if (rp.y < gh + 18.0f) rp.y = gh + 18.0f;
                    } else {
                        // Pilones de tierra o meta
                        rp.y = gh + 18.0f;
                    }

                    // Mirando DIRECTO a la próxima gate
                    Vector3 toNext = Vector3Subtract(cpNext->position, rp);
                    Vector3 toNextH = (Vector3){ toNext.x, 0.0f, toNext.z };
                    if (Vector3Length(toNextH) < 0.1f) {
                        toNextH = cpLast->direction;
                    } else {
                        toNextH = Vector3Normalize(toNextH);
                    }
                    newHeading = atan2f(-toNextH.x, toNextH.z);
                } else {
                    // Si aún no cruzó ninguna gate, volver al spawn inicial mirando al CP 0
                    RaceCheckpoint *cp0 = &race->checkpoints[0];
                    rp = race->spawnPosition;

                    float gh = Terrain_GetHeight(rp.x, rp.z);
                    const BiomeDefinition *bOob = Biome_Get(Biome_GetActive());
                    if (bOob && bOob->hasWater && gh < bOob->waterLevel) gh = bOob->waterLevel;
                    if (rp.y < gh + 13.5f) rp.y = gh + 13.5f;

                    Vector3 toNext = Vector3Subtract(cp0->position, rp);
                    Vector3 toNextH = (Vector3){ toNext.x, 0.0f, toNext.z };
                    if (Vector3Length(toNextH) < 0.1f) {
                        toNextH = cp0->direction;
                    } else {
                        toNextH = Vector3Normalize(toNextH);
                    }
                    newHeading = atan2f(-toNextH.x, toNextH.z);
                }

                // Normalizar rumbo [0 .. 2*PI)
                if (newHeading < 0.0f) newHeading += 2.0f * PI;
                if (newHeading >= 2.0f * PI) newHeading -= 2.0f * PI;

                // Reubicar jugador y resetear cinemática
                player->position = rp;
                player->prevPosition = rp;
                player->heading = newHeading;
                player->forward = (Vector3){ -sinf(newHeading), 0.0f, cosf(newHeading) };
                player->pitch = 0.0f;
                player->targetPitch = 0.0f;
                player->roll = 0.0f;
                player->targetRoll = 0.0f;
                player->verticalVelocity = 0.0f;
                player->forwardSpeed = player->cruiseSpeed * 0.75f;
                player->velocity = Vector3Scale(player->forward, player->forwardSpeed);
                player->lateralSlip = 0.0f;
                player->driftAngle = 0.0f;
                player->wallImpactTimer = 0.0f;
                player->wallImpactTriggered = false;

                // Reset de estado OOB y solicitud de snap de cámara
                race->outOfBoundsTimer = 0.0f;
                race->isOutOfBoundsWarning = false;
                race->cameraSnapRequested = true;

                // Feedback visual y sonoro
                race->feedbackTimer = 3.0f;
                race->lastPassedGate = -999;
                Audio_PlayWarning();
            }
        } else {
            race->outOfBoundsTimer = 0.0f;
            race->isOutOfBoundsWarning = false;
        }
    } else {
        race->outOfBoundsTimer = 0.0f;
        race->isOutOfBoundsWarning = false;
    }
}

void Race_Draw3D(const RaceTrack *race, const Camera3D *camera) {
    float time = (float)GetTime();
    const BiomeDefinition *bDef = Biome_Get(Biome_GetActive());
    Color fogCol = bDef->fogHorizonColor;
    float fogStart = bDef->fogStart;
    float fogEnd   = bDef->fogEnd;

    for (int i = 0; i < race->totalCheckpoints; i++) {
        const RaceCheckpoint *cp = &race->checkpoints[i];
        float dist = Vector3Distance(camera->position, cp->position);
        if (dist > 4500.0f) continue;

        bool isActive = (i == race->currentCheckpoint);
        bool isPassed = cp->isPassed;

        float rawFog = Clamp((dist - fogStart) / (fogEnd - fogStart), 0.0f, 1.0f);
        float fogFactor = rawFog * rawFog * (3.0f - 2.0f * rawFog);

        Color gateTint = isPassed ? (Color){ 160, 230, 180, 200 } : WHITE;
        if (fogFactor > 0.01f) {
            gateTint.r = (unsigned char)Lerp((float)gateTint.r, (float)fogCol.r, fogFactor);
            gateTint.g = (unsigned char)Lerp((float)gateTint.g, (float)fogCol.g, fogFactor);
            gateTint.b = (unsigned char)Lerp((float)gateTint.b, (float)fogCol.b, fogFactor);
            gateTint.a = (unsigned char)Lerp((float)gateTint.a, (float)fogCol.a * 0.3f, fogFactor * 0.7f);
        }

        if (cp->type == CHECKPOINT_GROUND_PYLON || cp->type == CHECKPOINT_FINISH_LINE) {
            float halfW = cp->width * 0.5f;
            Vector3 pLeft = Vector3Subtract(cp->position, Vector3Scale(cp->rightVec, halfW));
            Vector3 pRight = Vector3Add(cp->position, Vector3Scale(cp->rightVec, halfW));

            float groundL = Terrain_GetHeight(pLeft.x, pLeft.z);
            float groundR = Terrain_GetHeight(pRight.x, pRight.z);
            if (bDef->hasWater) {
                if (groundL < bDef->waterLevel) groundL = bDef->waterLevel;
                if (groundR < bDef->waterLevel) groundR = bDef->waterLevel;
            }

            float pylonDrawH = RACE_PYLON_HEIGHT;
            pLeft.y = groundL + pylonDrawH * 0.5f;
            pRight.y = groundR + pylonDrawH * 0.5f;

            if (race->texPylon.id != 0) {
                DrawBillboard(*camera, race->texPylon, pLeft, pylonDrawH, gateTint);
                DrawBillboard(*camera, race->texPylon, pRight, pylonDrawH, gateTint);
            }

            if (isActive) {
                float pulse = sinf(time * 6.0f) * 0.3f + 0.7f;
                Color laserCol = (Color){ 0, 240, 255, (unsigned char)(220 * pulse) };
                Vector3 topL = { pLeft.x, pLeft.y + pylonDrawH * 0.5f, pLeft.z };
                Vector3 topR = { pRight.x, pRight.y + pylonDrawH * 0.5f, pRight.z };
                DrawLine3D(topL, topR, laserCol);
                DrawLine3D((Vector3){ topL.x, topL.y + 0.8f, topL.z }, (Vector3){ topR.x, topR.y + 0.8f, topR.z }, laserCol);
            }
        } else if (cp->type == CHECKPOINT_AIR_RING) {
            float ringDrawSize = cp->width;
            if (race->texRing.id != 0) {
                DrawBillboard(*camera, race->texRing, cp->position, ringDrawSize, gateTint);
            }

            if (isActive) {
                float pulse = sinf(time * 8.0f) * 0.25f + 0.75f;
                Color ringAura = (Color){ 255, 190, 40, (unsigned char)(200 * pulse) };
                DrawSphereWires(cp->position, cp->width * 0.48f, 6, 8, ringAura);
            }
        }
    }
}

void Race_DrawHUD(const RaceTrack *race, const PlayerJet *player, const Camera3D *camera, int screenWidth, int screenHeight) {
    Color colCyan  = UI_COLOR_AC4_CYAN;
    Color colAmber = UI_COLOR_AC4_AMBER;

    // 1. Cuenta regresiva en pantalla
    if (race->isCountdown) {
        int stage = (int)(race->countdownTimer + 0.99f);
        const char *cdText = "READY";
        Color cdColor = colAmber;

        if (stage == 3) { cdText = "THREE"; cdColor = colAmber; }
        else if (stage == 2) { cdText = "TWO"; cdColor = colAmber; }
        else if (stage == 1) { cdText = "ONE"; cdColor = colAmber; }
        else if (stage <= 0) { cdText = "FLY // GO!"; cdColor = UI_COLOR_AC4_GREEN; }

        UI_DrawAlertBanner((float)screenWidth * 0.5f, (float)screenHeight * 0.38f, cdText, cdColor, 1.0f);
        return;
    }

    // 2. CronÃ³metro en cabecera superior
    int topY = 16;
    int tSec = (int)race->raceTimer;
    int tMin = tSec / 60;
    int tS   = tSec % 60;
    int tC   = (int)((race->raceTimer - (float)tSec) * 100.0f);

    const char *timeStr = TextFormat("%02d:%02d.%02d", tMin, tS, tC);
    Vector2 tSz = UI_MeasureTextHud(timeStr, 22.0f);
    UI_DrawTextHud(timeStr, ((float)screenWidth - tSz.x) * 0.5f, (float)topY, 22.0f, UI_COLOR_STEEL_WHITE);

    const char *gateStr = TextFormat("GATE %02d / %02d", race->currentCheckpoint + 1, race->totalCheckpoints);
    Vector2 gSz = UI_MeasureTextHud(gateStr, 13.0f);
    UI_DrawTextHud(gateStr, ((float)screenWidth - gSz.x) * 0.5f, (float)(topY + 26), 13.0f, colCyan);

    const HighscoreEntry *circuitRec = Records_GetBest(race->currentBiome);
    if (circuitRec && circuitRec->isValid) {
        int rSec = (int)circuitRec->finishTime;
        int rMin = rSec / 60;
        int rS   = rSec % 60;
        int rC   = (int)((circuitRec->finishTime - (float)rSec) * 100.0f);
        const char *recStr = TextFormat("RECORD // %02d:%02d.%02d [%s]", rMin, rS, rC, circuitRec->pilotTag);
        Vector2 rSz = UI_MeasureTextHud(recStr, 10.0f);
        UI_DrawTextHud(recStr, ((float)screenWidth - rSz.x) * 0.5f, (float)(topY + 43), 10.0f, colAmber);
    }

    // 3. Navegador tÃ¡ctico 3D hacia la prÃ³xima puerta
    if (race->currentCheckpoint < race->totalCheckpoints) {
        const RaceCheckpoint *nextGate = &race->checkpoints[race->currentCheckpoint];
        Vector2 screenGate = GetWorldToScreenEx(nextGate->position, *camera, screenWidth, screenHeight);
        float distToGate = Vector3Distance(player->position, nextGate->position);

        Vector3 camForward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
        Vector3 toGateVec  = Vector3Normalize(Vector3Subtract(nextGate->position, camera->position));
        bool isInFront = (Vector3DotProduct(camForward, toGateVec) > 0.15f);

        if (isInFront && screenGate.x > 30 && screenGate.x < screenWidth - 30 &&
            screenGate.y > 30 && screenGate.y < screenHeight - 30) {
            int gx = (int)screenGate.x;
            int gy = (int)screenGate.y;

            int dSize = 14;
            DrawLine(gx, gy - dSize, gx + dSize, gy, colCyan);
            DrawLine(gx + dSize, gy, gx, gy + dSize, colCyan);
            DrawLine(gx, gy + dSize, gx - dSize, gy, colCyan);
            DrawLine(gx - dSize, gy, gx, gy - dSize, colCyan);

            const char *distTxt = (distToGate > 1000.0f) ? TextFormat("%.1f KM", distToGate * 0.001f) : TextFormat("%.0f M", distToGate);
            Vector2 dtw = UI_MeasureTextHud(distTxt, 11.0f);
            UI_DrawTextHud(distTxt, (float)gx - dtw.x * 0.5f, (float)(gy + dSize + 5), 11.0f, UI_COLOR_STEEL_WHITE);
        }
    }

    // 4. Banner de advertencia de fuera de curso (countdown hacia TP)
    if (race->isOutOfBoundsWarning && !race->isFinished && !race->isCountdown) {
        float remaining = fmaxf(0.0f, 5.0f - race->outOfBoundsTimer);
        Color warnOob = (Color){ 255, 140, 35, 240 };
        int oobY = (int)((float)screenHeight * 0.22f);
        UI_DrawAlertBanner((float)screenWidth * 0.5f, (float)oobY, TextFormat("OUT OF COURSE // RETURNING IN %.1fs", remaining), warnOob, 1.0f);
    }

    // 5. Banner de feedback de cruce o reposición
    if (race->feedbackTimer > 0.0f) {
        float alpha = fminf(1.0f, race->feedbackTimer * 1.5f);
        int pmy = (int)((float)screenHeight * 0.14f);

        if (race->lastPassedGate == -999) {
            Color missCol = (Color){ 255, 65, 75, (unsigned char)(255 * alpha) };
            UI_DrawAlertBanner((float)screenWidth * 0.5f, (float)pmy, "OUT OF COURSE // REPOSITIONED  [ +5.0s ]", missCol, alpha);
        } else if (race->lastPassedGate < 0) {
            int gateNum = -race->lastPassedGate;
            Color missCol = (Color){ 255, 65, 75, (unsigned char)(255 * alpha) };
            UI_DrawAlertBanner((float)screenWidth * 0.5f, (float)pmy, TextFormat("GATE %02d MISSED  [ +5.0s PENALTY ]", gateNum), missCol, alpha);
        } else {
            float splitD = race->bestSplitDelta;
            const char *baseMsg = (splitD <= 0.0f) ? 
                TextFormat("GATE %02d CLEARED  [ %.2fs ]", race->lastPassedGate, splitD) :
                TextFormat("GATE %02d CLEARED  [ +%.2fs ]", race->lastPassedGate, splitD);

            if (player->perfectGateTimer > 0.0f) {
                Color goldCol = (Color){ 255, 185, 45, (unsigned char)(255 * alpha) };
                UI_DrawAlertBanner((float)screenWidth * 0.5f, (float)pmy, TextFormat("%s  //  PERFECT RECHARGE +25%%", baseMsg), goldCol, alpha);
            } else {
                Color flashCol = (Color){ 120, 235, 185, (unsigned char)(255 * alpha) };
                UI_DrawAlertBanner((float)screenWidth * 0.5f, (float)pmy, baseMsg, flashCol, alpha);
            }
        }
    }

    // 5. Pantalla de resultados al finalizar
    if (race->isFinished) {
        DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 2, 4, 8, 225 });

        int cardW = 560;
        int cardH = 385;
        int cardX = (screenWidth - cardW) / 2;
        int cardY = (screenHeight - cardH) / 2;
        Rectangle debriefRec = { (float)cardX, (float)cardY, (float)cardW, (float)cardH };

        UI_DrawGlassPanel(debriefRec, "TACTICAL FLIGHT DEBRIEFING", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);
        UI_DrawTextTitle("SORTIE COMPLETE // FLIGHT RECORD", (float)(cardX + 35), (float)(cardY + 26), 18.0f, UI_COLOR_STEEL_WHITE);
        DrawLine(cardX + 35, cardY + 52, cardX + cardW - 35, cardY + 52, (Color){ 35, 70, 95, 140 });

        UI_DrawTextMenu("TOTAL FLIGHT DURATION:", (float)(cardX + 35), (float)(cardY + 66), 11.0f, UI_COLOR_MUTED_TEXT);
        UI_DrawTextHud(TextFormat("%02d:%02d.%02d", tMin, tS, tC), (float)(cardX + 35), (float)(cardY + 82), 26.0f, UI_COLOR_STEEL_WHITE);

        const HighscoreEntry *curRec = Records_GetBest(race->currentBiome);
        if (curRec && curRec->isValid) {
            int bSec = (int)curRec->finishTime;
            int bMin = bSec / 60;
            int bS   = bSec % 60;
            int bC   = (int)((curRec->finishTime - (float)bSec) * 100.0f);
            UI_DrawTextMenu(TextFormat("CIRCUIT RECORD:         %02d:%02d.%02d [%s - %s]", bMin, bS, bC, curRec->pilotTag, curRec->rank),
                           (float)(cardX + 35), (float)(cardY + 116), 12.0f, UI_COLOR_AC4_AMBER);
        }

        if (race->isNewRecord) {
            float pulse = sinf((float)GetTime() * 8.0f) * 0.25f + 0.75f;
            Color goldCol = (Color){ 255, 205, 50, (unsigned char)(255 * pulse) };
            UI_DrawTextTitle("* NEW ALL-TIME RECORD! *", (float)(cardX + 35), (float)(cardY + 138), 14.0f, goldCol);
        } else if (race->qualifyingRank >= 0) {
            float pulse = sinf((float)GetTime() * 8.0f) * 0.25f + 0.75f;
            Color cyanCol = (Color){ 68, 224, 195, (unsigned char)(255 * pulse) };
            UI_DrawTextTitle(TextFormat("* QUALIFIED FOR TOP 5 [#%02d] *", race->qualifyingRank + 1), (float)(cardX + 35), (float)(cardY + 138), 13.0f, cyanCol);
        }

        int nextStatsY = (race->isNewRecord || race->qualifyingRank >= 0) ? 164 : 144;
        UI_DrawTextMenu(TextFormat("CHECKPOINTS CLEARED:    %02d / %02d", race->totalCheckpoints, race->totalCheckpoints), (float)(cardX + 35), (float)(cardY + nextStatsY), 13.0f, UI_COLOR_STEEL_WHITE);
        UI_DrawTextMenu(TextFormat("AIRSPEED REACHED:       %04d KTS (MACH %.2f)", (int)player->speedKnots, player->machNumber), (float)(cardX + 35), (float)(cardY + nextStatsY + 24), 13.0f, UI_COLOR_STEEL_WHITE);

        const char *rank = "PILOT EVALUATION:  RANK S [ACE AVIATOR]";
        Color rankCol = UI_COLOR_AC4_GREEN;
        if (race->finishTotalTime > 120.0f) { rank = "PILOT EVALUATION:  RANK B [QUALIFIED]"; rankCol = UI_COLOR_AC4_AMBER; }
        else if (race->finishTotalTime > 100.0f) { rank = "PILOT EVALUATION:  RANK A [VETERAN]"; rankCol = UI_COLOR_AC4_CYAN; }

        UI_DrawTextTitle(rank, (float)(cardX + 35), (float)(cardY + nextStatsY + 58), 15.0f, rankCol);
        DrawLine(cardX + 35, cardY + cardH - 50, cardX + cardW - 35, cardY + cardH - 50, (Color){ 35, 70, 95, 140 });

        // Caja de entrada de iniciales si clasificó en el Top 5
        if (!race->nameEntered && race->qualifyingRank >= 0) {
            int boxW = cardW - 70;
            int boxH = 100;
            int boxX = cardX + 35;
            int boxY = cardY + cardH - 115;
            Rectangle boxRec = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };

            DrawRectangleRounded(boxRec, 0.15f, 4, (Color){ 4, 12, 22, 245 });
            DrawRectangleRoundedLinesEx(boxRec, 0.15f, 4, 1.2f, UI_COLOR_AC4_AMBER);

            const char *topStr = TextFormat("HALL OF FAME ENTRY // POSITION #%02d", race->qualifyingRank + 1);
            UI_DrawTextHud(topStr, (float)(boxX + 16), (float)(boxY + 12), 11.0f, UI_COLOR_AC4_AMBER);
            UI_DrawTextHud("ENTER 3-LETTER PILOT TAG:", (float)(boxX + 16), (float)(boxY + 28), 10.0f, UI_COLOR_STEEL_WHITE);

            int charStartX = boxX + boxW - 145;
            int charY = boxY + 14;
            for (int c = 0; c < 3; c++) {
                Rectangle cRec = { (float)(charStartX + c * 38), (float)charY, 32.0f, 36.0f };
                bool isCursor = (c == race->tagCursor);
                Color bCol = isCursor ? UI_COLOR_AC4_CYAN : (Color){ 40, 75, 110, 160 };

                DrawRectangleRounded(cRec, 0.2f, 4, isCursor ? (Color){ 16, 40, 70, 230 } : (Color){ 8, 20, 36, 180 });
                DrawRectangleRoundedLinesEx(cRec, 0.2f, 4, isCursor ? 2.0f : 1.0f, bCol);

                char letterStr[2] = { race->pilotTag[c], '\0' };
                Vector2 lSz = UI_MeasureTextTitle(letterStr, 18.0f);
                UI_DrawTextTitle(letterStr, cRec.x + (cRec.width - lSz.x) * 0.5f, cRec.y + (cRec.height - lSz.y) * 0.5f, 18.0f, UI_COLOR_STEEL_WHITE);

                if (isCursor) {
                    float blink = sinf((float)GetTime() * 8.0f) * 0.5f + 0.5f;
                    if (blink > 0.3f) {
                        DrawLine((int)cRec.x + 4, (int)(cRec.y + cRec.height - 4), (int)(cRec.x + cRec.width - 4), (int)(cRec.y + cRec.height - 4), UI_COLOR_AC4_CYAN);
                    }
                }
            }

            UI_DrawTextHud("[W/S / UP/DN]: LETTER    [ENTER / (A)]: CONFIRM CHAR    [A-Z]: DIRECT TYPE",
                           (float)(boxX + 16), (float)(boxY + boxH - 22), 9.0f, (Color){ 110, 155, 185, 200 });
        } else {
            UI_DrawTextMenu("[ ESC ]: RETURN TO MENU      [ R / SPACE / (A) ]: RESTART", (float)(cardX + 35), (float)(cardY + cardH - 34), 12.0f, (Color){ 110, 155, 185, 200 });
        }
    }
}

void Race_Unload(RaceTrack *race) {
    if (race->isTexturesLoaded) {
        if (race->texPylon.id != 0) UnloadTexture(race->texPylon);
        if (race->texRing.id != 0)  UnloadTexture(race->texRing);
        race->isTexturesLoaded = false;
    }
}

