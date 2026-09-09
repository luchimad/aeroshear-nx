#include "aircraft.h"
#include "config.h"
#include <string.h>

// Catálogo Modular de Aeronaves - Alpha Edition: AEROSHEAR RS-01 PHANTOM
static AircraftDefinition s_aircraftDatabase[MAX_AIRCRAFT] = {
    {
        .id = "rs01",
        .name = "AEROSHEAR RS-01 PHANTOM",
        .role = "HIGH-G TRANSONIC INTERCEPTOR",
        .manufacturer = "AEROSHEAR AEROSPACE / SKUNKWORKS",
        .description = "Air superiority and high-G racing craft. Optimized for rapid pitch-dive transitions and ground-shear kinetic retention.",
        .cruiseSpeed = SPEED_CRUISE,
        .afterburnerSpeed = SPEED_AFTERBURNER,
        .turnRate = 1.75f,
        .pitchRate = 1.65f,
        .rollRate = 3.60f,
        .acceleration = ACCEL_AFTERBURNER,
        .climbRate = 277.5f,
        .kineticCapacity = 300.0f,
        .kineticEfficiency = 1.15f,
        .hoverHeight = 12.5f,
        .climbCeiling = 225.0f,
        .statSpeed = 9,
        .statMobility = 9,
        .statStealth = 8,
        .statArmor = 7,
        .spritePath = SPRITE_PATH_RACESHIP_1,
        .spriteCols = 7,
        .spriteRows = 5,
        .baseScale = SPRITE_BASE_SCALE,
        .isUnlocked = true
    }
};

static int s_aircraftCount = 1;
static SpriteSheet s_aircraftSprites[MAX_AIRCRAFT] = { 0 };

void Aircraft_Init(void) {
    for (int i = 0; i < s_aircraftCount; i++) {
        const AircraftDefinition *def = &s_aircraftDatabase[i];
        SpriteSheet *sheet = &s_aircraftSprites[i];

        sheet->texture = (Texture2D){ 0 };
        sheet->cols = (def->spriteCols > 0) ? def->spriteCols : 7;
        sheet->rows = (def->spriteRows > 0) ? def->spriteRows : 5;

        if (def->spritePath && FileExists(def->spritePath)) {
            sheet->texture = LoadTexture(def->spritePath);
            if (sheet->texture.id > 0) {
                sheet->frameWidth = (float)sheet->texture.width / (float)sheet->cols;
                sheet->frameHeight = (float)sheet->texture.height / (float)sheet->rows;
                sheet->isLoaded = true;
                SetTextureFilter(sheet->texture, TEXTURE_FILTER_POINT);
            }
        }
    }
}

void Aircraft_UnloadAll(void) {
    for (int i = 0; i < s_aircraftCount; i++) {
        if (s_aircraftSprites[i].isLoaded) {
            UnloadTexture(s_aircraftSprites[i].texture);
            s_aircraftSprites[i].isLoaded = false;
        }
    }
}

int Aircraft_GetCount(void) {
    return s_aircraftCount;
}

const AircraftDefinition* Aircraft_Get(int index) {
    if (index < 0 || index >= s_aircraftCount) return &s_aircraftDatabase[0];
    return &s_aircraftDatabase[index];
}

const SpriteSheet* Aircraft_GetSprite(int index) {
    if (index < 0 || index >= s_aircraftCount) return &s_aircraftSprites[0];
    return &s_aircraftSprites[index];
}

const AircraftDefinition* Aircraft_GetById(const char *id) {
    if (!id) return &s_aircraftDatabase[0];
    for (int i = 0; i < s_aircraftCount; i++) {
        if (strcmp(s_aircraftDatabase[i].id, id) == 0) {
            return &s_aircraftDatabase[i];
        }
    }
    return &s_aircraftDatabase[0];
}

int Aircraft_Register(const AircraftDefinition *def) {
    if (!def || s_aircraftCount >= MAX_AIRCRAFT) return -1;
    s_aircraftDatabase[s_aircraftCount] = *def;
    int idx = s_aircraftCount++;
    
    SpriteSheet *sheet = &s_aircraftSprites[idx];
    sheet->texture = (Texture2D){ 0 };
    sheet->cols = (def->spriteCols > 0) ? def->spriteCols : 7;
    sheet->rows = (def->spriteRows > 0) ? def->spriteRows : 5;

    if (def->spritePath && FileExists(def->spritePath)) {
        sheet->texture = LoadTexture(def->spritePath);
        if (sheet->texture.id > 0) {
            sheet->frameWidth = (float)sheet->texture.width / (float)sheet->cols;
            sheet->frameHeight = (float)sheet->texture.height / (float)sheet->rows;
            sheet->isLoaded = true;
            SetTextureFilter(sheet->texture, TEXTURE_FILTER_POINT);
        }
    }
    return idx;
}
