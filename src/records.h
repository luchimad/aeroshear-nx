#ifndef RECORDS_H
#define RECORDS_H

#include "biome.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // LOCAL SAVE & CIRCUIT LEADERBOARD (HALL OF FAME TOP 5)
// ============================================================================

#define RECORDS_FILE_PATH       "records.dat"
#define RECORDS_MAGIC           "ASNX"
#define RECORDS_VERSION         3

#define TOP_SCORES_COUNT        5
#define PILOT_TAG_LEN           4   // 3 letras + null terminator (ej. "LUC", "ACE")

typedef struct HighscoreEntry {
    char pilotTag[PILOT_TAG_LEN];   // Iniciales del piloto (3 caracteres)
    float finishTime;               // Tiempo total de carrera en segundos
    float maxSpeedKmh;              // Velocidad máxima registrada (km/h)
    char rank[32];                  // Rango alcanzado (ej. "RANK S [ACE PILOT]")
    unsigned int seed;              // Seed procedural de la carrera
    bool isValid;                   // Entrada válida o vacía
} HighscoreEntry;

typedef struct CircuitLeaderboard {
    HighscoreEntry entries[TOP_SCORES_COUNT];
} CircuitLeaderboard;

typedef struct SaveData {
    char magic[4];
    int version;
    CircuitLeaderboard boards[BIOME_COUNT];
    int totalSortiesCompleted;
} SaveData;

void Records_Init(void);
void Records_Save(void);
const CircuitLeaderboard* Records_GetLeaderboard(BiomeType biome);
const HighscoreEntry* Records_GetBest(BiomeType biome);

// Comprueba si un tiempo califica para el Top 5. Retorna posición 0..4, o -1 si no califica.
int Records_CheckQualify(BiomeType biome, float time);

// Inserta un puntaje en la posición obtenida, desplazando los puestos inferiores y guardando a disco.
bool Records_InsertScore(BiomeType biome, int rankPos, const char *pilotTag, float time, float maxSpeedKmh, const char *rank, unsigned int seed);

bool Records_IsNewRecord(void);
int Records_GetLastQualifyingRank(void);
void Records_ResetNewRecordFlag(void);

#endif // RECORDS_H
