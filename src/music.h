#ifndef MUSIC_H
#define MUSIC_H

#include "raylib.h"
#include "biome.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // FLIGHT CORE - ADAPTIVE BIOME MUSIC STREAMER
// ============================================================================

void Music_Init(void);
void Music_Update(float dt);
void Music_PlayMenu(void);
void Music_StartBiome(BiomeType biome);
void Music_NextTrack(void);
void Music_SetVolume(float volume);
float Music_GetVolume(void);
const char* Music_GetCurrentTitle(void);
const char* Music_GetCurrentArtist(void);
float Music_GetOsdTimer(void);
void Music_TriggerDucking(float duration);
void Music_Stop(void);
void Music_Unload(void);

#endif // MUSIC_H
