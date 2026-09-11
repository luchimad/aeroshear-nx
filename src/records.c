#include "records.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static SaveData s_saveData = { 0 };
static bool s_isNewAbsoluteRecord = false;
static int s_lastQualifyingRank = -1;

static void InitDefaultLeaderboards(void) {
    memset(&s_saveData, 0, sizeof(SaveData));
    memcpy(s_saveData.magic, RECORDS_MAGIC, 4);
    s_saveData.version = RECORDS_VERSION;
    s_saveData.totalSortiesCompleted = 0;

    // Récords por defecto Delta Straits (Pistas fluviales)
    static const struct {
        const char *tag; float time; float spd; const char *rank; unsigned int seed;
    } defaultDelta[TOP_SCORES_COUNT] = {
        { "ACE", 88.45f,  1920.0f, "RANK S [ACE PILOT]",       101824u },
        { "TDR", 94.20f,  1840.0f, "RANK S [ACE PILOT]",       392104u },
        { "W03", 102.80f, 1760.0f, "RANK A [VETERAN PILOT]",   781932u },
        { "FEI", 109.15f, 1690.0f, "RANK A [VETERAN PILOT]",   451209u },
        { "AGS", 118.60f, 1620.0f, "RANK B [QUALIFIED PILOT]", 602381u }
    };

    // Récords por defecto Hot Sands (Desierto árido)
    static const struct {
        const char *tag; float time; float spd; const char *rank; unsigned int seed;
    } defaultHotsands[TOP_SCORES_COUNT] = {
        { "PHZ", 92.10f,  1950.0f, "RANK S [ACE PILOT]",       519820u },
        { "DNE", 98.50f,  1860.0f, "RANK S [ACE PILOT]",       841029u },
        { "SKW", 105.30f, 1790.0f, "RANK A [VETERAN PILOT]",   294103u },
        { "SOL", 112.00f, 1710.0f, "RANK A [VETERAN PILOT]",   673912u },
        { "RAW", 121.40f, 1630.0f, "RANK B [QUALIFIED PILOT]", 918234u }
    };

    for (int i = 0; i < TOP_SCORES_COUNT; i++) {
        HighscoreEntry *eD = &s_saveData.boards[BIOME_DELTASTRAITS].entries[i];
        snprintf(eD->pilotTag, sizeof(eD->pilotTag), "%s", defaultDelta[i].tag);
        eD->finishTime = defaultDelta[i].time;
        eD->maxSpeedKmh = defaultDelta[i].spd;
        snprintf(eD->rank, sizeof(eD->rank), "%s", defaultDelta[i].rank);
        eD->seed = defaultDelta[i].seed;
        eD->isValid = true;

        HighscoreEntry *eH = &s_saveData.boards[BIOME_HOTSANDS].entries[i];
        snprintf(eH->pilotTag, sizeof(eH->pilotTag), "%s", defaultHotsands[i].tag);
        eH->finishTime = defaultHotsands[i].time;
        eH->maxSpeedKmh = defaultHotsands[i].spd;
        snprintf(eH->rank, sizeof(eH->rank), "%s", defaultHotsands[i].rank);
        eH->seed = defaultHotsands[i].seed;
        eH->isValid = true;
    }
}

void Records_Init(void) {
    s_isNewAbsoluteRecord = false;
    s_lastQualifyingRank = -1;

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

    InitDefaultLeaderboards();
    Records_Save();
}

void Records_Save(void) {
    FILE *f = fopen(RECORDS_FILE_PATH, "wb");
    if (f) {
        fwrite(&s_saveData, sizeof(SaveData), 1, f);
        fclose(f);
    }
}

const CircuitLeaderboard* Records_GetLeaderboard(BiomeType biome) {
    if (biome < 0 || biome >= BIOME_COUNT) {
        return &s_saveData.boards[0];
    }
    return &s_saveData.boards[biome];
}

const HighscoreEntry* Records_GetBest(BiomeType biome) {
    if (biome < 0 || biome >= BIOME_COUNT) {
        return &s_saveData.boards[0].entries[0];
    }
    return &s_saveData.boards[biome].entries[0];
}

int Records_CheckQualify(BiomeType biome, float time) {
    if (biome < 0 || biome >= BIOME_COUNT || time <= 0.0f) return -1;

    const CircuitLeaderboard *board = &s_saveData.boards[biome];
    for (int i = 0; i < TOP_SCORES_COUNT; i++) {
        if (!board->entries[i].isValid || time < board->entries[i].finishTime) {
            return i;
        }
    }
    return -1;
}

bool Records_InsertScore(BiomeType biome, int rankPos, const char *pilotTag, float time, float maxSpeedKmh, const char *rank, unsigned int seed) {
    if (biome < 0 || biome >= BIOME_COUNT || rankPos < 0 || rankPos >= TOP_SCORES_COUNT) {
        return false;
    }

    s_saveData.totalSortiesCompleted++;
    CircuitLeaderboard *board = &s_saveData.boards[biome];

    // Desplazar entradas inferiores
    for (int j = TOP_SCORES_COUNT - 1; j > rankPos; j--) {
        board->entries[j] = board->entries[j - 1];
    }

    // Insertar nueva entrada
    HighscoreEntry *entry = &board->entries[rankPos];
    char cleanTag[PILOT_TAG_LEN] = "PIL";
    if (pilotTag && strlen(pilotTag) > 0) {
        for (int c = 0; c < 3 && pilotTag[c] != '\0'; c++) {
            cleanTag[c] = (char)toupper((unsigned char)pilotTag[c]);
        }
        cleanTag[3] = '\0';
    }
    snprintf(entry->pilotTag, sizeof(entry->pilotTag), "%s", cleanTag);

    entry->finishTime = time;
    entry->maxSpeedKmh = maxSpeedKmh;
    if (rank) {
        strncpy(entry->rank, rank, sizeof(entry->rank) - 1);
        entry->rank[sizeof(entry->rank) - 1] = '\0';
    }
    entry->seed = seed;
    entry->isValid = true;

    s_isNewAbsoluteRecord = (rankPos == 0);
    s_lastQualifyingRank = rankPos;

    Records_Save();
    return true;
}

bool Records_IsNewRecord(void) {
    return s_isNewAbsoluteRecord;
}

int Records_GetLastQualifyingRank(void) {
    return s_lastQualifyingRank;
}

void Records_ResetNewRecordFlag(void) {
    s_isNewAbsoluteRecord = false;
    s_lastQualifyingRank = -1;
}
