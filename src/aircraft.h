#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "raylib.h"
#include "game_types.h"
#include <stdbool.h>

#define MAX_AIRCRAFT 8

typedef struct AircraftDefinition {
    const char *id;
    const char *name;
    const char *role;
    const char *manufacturer;
    const char *description;
    
    // Físicas
    float cruiseSpeed;
    float afterburnerSpeed;
    float turnRate;
    float pitchRate;
    float rollRate;
    float acceleration;
    float climbRate;

    // Efecto Suelo & KE
    float kineticCapacity;
    float kineticEfficiency;
    float hoverHeight;
    float climbCeiling;

    // Stats
    int statSpeed;
    int statMobility;
    int statStealth;
    int statArmor;

    // Spritesheet
    const char *spritePath;
    int spriteCols;
    int spriteRows;
    float baseScale;
    bool isUnlocked;
} AircraftDefinition;

void Aircraft_Init(void);
void Aircraft_UnloadAll(void);
int Aircraft_GetCount(void);
const AircraftDefinition* Aircraft_Get(int index);
const SpriteSheet* Aircraft_GetSprite(int index);
const AircraftDefinition* Aircraft_GetById(const char *id);
int Aircraft_Register(const AircraftDefinition *def);

#endif // AIRCRAFT_H
