#include "records.h"
#include <stdio.h>
#include <string.h>

static SaveData s_saveData = { 0 };
static bool s_lastRunWasNewRecord = false;

void Records_Init(void) {
    memset(&s_saveData, 0, sizeof(SaveData));
    memcpy(s_saveData.magic, RECORDS_MAGIC, 4);
    s_saveData.version = RECORDS_VERSION;
    s_lastRunWasNewRecord = false;

    FILE *f = fopen(RECORDS_FILE_PATH, "rb");
    if (f) {
        SaveData loaded;
        size_t readCount = fread(&loaded, sizeof(SaveData), 1, f);
        fclose(f);

        if (readCount == 1 && memcmp(loaded.magic, RECORDS_MAGIC, 4) == 0 && loaded.version == RECORDS_VERSION) {
            s_saveData = loaded;
            return;
        }
    }

    // Inicialización por defecto si no existe archivo previo
    for (int i = 0; i < BIOME_COUNT; i++) {
        s_saveData.records[i].hasRecord = false;
        s_saveData.records[i].bestTime = 0.0f;
        s_saveData.records[i].maxSpeedKmh = 0.0f;
        snprintf(s_saveData.records[i].rank, sizeof(s_saveData.records[i].rank), "---");
    }
    s_saveData.totalSortiesCompleted = 0;
    Records_Save();
}

void Records_Save(void) {
    FILE *f = fopen(RECORDS_FILE_PATH, "wb");
    if (f) {
        fwrite(&s_saveData, sizeof(SaveData), 1, f);
        fclose(f);
    }
}

const CircuitRecord* Records_Get(BiomeType biome) {
    if (biome < 0 || biome >= BIOME_COUNT) {
        return &s_saveData.records[0];
    }
    return &s_saveData.records[biome];
}

bool Records_Submit(BiomeType biome, float time, float maxSpeedKmh, const char *rank) {
    s_lastRunWasNewRecord = false;
    if (biome < 0 || biome >= BIOME_COUNT || time <= 0.0f) return false;

    s_saveData.totalSortiesCompleted++;
    CircuitRecord *rec = &s_saveData.records[biome];

    bool isNew = (!rec->hasRecord || time < rec->bestTime);
    if (isNew) {
        rec->hasRecord = true;
        rec->bestTime = time;
        if (maxSpeedKmh > rec->maxSpeedKmh) {
            rec->maxSpeedKmh = maxSpeedKmh;
        }
        if (rank) {
            strncpy(rec->rank, rank, sizeof(rec->rank) - 1);
            rec->rank[sizeof(rec->rank) - 1] = '\0';
        }
        s_lastRunWasNewRecord = true;
    }

    Records_Save();
    return isNew;
}

bool Records_IsNewRecord(void) {
    return s_lastRunWasNewRecord;
}

void Records_ResetNewRecordFlag(void) {
    s_lastRunWasNewRecord = false;
}
