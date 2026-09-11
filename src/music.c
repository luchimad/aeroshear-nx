#include "music.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DYNAMIC_TRACKS 64

typedef struct TrackDef {
    char path[256];
    char artist[64];
    char title[64];
} TrackDef;

typedef struct BiomePlaylist {
    TrackDef tracks[MAX_DYNAMIC_TRACKS];
    int count;
    int currentIndex;
} BiomePlaylist;

static BiomePlaylist s_menuPlaylist = { 0 };
static BiomePlaylist s_dunesPlaylist = { 0 };
static BiomePlaylist s_straitsPlaylist = { 0 };

static TrackDef s_bootEditTrack = { 0 };
static bool s_hasBootEditTrack = false;
static bool s_hasBootPlayed = false;

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
static char s_currentTitle[64] = "";
static char s_currentArtist[64] = "";

static bool IsSupportedAudio(const char *path) {
    if (!path) return false;
    return (IsFileExtension(path, ".mp3") ||
            IsFileExtension(path, ".wav") ||
            IsFileExtension(path, ".ogg") ||
            IsFileExtension(path, ".flac") ||
            IsFileExtension(path, ".xm")  ||
            IsFileExtension(path, ".mod"));
}

static void ScanDirectoryForTracks(BiomePlaylist *playlist, const char *primaryDir, const char *fallbackDir) {
    playlist->count = 0;
    playlist->currentIndex = 0;

    const char *dirToScan = NULL;
    if (DirectoryExists(primaryDir)) {
        dirToScan = primaryDir;
    } else if (fallbackDir && DirectoryExists(fallbackDir)) {
        dirToScan = fallbackDir;
    }

    if (!dirToScan) return;

    FilePathList files = LoadDirectoryFiles(dirToScan);
    for (unsigned int i = 0; i < files.count && playlist->count < MAX_DYNAMIC_TRACKS; i++) {
        const char *p = files.paths[i];
        if (IsSupportedAudio(p)) {
            // El tema Boot Edit se reserva EXCLUSIVAMENTE para el arranque inicial del juego
            if (strstr(p, "BOOT EDIT") != NULL) {
                strncpy(s_bootEditTrack.path, p, sizeof(s_bootEditTrack.path) - 1);
                s_bootEditTrack.path[sizeof(s_bootEditTrack.path) - 1] = '\0';
                strncpy(s_bootEditTrack.artist, "rePronto", sizeof(s_bootEditTrack.artist) - 1);
                strncpy(s_bootEditTrack.title, "Shear (BOOT EDIT)", sizeof(s_bootEditTrack.title) - 1);
                s_hasBootEditTrack = true;
                continue; // NUNCA incluirlo en la rotación normal del menú
            }

            TrackDef *t = &playlist->tracks[playlist->count];
            strncpy(t->path, p, sizeof(t->path) - 1);
            t->path[sizeof(t->path) - 1] = '\0';

            const char *baseName = GetFileNameWithoutExt(p);
            const char *separator = strstr(baseName, " - ");
            if (!separator) separator = strstr(baseName, " – ");

            if (separator) {
                int artistLen = (int)(separator - baseName);
                if (artistLen > (int)sizeof(t->artist) - 1) artistLen = (int)sizeof(t->artist) - 1;
                strncpy(t->artist, baseName, artistLen);
                t->artist[artistLen] = '\0';

                const char *titlePart = separator + 3;
                strncpy(t->title, titlePart, sizeof(t->title) - 1);
                t->title[sizeof(t->title) - 1] = '\0';
            } else {
                strncpy(t->artist, "UNKNOWN", sizeof(t->artist) - 1);
                t->artist[sizeof(t->artist) - 1] = '\0';
                strncpy(t->title, baseName, sizeof(t->title) - 1);
                t->title[sizeof(t->title) - 1] = '\0';
            }
            playlist->count++;
        }
    }
    UnloadDirectoryFiles(files);
}

