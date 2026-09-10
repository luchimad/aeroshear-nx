#include "music.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TrackDef {
    const char *path;
    const char *artist;
    const char *title;
} TrackDef;

// Catálogo Oficial de Pistas
static const TrackDef s_menuTrack = {
    "assets/Audio/Music/rePronto - Shear (fluoresense Edit) [Main Menu].mp3",
    "rePronto",
    "Shear (fluoresense Edit)"
};

static const TrackDef s_hotsandsTracks[] = {
    { "assets/Audio/Music/PHaze - Aftersin.mp3", "PHaze", "Aftersin" },
    { "assets/Audio/Music/PHaze - Exposure Recall.mp3", "PHaze", "Exposure Recall" }
};
#define HOTSANDS_TRACK_COUNT (int)(sizeof(s_hotsandsTracks) / sizeof(s_hotsandsTracks[0]))

static const TrackDef s_deltastraitsTracks[] = {
    { "assets/Audio/Music/am33go - bounceCraft.wav", "am33go", "bounceCraft" },
    { "assets/Audio/Music/am33go - Yo! guanna.wav", "am33go", "Yo! guanna" }
};
#define DELTASTRAITS_TRACK_COUNT (int)(sizeof(s_deltastraitsTracks) / sizeof(s_deltastraitsTracks[0]))

typedef enum MusicState {
    MUSIC_STATE_IDLE = 0,
    MUSIC_STATE_MENU,
    MUSIC_STATE_GAMEPLAY
} MusicState;

static MusicState s_currentState = MUSIC_STATE_IDLE;
static Music s_musicStream = { 0 };
static bool s_isStreamLoaded = false;
static float s_masterMusicVol = 0.80f;
static float s_currentFadeVol = 0.0f;
static float s_targetFadeVol = 0.80f;
static float s_duckTimer = 0.0f;
static float s_osdTimer = 0.0f;

static BiomeType s_currentBiome = BIOME_DELTASTRAITS;
static int s_currentTrackIndex = 0;
static char s_currentTitle[64] = "";
static char s_currentArtist[64] = "";

static void LoadAndPlay(const char *path, const char *artist, const char *title, bool loop) {
    if (s_isStreamLoaded) {
        StopMusicStream(s_musicStream);
        UnloadMusicStream(s_musicStream);
        s_isStreamLoaded = false;
    }

    if (!path || !FileExists(path)) {
        s_currentTitle[0] = '\0';
        s_currentArtist[0] = '\0';
        return;
    }

    s_musicStream = LoadMusicStream(path);
    if (s_musicStream.ctxData != NULL) {
        s_musicStream.looping = loop;
        PlayMusicStream(s_musicStream);
        SetMusicVolume(s_musicStream, 0.0f);
        s_currentFadeVol = 0.0f;
        s_targetFadeVol = 1.0f;
        s_isStreamLoaded = true;
        s_osdTimer = 3.5f;

        strncpy(s_currentArtist, artist ? artist : "UNKNOWN", sizeof(s_currentArtist) - 1);
        s_currentArtist[sizeof(s_currentArtist) - 1] = '\0';
        strncpy(s_currentTitle, title ? title : "UNTITLED", sizeof(s_currentTitle) - 1);
        s_currentTitle[sizeof(s_currentTitle) - 1] = '\0';
    }
}

void Music_Init(void) {
    s_currentState = MUSIC_STATE_IDLE;
    s_isStreamLoaded = false;
    s_masterMusicVol = 0.80f;
    s_currentFadeVol = 0.0f;
    s_targetFadeVol = 0.80f;
    s_duckTimer = 0.0f;
    s_osdTimer = 0.0f;
    s_currentTrackIndex = 0;
    s_currentTitle[0] = '\0';
    s_currentArtist[0] = '\0';
}

void Music_PlayMenu(void) {
    if (s_currentState == MUSIC_STATE_MENU && s_isStreamLoaded) return;

    s_currentState = MUSIC_STATE_MENU;
    LoadAndPlay(s_menuTrack.path, s_menuTrack.artist, s_menuTrack.title, true);
}

void Music_StartBiome(BiomeType biome) {
    s_currentState = MUSIC_STATE_GAMEPLAY;
    s_currentBiome = biome;
    s_currentTrackIndex = 0;

    const TrackDef *track = NULL;
    if (biome == BIOME_HOTSANDS) {
        track = &s_hotsandsTracks[s_currentTrackIndex % HOTSANDS_TRACK_COUNT];
    } else {
        track = &s_deltastraitsTracks[s_currentTrackIndex % DELTASTRAITS_TRACK_COUNT];
    }

    if (track) {
        LoadAndPlay(track->path, track->artist, track->title, true);
    }
}

void Music_NextTrack(void) {
    if (s_currentState != MUSIC_STATE_GAMEPLAY) return;

    s_currentTrackIndex++;
    const TrackDef *track = NULL;
    if (s_currentBiome == BIOME_HOTSANDS) {
        track = &s_hotsandsTracks[s_currentTrackIndex % HOTSANDS_TRACK_COUNT];
    } else {
        track = &s_deltastraitsTracks[s_currentTrackIndex % DELTASTRAITS_TRACK_COUNT];
    }

    if (track) {
        LoadAndPlay(track->path, track->artist, track->title, true);
    }
}

void Music_SetVolume(float volume) {
    s_masterMusicVol = Clamp(volume, 0.0f, 1.0f);
}

float Music_GetVolume(void) {
    return s_masterMusicVol;
}

const char* Music_GetCurrentTitle(void) {
    return s_currentTitle;
}

const char* Music_GetCurrentArtist(void) {
    return s_currentArtist;
}

float Music_GetOsdTimer(void) {
    return s_osdTimer;
}

void Music_TriggerDucking(float duration) {
    if (duration > s_duckTimer) {
        s_duckTimer = duration;
    }
}

void Music_Stop(void) {
    if (s_isStreamLoaded) {
        StopMusicStream(s_musicStream);
        UnloadMusicStream(s_musicStream);
        s_isStreamLoaded = false;
    }
    s_currentState = MUSIC_STATE_IDLE;
    s_currentTitle[0] = '\0';
    s_currentArtist[0] = '\0';
    s_osdTimer = 0.0f;
}

void Music_Update(float dt) {
    if (s_osdTimer > 0.0f) {
        s_osdTimer -= dt;
        if (s_osdTimer < 0.0f) s_osdTimer = 0.0f;
    }

    if (s_duckTimer > 0.0f) {
        s_duckTimer -= dt;
        if (s_duckTimer < 0.0f) s_duckTimer = 0.0f;
    }

    if (s_currentState == MUSIC_STATE_GAMEPLAY) {
        // Atajo teclado [N] para saltar pista
        if (IsKeyPressed(KEY_N)) {
            Music_NextTrack();
            return;
        }
    }

    if (!s_isStreamLoaded) return;

    UpdateMusicStream(s_musicStream);

    // Suavizado dinámico de volumen con soporte de atenuación (ducking)
    float targetVol = s_targetFadeVol;
    if (s_duckTimer > 0.0f) {
        targetVol *= 0.35f; // Atenuación por voz del locutor
    }

    s_currentFadeVol = Lerp(s_currentFadeVol, targetVol, dt * 3.5f);
    float finalVol = s_currentFadeVol * s_masterMusicVol;
    SetMusicVolume(s_musicStream, finalVol);
}

void Music_Unload(void) {
    Music_Stop();
}
