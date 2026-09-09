#ifndef RECORDS_H
#define RECORDS_H

#include "biome.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // LOCAL SAVE & CIRCUIT RECORDS SYSTEM
// ============================================================================

#define RECORDS_FILE_PATH   "records.dat"
#define RECORDS_MAGIC       "ASNX"
#define RECORDS_VERSION     1

typedef struct CircuitRecord {
    float bestTime;         // Mejor tiempo total en segundos
    float maxSpeedKmh;      // Velocidad máxima registrada (km/h)
    char rank[32];          // Rango de piloto (ej. "RANK S [ACE AVIATOR]")
    bool hasRecord;         // Indica si existe un récord registrado
} CircuitRecord;

typedef struct SaveData {
    char magic[4];
    int version;
    CircuitRecord records[BIOME_COUNT];
    int totalSortiesCompleted;
} SaveData;

void Records_Init(void);
void Records_Save(void);
const CircuitRecord* Records_Get(BiomeType biome);
bool Records_Submit(BiomeType biome, float time, float maxSpeedKmh, const char *rank);
bool Records_IsNewRecord(void);
void Records_ResetNewRecordFlag(void);

#endif // RECORDS_H
