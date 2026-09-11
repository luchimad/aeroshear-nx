#ifndef RACE_H
#define RACE_H

#include "raylib.h"
#include "biome.h"
#include "player.h"
#include <stdbool.h>

#define MAX_RACE_CHECKPOINTS 24

typedef enum CheckpointType {
    CHECKPOINT_GROUND_PYLON  = 0,
    CHECKPOINT_AIR_RING      = 1,
    CHECKPOINT_FINISH_LINE   = 2
} CheckpointType;

typedef struct RaceCheckpoint {
    int index;
    CheckpointType type;
    Vector3 position;
    Vector3 direction;
    Vector3 rightVec;
    float width;
    float height;
    float targetSpeed;
    bool isPassed;
    float splitTime;
    float parTime;
} RaceCheckpoint;

typedef struct RaceTrack {
    RaceCheckpoint checkpoints[MAX_RACE_CHECKPOINTS];
    int totalCheckpoints;
    int currentCheckpoint;

    float raceTimer;
    float countdownTimer;
    bool isCountdown;
    bool isStarted;
    bool isFinished;
    float finishTotalTime;
    float bestSplitDelta;

    float feedbackTimer;
    float outOfBoundsTimer;
    float outOfBoundsGraceTimer;
    bool isOutOfBoundsWarning;
    bool cameraSnapRequested;
    int lastPassedGate;
    bool isMissedWarning;
    bool halfwayAnnounced;

    char trackName[64];
    Vector3 spawnPosition;
    BiomeType currentBiome;
    bool isNewRecord;
    int qualifyingRank;
    bool nameEntered;
    char pilotTag[4];
    int tagCursor;

    float maxSpeedKmh;
    float maxSpeedKnots;
    float maxMach;

    // Sistema Copiloto N.A.D.I.A. // Réseau-Orbital Avionics
    bool nadiaActive;
    float nadiaTimer;
    float nadiaMaxDuration;
    char nadiaTitle[64];
    char nadiaSubtitle[64];
    char nadiaDistance[32];
    float nadiaWarnCooldown;

    Texture2D texPylon;
    Texture2D texRing;
    Shader billboardShader;
    bool isTexturesLoaded;
    unsigned int runSeed;
} RaceTrack;

void Race_Init(RaceTrack *race, BiomeType biome, unsigned int seed, Vector3 *startPlayerPos, float *startYaw);
void Race_Update(RaceTrack *race, PlayerJet *player, float dt);
void Race_Draw3D(const RaceTrack *race, const Camera3D *camera);
void Race_DrawHUD(const RaceTrack *race, const PlayerJet *player, const Camera3D *camera, int screenWidth, int screenHeight);
void Race_Unload(RaceTrack *race);

void Nadia_TriggerGateCallout(RaceTrack *race, const PlayerJet *player, int nextIdx);
void Nadia_TriggerStallWarning(RaceTrack *race);

#endif // RACE_H
