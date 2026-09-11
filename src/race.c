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

void Race_Init(RaceTrack *race, BiomeType biome, unsigned int seed, Vector3 *startPlayerPos, float *startYaw) {
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
    race->runSeed = seed;
    race->isNewRecord = false;
    race->qualifyingRank = -1;
    race->nameEntered = false;
    strcpy(race->pilotTag, "AAA");
    race->tagCursor = 0;

    race->maxSpeedKmh = 0.0f;
    race->maxSpeedKnots = 0.0f;
    race->maxMach = 0.0f;

    // Inicialización N.A.D.I.A. // Réseau-Orbital Avionics
    race->nadiaActive = false;
    race->nadiaTimer = 0.0f;
    race->nadiaMaxDuration = 2.8f;
    race->nadiaTitle[0] = '\0';
    race->nadiaSubtitle[0] = '\0';
    race->nadiaDistance[0] = '\0';
    race->nadiaWarnCooldown = 0.0f;
    Audio_ClearNadiaQueue();

    const BiomeDefinition *bDef = Biome_Get(biome);
    snprintf(race->trackName, sizeof(race->trackName), "%s CIRCUIT", bDef->name);

    if (!race->isTexturesLoaded) {
        race->texPylon = LoadTexture(PATH_TEX_PYLON);
        race->texRing  = LoadTexture(PATH_TEX_RING);
        race->billboardShader = LoadShaderFromMemory(NULL, s_alphaDiscardFs);
        SetTextureFilter(race->texPylon, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(race->texRing,  TEXTURE_FILTER_BILINEAR);
        race->isTexturesLoaded = true;
    }

    float circuitRadius = 20800.0f; // Distancia entre waypoints ampliada 200% (de 10400m a 20800m)
    float parTimeStep = 10.4f;      // Adaptado proporcionalmente con velocidad duplicada (880 m/s)
    float seedPhase = (float)(seed % 1000) * 0.006283f;

    // 1. Coordenadas horizontales de los 18 checkpoints
    for (int i = 0; i < race->totalCheckpoints; i++) {
        RaceCheckpoint *cp = &race->checkpoints[i];
        cp->index = i;
        cp->isPassed = false;
        cp->splitTime = 0.0f;
        cp->parTime = (float)(i + 1) * parTimeStep;

        float angle = ((float)i / (float)race->totalCheckpoints) * 2.0f * PI;
        float rOffset = sinf(angle * 2.0f + seedPhase) * 2400.0f + cosf(angle * 3.0f + seedPhase * 1.5f) * 1200.0f;
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
        float startDist = 1200.0f; // Distancia de arranque ampliada proporcionalmente

        startPlayerPos->x = cp0->position.x - startDir.x * startDist;
        startPlayerPos->z = cp0->position.z - startDir.z * startDist;

        float startGround = Terrain_GetHeight(startPlayerPos->x, startPlayerPos->z);
        if (bDef->hasWater && startGround < bDef->waterLevel) startGround = bDef->waterLevel;
        startPlayerPos->y = startGround + 21.0f;
        race->spawnPosition = *startPlayerPos;

        Vector3 toGate = Vector3Normalize(Vector3Subtract(cp0->position, *startPlayerPos));
        *startYaw = atan2f(-toGate.x, toGate.z);
    }
}

static void Race_OnFinish(RaceTrack *race, const PlayerJet *player) {
    if (race->isFinished) return;
    race->isFinished = true;
    race->finishTotalTime = race->raceTimer;
    race->nadiaActive = false;

    // Frenar y detener completamente el hovercraft al cruzar la meta
    if (player) {
        PlayerJet *p = (PlayerJet*)player;
        p->forwardSpeed = 0.0f;
        p->velocity = (Vector3){ 0 };
        p->throttle = 0.0f;
        p->isAfterburner = false;
        p->isAirbrake = false;
        p->speedKmh = 0.0f;
        p->speedKnots = 0.0f;
        p->machNumber = 0.0f;
    }

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

// ============================================================================
// SISTEMA DE COPILOTO TÁCTICO N.A.D.I.A. // RÉSEAU-ORBITAL AVIONICS
// ============================================================================
void Nadia_TriggerGateCallout(RaceTrack *race, const PlayerJet *player, int nextIdx) {
    if (!race || nextIdx < 0 || nextIdx >= race->totalCheckpoints) return;

    RaceCheckpoint *cpNext = &race->checkpoints[nextIdx];
    Vector3 pCurr = (player) ? player->position : race->spawnPosition;
    Vector3 toNext = Vector3Subtract(cpNext->position, pCurr);
    float dist = Vector3Length(toNext);

    // Formatear distancia en metros o kilómetros
    if (dist >= 1000.0f) {
        snprintf(race->nadiaDistance, sizeof(race->nadiaDistance), "%.1f KM", dist * 0.001f);
    } else {
        snprintf(race->nadiaDistance, sizeof(race->nadiaDistance), "%.0f M", dist);
    }

    // Ángulo horizontal relativo al rumbo actual del piloto
    float desiredAngle = atan2f(-toNext.x, toNext.z);
    float currentHeading = (player) ? player->heading : 0.0f;
    float angleDiff = desiredAngle - currentHeading;
    while (angleDiff > PI)  angleDiff -= 2.0f * PI;
    while (angleDiff < -PI) angleDiff += 2.0f * PI;
    float degDiff = angleDiff * RAD2DEG;

    // Diferencia de cota vertical
    float deltaY = cpNext->position.y - pCurr.y;

    NadiaCallout callout = NADIA_CALLOUT_NONE;

    if (cpNext->type == CHECKPOINT_FINISH_LINE) {
        snprintf(race->nadiaTitle, sizeof(race->nadiaTitle), "FINAL GATE : SURFACE [FINISH LINE]");
        snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : MAXIMUM THRUST // FLAT OUT");
        callout = NADIA_CALLOUT_FINAL_GATE;
    } else if (cpNext->type == CHECKPOINT_AIR_RING) {
        snprintf(race->nadiaTitle, sizeof(race->nadiaTitle), "TARGET GATE %02d : AERIAL [RING]", nextIdx + 1);
        if (degDiff > 16.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : CLIMB // LONG RIGHT");
            callout = NADIA_CALLOUT_AIR_RIGHT;
        } else if (degDiff < -16.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : CLIMB // LONG LEFT");
            callout = NADIA_CALLOUT_AIR_LEFT;
        } else {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : CLIMB // HIGH APEX");
            callout = NADIA_CALLOUT_AIR_CLIMB;
        }
    } else {
        // CHECKPOINT_GROUND_PYLON (Superficie)
        snprintf(race->nadiaTitle, sizeof(race->nadiaTitle), "TARGET GATE %02d : SURFACE [PYLON]", nextIdx + 1);

        bool prevWasAirGate = (nextIdx > 0 && race->checkpoints[nextIdx - 1].type == CHECKPOINT_AIR_RING);

        if (prevWasAirGate && deltaY < -28.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : DESCENT // GROUND SHEAR");
            callout = NADIA_CALLOUT_SURF_FLAT;
        } else if (degDiff > 32.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : HARD RIGHT // AIRBRAKE APEX");
            callout = NADIA_CALLOUT_SURF_RIGHT_HARD;
        } else if (degDiff > 12.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : LONG RIGHT // BANK ANGLE");
            callout = NADIA_CALLOUT_SURF_RIGHT_LONG;
        } else if (degDiff < -32.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : HARD LEFT // AIRBRAKE APEX");
            callout = NADIA_CALLOUT_SURF_LEFT_HARD;
        } else if (degDiff < -12.0f) {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : LONG LEFT // BANK ANGLE");
            callout = NADIA_CALLOUT_SURF_LEFT_LONG;
        } else {
            snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "VECTOR : STRAIGHT // FLAT OUT");
            callout = NADIA_CALLOUT_SURF_FLAT;
        }
    }

    race->nadiaActive = true;
    race->nadiaTimer = 2.8f;
    race->nadiaMaxDuration = 2.8f;

    Audio_PlayNadia(callout);
}

void Nadia_TriggerStallWarning(RaceTrack *race) {
    if (!race || race->isFinished || race->isCountdown) return;
    if (race->nadiaWarnCooldown > 0.0f) return;
    if (Audio_IsNadiaPlaying()) return;

    race->nadiaActive = true;
    race->nadiaTimer = 2.4f;
    race->nadiaMaxDuration = 2.4f;
    race->nadiaWarnCooldown = 8.0f; // Cooldown para no saturar

    snprintf(race->nadiaTitle, sizeof(race->nadiaTitle), "SYSTEM ALERT : KINETIC ENERGY");
    snprintf(race->nadiaSubtitle, sizeof(race->nadiaSubtitle), "CAUTION : AIRFLOW COMPRESSION CRITICAL");
    snprintf(race->nadiaDistance, sizeof(race->nadiaDistance), "STALL");

    Audio_PlayNadia(NADIA_CALLOUT_WARN_KE);
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
            Nadia_TriggerGateCallout(race, player, 0); // Anuncio de primera puerta
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
                    const char *rank = "RANK S [ACE PILOT]";
                    if (race->finishTotalTime > 120.0f) rank = "RANK B [QUALIFIED PILOT]";
                    else if (race->finishTotalTime > 100.0f) rank = "RANK A [VETERAN PILOT]";
                    Records_InsertScore(race->currentBiome, race->qualifyingRank, race->pilotTag, race->finishTotalTime, race->maxSpeedKmh, rank, race->runSeed);
                    race->nameEntered = true;
                }
            }
        }
        return;
    }

    race->raceTimer += dt;

    // Actualización de temporizadores N.A.D.I.A. (acoplado a la cola de audio)
    if (race->nadiaTimer > 0.0f) {
        race->nadiaTimer -= dt;
        if (race->nadiaTimer <= 0.0f) {
            race->nadiaActive = false;
        }
    }
    // Si Nadia sigue hablando por radio o hay mensajes encolados, mantener widget vivo
    if (Audio_IsNadiaPlaying() || Audio_GetNadiaQueueCount() > 0) {
        race->nadiaActive = true;
        if (race->nadiaTimer < 0.65f) race->nadiaTimer = 0.65f;
    }
    if (race->nadiaWarnCooldown > 0.0f) {
        race->nadiaWarnCooldown -= dt;
    }

    if (player) {
        if (player->speedKmh > race->maxSpeedKmh) race->maxSpeedKmh = player->speedKmh;
        if (player->speedKnots > race->maxSpeedKnots) race->maxSpeedKnots = player->speedKnots;
        if (player->machNumber > race->maxMach) race->maxMach = player->machNumber;

        // Actualizar distancia en tiempo real dentro del cartelito de N.A.D.I.A.
        if (race->nadiaActive && race->currentCheckpoint < race->totalCheckpoints) {
            float dGate = Vector3Distance(player->position, race->checkpoints[race->currentCheckpoint].position);
            if (dGate >= 1000.0f) {
                snprintf(race->nadiaDistance, sizeof(race->nadiaDistance), "%.1f KM", dGate * 0.001f);
            } else {
                snprintf(race->nadiaDistance, sizeof(race->nadiaDistance), "%.0f M", dGate);
            }
        }

        // Alerta de pérdida de energía cinética (Stall)
        if (player->isStalling && !race->isFinished && !race->isCountdown) {
            Nadia_TriggerStallWarning(race);
        }
    }

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

            // Recarga instantánea por paso perfecto (+25% Boost, +52.5 KE, +50%)
            if (isPerfectPass) {
                player->boostEnergy = Clamp(player->boostEnergy + BOOST_PERFECT_GATE_BONUS, 0.0f, player->maxBoostEnergy);
                player->kineticEnergy = Clamp(player->kineticEnergy + 52.5f, 0.0f, player->maxKineticEnergy);
                player->perfectGateTimer = 2.0f;
                Audio_PlayPerfectGate();
                Audio_PlayNadia(NADIA_CALLOUT_PERFECT_GATE);
            } else {
                Audio_PlayCheckpoint();
            }

            if (race->currentCheckpoint >= race->totalCheckpoints) {
                Race_OnFinish(race, player);
            } else {
                Nadia_TriggerGateCallout(race, player, race->currentCheckpoint);
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
            } else {
                Nadia_TriggerGateCallout(race, player, race->currentCheckpoint);
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

        const float OOB_RADIUS = 4800.0f; // Distancia de teleport fuera de pista ampliada en un 200% (era 2400m)
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
        if (dist > 14000.0f) continue;

        bool isActive = (i == race->currentCheckpoint);
        bool isPassed = cp->isPassed;

        float drawFogEnd = (fogEnd < 3000.0f) ? 4800.0f : fogEnd;
        float rawFog = Clamp((dist - fogStart) / (drawFogEnd - fogStart), 0.0f, 1.0f);
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
                if (race->billboardShader.id != 0) BeginShaderMode(race->billboardShader);
                DrawBillboard(*camera, race->texPylon, pLeft, pylonDrawH, gateTint);
                DrawBillboard(*camera, race->texPylon, pRight, pylonDrawH, gateTint);
                if (race->billboardShader.id != 0) EndShaderMode();
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
                if (race->billboardShader.id != 0) BeginShaderMode(race->billboardShader);
                DrawBillboard(*camera, race->texRing, cp->position, ringDrawSize, gateTint);
                if (race->billboardShader.id != 0) EndShaderMode();
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

    // ========================================================================
    // 6. WIDGET DE AVIONICA Y COPILOTO N.A.D.I.A. (RÉSEAU-ORBITAL // 108.4 MHz)
    // ========================================================================
    if (Audio_IsNadiaEnabled() && race->nadiaActive && race->nadiaTimer > 0.0f && !race->isFinished) {
        float alpha = 1.0f;
        if (race->nadiaTimer < 0.35f) alpha = race->nadiaTimer / 0.35f;
        else if (race->nadiaTimer > race->nadiaMaxDuration - 0.25f) alpha = (race->nadiaMaxDuration - race->nadiaTimer) / 0.25f;
        alpha = Clamp(alpha, 0.0f, 1.0f);

        // Medir dimensiones del texto para garantizar que NUNCA se salga del marco ni solape con el badge de distancia
        Vector2 tSz = UI_MeasureTextTitle(race->nadiaTitle, 11.5f);
        Vector2 sSz = UI_MeasureTextMenu(race->nadiaSubtitle, 10.5f);
        Vector2 dSz = UI_MeasureTextHud(race->nadiaDistance, 11.0f);
        int distW = (int)dSz.x + 18;
        float maxTextW = fmaxf(tSz.x, sSz.x);

        int textXOffset = 78;
        int minRequiredW = (int)((float)textXOffset + maxTextW + (float)distW + 36.0f);
        int boxW = (int)fmaxf((float)minRequiredW, 560.0f);
        if (boxW > (int)((float)screenWidth * 0.75f)) {
            boxW = (int)((float)screenWidth * 0.75f);
        }
        int boxH = 56;
        int baseBoxX = (int)fmaxf(36.0f, (float)screenWidth * 0.035f);
        float slide = (1.0f - alpha) * -20.0f;
        int boxX = (int)((float)baseBoxX + slide);
        int boxY = 88; // Ubicado con margen de respiración bajo el banner OSD de audio

        Rectangle nRec = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };

        // 1. Sombra suave de elevación
        DrawRectangleRounded((Rectangle){ nRec.x, nRec.y + 3, nRec.width, nRec.height }, 0.18f, 4, (Color){ 0, 4, 10, (unsigned char)(110 * alpha) });

        // 2. Cuerpo Glassy oscuro de obsidiana
        Color glassBg = (Color){ 6, 14, 26, (unsigned char)(215 * alpha) };
        DrawRectangleRounded(nRec, 0.18f, 4, glassBg);

        // 3. Brillo especular superior de vidrio líquido
        DrawRectangleGradientV((int)nRec.x + 2, (int)nRec.y + 1, (int)nRec.width - 4, 24,
                               (Color){ 255, 255, 255, (unsigned char)(32 * alpha) },
                               (Color){ 255, 255, 255, 0 });
        DrawLine((int)nRec.x + 8, (int)nRec.y, (int)(nRec.x + nRec.width - 8), (int)nRec.y, (Color){ 255, 255, 255, (unsigned char)(90 * alpha) });

        // 4. Borde esmerilado con acento Réseau-Orbital Magenta
        Color borderCol = (Color){ 255, 0, 127, (unsigned char)(165 * alpha) };
        DrawRectangleRoundedLinesEx(nRec, 0.18f, 4, 1.0f, borderCol);

        // 5. Píldora de Título de la Transmisión
        int tabW = 244;
        int tabH = 17;
        Rectangle tabRec = { nRec.x + 14, nRec.y - 8, (float)tabW, (float)tabH };
        DrawRectangleRounded(tabRec, 0.35f, 4, (Color){ 14, 24, 42, (unsigned char)(240 * alpha) });
        DrawRectangleRoundedLinesEx(tabRec, 0.35f, 4, 1.0f, (Color){ 255, 0, 127, (unsigned char)(180 * alpha) });

        // Dot de transmisión activa (pulsa con el tiempo)
        float pulse = sinf((float)GetTime() * 12.0f) * 0.5f + 0.5f;
        Color dotCol = (Color){ 255, 0, 127, (unsigned char)((160 + 95 * pulse) * alpha) };
        DrawCircle((int)tabRec.x + 9, (int)tabRec.y + 8, 3.2f, dotCol);

        UI_DrawTextHud("RESEAU-ORBITAL // N.A.D.I.A. 108.4MHz", tabRec.x + 18, tabRec.y + 3, 9.5f, (Color){ 225, 245, 255, (unsigned char)(245 * alpha) });

        // 6. Avatar Vectorial de N.A.D.I.A. (Órbita e Icono)
        int iconCenterX = (int)nRec.x + 28;
        int iconCenterY = (int)nRec.y + 30;

        // Anillo de órbita giratorio
        float orbitAngle = (float)GetTime() * 2.8f;
        DrawCircleLines(iconCenterX, iconCenterY, 13.0f, (Color){ 45, 95, 145, (unsigned char)(140 * alpha) });
        Vector2 satPos = {
            (float)iconCenterX + cosf(orbitAngle) * 13.0f,
            (float)iconCenterY + sinf(orbitAngle) * 6.5f
        };
        DrawCircleV(satPos, 2.5f, (Color){ 255, 0, 127, (unsigned char)(255 * alpha) });
        DrawCircle(iconCenterX, iconCenterY, 3.5f, (Color){ 0, 240, 255, (unsigned char)(220 * alpha) });

        // 4 Barras de Audio Equalizer oscilando en tiempo real
        int eqStartX = iconCenterX + 22;
        int eqBaseY = iconCenterY + 10;
        for (int b = 0; b < 4; b++) {
            float barVal = sinf((float)GetTime() * 22.0f + (float)b * 1.6f) * 0.5f + 0.5f;
            int barH = 4 + (int)(barVal * 15.0f);
            DrawRectangle(eqStartX + b * 5, eqBaseY - barH, 3, barH, (Color){ 0, 240, 255, (unsigned char)(190 * alpha) });
        }

        // 7. Texto de Telemetría Táctica
        int textX = (int)nRec.x + textXOffset;
        int line1Y = (int)nRec.y + 13;
        int line2Y = (int)nRec.y + 32;

        UI_DrawTextTitle(race->nadiaTitle, (float)textX, (float)line1Y, 11.5f, (Color){ 240, 248, 255, (unsigned char)(255 * alpha) });
        UI_DrawTextMenu(race->nadiaSubtitle, (float)textX, (float)line2Y, 10.5f, (Color){ 0, 240, 255, (unsigned char)(235 * alpha) });

        // 8. Distancia / ETA a la derecha (sin colisión)
        int distH = 20;
        int distX = (int)(nRec.x + nRec.width - distW - 14);
        int distY = (int)nRec.y + 18;

        Rectangle distRec = { (float)distX, (float)distY, (float)distW, (float)distH };
        DrawRectangleRounded(distRec, 0.30f, 4, (Color){ 10, 22, 38, (unsigned char)(190 * alpha) });
        DrawRectangleRoundedLinesEx(distRec, 0.30f, 4, 1.0f, (Color){ 255, 185, 45, (unsigned char)(160 * alpha) });
        UI_DrawTextHud(race->nadiaDistance, (float)(distX + 9), (float)(distY + 4), 11.0f, (Color){ 255, 195, 60, (unsigned char)(255 * alpha) });
    }

    // 5. Pantalla de resultados al finalizar
    if (race->isFinished) {
        DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 2, 4, 8, 225 });

        int cardW = 620;
        int cardH = 415;
        int cardX = (screenWidth - cardW) / 2;
        int cardY = (screenHeight - cardH) / 2;
        Rectangle debriefRec = { (float)cardX, (float)cardY, (float)cardW, (float)cardH };

        UI_DrawGlassPanel(debriefRec, "HCRB RACE DEBRIEFING", UI_COLOR_AC4_CYAN, UI_COLOR_PANEL_BG);
        UI_DrawTextTitle("SORTIE COMPLETE // HCRB RECORD", (float)(cardX + 35), (float)(cardY + 26), 18.0f, UI_COLOR_STEEL_WHITE);
        UI_DrawTextHud(TextFormat("CIRCUIT SEED : %06u", race->runSeed), (float)(cardX + cardW - 220), (float)(cardY + 28), 12.0f, UI_COLOR_AC4_CYAN);
        DrawLine(cardX + 35, cardY + 52, cardX + cardW - 35, cardY + 52, (Color){ 35, 70, 95, 140 });

        UI_DrawTextMenu("TOTAL RACE DURATION:", (float)(cardX + 35), (float)(cardY + 66), 11.0f, UI_COLOR_MUTED_TEXT);
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
        UI_DrawTextMenu(TextFormat("CHECKPOINTS CLEARED:    %02d / %02d", race->totalCheckpoints, race->totalCheckpoints), (float)(cardX + 35), (float)(cardY + nextStatsY), 12.0f, UI_COLOR_STEEL_WHITE);
        UI_DrawTextMenu(TextFormat("VEL MAX:                %04d KM/H (MACH %.2f)", (int)race->maxSpeedKmh, race->maxMach), (float)(cardX + 35), (float)(cardY + nextStatsY + 20), 12.0f, UI_COLOR_STEEL_WHITE);
        UI_DrawTextMenu(TextFormat("MISSION SEED CODE:      %06u", race->runSeed), (float)(cardX + 35), (float)(cardY + nextStatsY + 40), 12.0f, UI_COLOR_AC4_CYAN);

        const char *rank = "PILOT EVALUATION:  RANK S [ACE PILOT]";
        Color rankCol = UI_COLOR_AC4_GREEN;
        if (race->finishTotalTime > 120.0f) { rank = "PILOT EVALUATION:  RANK B [QUALIFIED PILOT]"; rankCol = UI_COLOR_AC4_AMBER; }
        else if (race->finishTotalTime > 100.0f) { rank = "PILOT EVALUATION:  RANK A [VETERAN PILOT]"; rankCol = UI_COLOR_AC4_CYAN; }

        UI_DrawTextTitle(rank, (float)(cardX + 35), (float)(cardY + nextStatsY + 62), 14.0f, rankCol);
        DrawLine(cardX + 35, cardY + cardH - 50, cardX + cardW - 35, cardY + cardH - 50, (Color){ 35, 70, 95, 140 });

        // Caja de entrada de iniciales si clasificó en el Top 5
        if (!race->nameEntered && race->qualifyingRank >= 0) {
            int boxW = cardW - 70;
            int boxH = 106;
            int boxX = cardX + 35;
            int boxY = cardY + cardH - 124;
            Rectangle boxRec = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };

            DrawRectangleRounded(boxRec, 0.15f, 4, (Color){ 4, 12, 22, 245 });
            DrawRectangleRoundedLinesEx(boxRec, 0.15f, 4, 1.2f, UI_COLOR_AC4_AMBER);

            const char *topStr = TextFormat("HALL OF FAME ENTRY // POSITION #%02d", race->qualifyingRank + 1);
            UI_DrawTextHud(topStr, (float)(boxX + 16), (float)(boxY + 14), 11.0f, UI_COLOR_AC4_AMBER);
            UI_DrawTextHud("ENTER 3-LETTER PILOT TAG:", (float)(boxX + 16), (float)(boxY + 32), 10.0f, UI_COLOR_STEEL_WHITE);

            int charStartX = boxX + boxW - 145;
            int charY = boxY + 16;
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

            UI_DrawTextMenu("[W/S / UP/DN]: LETTER    [ENTER / (A)]: CONFIRM    [BKSP]: DELETE    [A-Z]: TYPE",
                            (float)(boxX + 16), (float)(boxY + boxH - 20), 9.5f, (Color){ 125, 170, 200, 220 });
        } else {
            UI_DrawTextMenu("[ ESC ]: RETURN TO MENU      [ R / SPACE / (A) ]: RESTART", (float)(cardX + 35), (float)(cardY + cardH - 34), 12.0f, (Color){ 110, 155, 185, 200 });
        }
    }
}

void Race_Unload(RaceTrack *race) {
    if (race->billboardShader.id != 0) {
        UnloadShader(race->billboardShader);
        race->billboardShader.id = 0;
    }
    if (race->isTexturesLoaded) {
        if (race->texPylon.id != 0) UnloadTexture(race->texPylon);
        if (race->texRing.id != 0)  UnloadTexture(race->texRing);
        race->isTexturesLoaded = false;
    }
}

