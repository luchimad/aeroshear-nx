#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // AUDIO & ANNOUNCER SUBSYSTEM (ALPHA EDITION)
// ============================================================================

void Audio_Init(void);
void Audio_Update(float speedRatio, bool isAfterburner, bool isAirbrake, float altitudeAGL, float dt);

// Locutor Profesional (Announcer Voices)
void Audio_PlayCountdownStage(int stage); // 3, 2, 1, 0 (GO)
void Audio_PlayHalfway(void);

// Efectos de Sonido Tácticos
void Audio_PlayCheckpoint(void);
void Audio_PlayPerfectGate(void);
void Audio_PlaySonicBoom(void);
void Audio_PlayWarning(void);
void Audio_PlayWallImpact(void);

void Audio_SetMasterVolume(float volume);
void Audio_Unload(void);

#endif // AUDIO_H