void Music_ScanFolders(void) {
    ScanDirectoryForTracks(&s_dunesPlaylist, "assets/Audio/Music/Dunes", "assets/Audio/Music/Hotsands");
    ScanDirectoryForTracks(&s_straitsPlaylist, "assets/Audio/Music/Straits", "assets/Audio/Music/Deltastraits");
    ScanDirectoryForTracks(&s_menuPlaylist, "assets/Audio/Music/Menu", "assets/Audio/Music");

    // Fallbacks de seguridad si las carpetas están vacías pero existen temas en la raíz
    if (!s_hasBootEditTrack) {
        const char *bDef1 = "assets/Audio/Music/Menu/Shear (BOOT EDIT).mp3";
        const char *bDef2 = "assets/Audio/Music/Shear (BOOT EDIT).mp3";
        const char *bChosen = FileExists(bDef1) ? bDef1 : (FileExists(bDef2) ? bDef2 : NULL);
        if (bChosen) {
            strncpy(s_bootEditTrack.path, bChosen, sizeof(s_bootEditTrack.path) - 1);
            s_bootEditTrack.path[sizeof(s_bootEditTrack.path) - 1] = '\0';
            strcpy(s_bootEditTrack.artist, "rePronto");
            strcpy(s_bootEditTrack.title, "Shear (BOOT EDIT)");
            s_hasBootEditTrack = true;
        }
    }

    if (s_menuPlaylist.count == 0) {
        const char *mDef1 = "assets/Audio/Music/Menu/rePronto - Shear (fluoresense Edit) [Main Menu].mp3";
        const char *mDef2 = "assets/Audio/Music/rePronto - Shear (fluoresense Edit) [Main Menu].mp3";
        const char *mChosen = FileExists(mDef1) ? mDef1 : (FileExists(mDef2) ? mDef2 : NULL);
        if (mChosen) {
            strncpy(s_menuPlaylist.tracks[0].path, mChosen, sizeof(s_menuPlaylist.tracks[0].path) - 1);
            s_menuPlaylist.tracks[0].path[sizeof(s_menuPlaylist.tracks[0].path) - 1] = '\0';
            strcpy(s_menuPlaylist.tracks[0].artist, "rePronto");
            strcpy(s_menuPlaylist.tracks[0].title, "Shear (fluoresense Edit)");
            s_menuPlaylist.count = 1;
        }
    }
    if (s_dunesPlaylist.count == 0) {
        const char *dDef = "assets/Audio/Music/PHaze - Aftersin.mp3";
        if (FileExists(dDef)) {
            strncpy(s_dunesPlaylist.tracks[0].path, dDef, sizeof(s_dunesPlaylist.tracks[0].path) - 1);
            strcpy(s_dunesPlaylist.tracks[0].artist, "PHaze");
            strcpy(s_dunesPlaylist.tracks[0].title, "Aftersin");
            s_dunesPlaylist.count = 1;
        }
    }
    if (s_straitsPlaylist.count == 0) {
        const char *sDef = "assets/Audio/Music/am33go - bounceCraft.wav";
        if (FileExists(sDef)) {
            strncpy(s_straitsPlaylist.tracks[0].path, sDef, sizeof(s_straitsPlaylist.tracks[0].path) - 1);
            strcpy(s_straitsPlaylist.tracks[0].artist, "am33go");
            strcpy(s_straitsPlaylist.tracks[0].title, "bounceCraft");
            s_straitsPlaylist.count = 1;
        }
    }

    TraceLog(LOG_INFO, "[MUSIC] Scanned playlists: Dunes=%d, Straits=%d, Menu=%d",
             s_dunesPlaylist.count, s_straitsPlaylist.count, s_menuPlaylist.count);
}

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
    s_currentTitle[0] = '\0';
    s_currentArtist[0] = '\0';

    Music_ScanFolders();
}

void Music_PlayBootMenu(void) {
    if (s_hasBootPlayed) {
        Music_PlayMenu();
        return;
    }
    s_hasBootPlayed = true;
    s_currentState = MUSIC_STATE_MENU;

    const char *path = s_hasBootEditTrack ? s_bootEditTrack.path : "assets/Audio/Music/Menu/Shear (BOOT EDIT).mp3";
    const char *artist = s_hasBootEditTrack ? s_bootEditTrack.artist : "rePronto";
    const char *title = s_hasBootEditTrack ? s_bootEditTrack.title : "Shear (BOOT EDIT)";

    LoadAndPlay(path, artist, title, true);
    // Inicio inmediato de audio sin latencia de fade para sincronía perfecta con el destello de encendido del CRT
    s_currentFadeVol = 1.0f;
    if (s_isStreamLoaded) {
        SetMusicVolume(s_musicStream, s_masterMusicVol);
    }
}

void Music_PlayMenu(void) {
    // Si ya está en menú y reproduciendo una pista regular (que no sea el Boot Edit), mantenerla
    if (s_currentState == MUSIC_STATE_MENU && s_isStreamLoaded) {
        if (strstr(s_currentTitle, "BOOT EDIT") == NULL) {
            return;
        }
    }

    s_currentState = MUSIC_STATE_MENU;
    if (s_menuPlaylist.count > 0) {
        const TrackDef *track = &s_menuPlaylist.tracks[s_menuPlaylist.currentIndex % s_menuPlaylist.count];
        LoadAndPlay(track->path, track->artist, track->title, s_menuPlaylist.count <= 1);
    }
}

void Music_StartBiome(BiomeType biome) {
    s_currentState = MUSIC_STATE_GAMEPLAY;
    s_currentBiome = biome;

    BiomePlaylist *pl = (biome == BIOME_HOTSANDS) ? &s_dunesPlaylist : &s_straitsPlaylist;
    if (pl->count > 0) {
        pl->currentIndex = 0;
        const TrackDef *track = &pl->tracks[0];
        LoadAndPlay(track->path, track->artist, track->title, pl->count <= 1);
    } else {
        Music_Stop();
        s_currentState = MUSIC_STATE_GAMEPLAY;
    }
}

void Music_NextTrack(void) {
    if (s_currentState == MUSIC_STATE_GAMEPLAY) {
        BiomePlaylist *pl = (s_currentBiome == BIOME_HOTSANDS) ? &s_dunesPlaylist : &s_straitsPlaylist;
        if (pl->count > 0) {
            pl->currentIndex = (pl->currentIndex + 1) % pl->count;
            const TrackDef *track = &pl->tracks[pl->currentIndex];
            LoadAndPlay(track->path, track->artist, track->title, pl->count <= 1);
        }
    } else if (s_currentState == MUSIC_STATE_MENU) {
        if (s_menuPlaylist.count > 0) {
            s_menuPlaylist.currentIndex = (s_menuPlaylist.currentIndex + 1) % s_menuPlaylist.count;
            const TrackDef *track = &s_menuPlaylist.tracks[s_menuPlaylist.currentIndex];
            LoadAndPlay(track->path, track->artist, track->title, s_menuPlaylist.count <= 1);
        }
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

    // Si la pista no está en loop infinito y llega al final, avanzar automáticamente al siguiente tema del escenario
    if (!s_musicStream.looping) {
        float played = GetMusicTimePlayed(s_musicStream);
        float total = GetMusicTimeLength(s_musicStream);
        if (total > 1.0f && played >= (total - 0.25f)) {
            Music_NextTrack();
            return;
        }
    }

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
