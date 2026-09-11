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

// N.A.D.I.A. Copiloto Onboard (Réseau-Orbital Avionics)
typedef enum NadiaCallout {
    NADIA_CALLOUT_NONE = 0,
    NADIA_CALLOUT_SURF_FLAT,
    NADIA_CALLOUT_SURF_RIGHT_LONG,
    NADIA_CALLOUT_SURF_RIGHT_HARD,
    NADIA_CALLOUT_SURF_LEFT_LONG,
    NADIA_CALLOUT_SURF_LEFT_HARD,
    NADIA_CALLOUT_SURF_DIVE,
    NADIA_CALLOUT_AIR_CLIMB,
    NADIA_CALLOUT_AIR_LEFT,
    NADIA_CALLOUT_AIR_RIGHT,
    NADIA_CALLOUT_FINAL_GATE,
    NADIA_CALLOUT_PERFECT_GATE,
    NADIA_CALLOUT_WARN_KE
} NadiaCallout;

void Audio_PlayNadia(NadiaCallout callout);
void Audio_ClearNadiaQueue(void);
int Audio_GetNadiaQueueCount(void);
void Audio_SetVoiceVolume(float volume);
float Audio_GetVoiceVolume(void);
bool Audio_IsNadiaPlaying(void);
void Audio_SetNadiaEnabled(bool enabled);
bool Audio_IsNadiaEnabled(void);
void Audio_StopAll(void);

// Efectos de Sonido Tácticos
void Audio_PlayCheckpoint(void);
void Audio_PlayPerfectGate(void);
void Audio_PlaySonicBoom(void);
void Audio_PlayWarning(void);
void Audio_PlayWallImpact(void);
void Audio_PlayCRTPowerOn(void);

void Audio_SetMasterVolume(float volume);
void Audio_Unload(void);

#endif // AUDIO_H
