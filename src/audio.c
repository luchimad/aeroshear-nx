#include "audio.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

// Bucles continuos de turbina
#define PATH_TURBINE_IDLE   "assets/Audio/Fx/Turbine_idle_loop.wav"
#define PATH_TURBINE_MAX    "assets/Audio/Fx/Turbine_max_loop.wav"

// Voces del locutor
#define PATH_VOICE_THREE    "assets/Audio/Announcer/Voice_three.mp3"
#define PATH_VOICE_TWO      "assets/Audio/Announcer/Voice_two.mp3"
#define PATH_VOICE_ONE      "assets/Audio/Announcer/Voice_one.mp3"
#define PATH_VOICE_GO       "assets/Audio/Announcer/Voice_go.mp3"
#define PATH_VOICE_HALFWAY  "assets/Audio/Announcer/Voice_halfway.mp3"

static Music musTurbineIdle = { 0 };
static Music musTurbineMax  = { 0 };
static bool isTurbineIdleLoaded = false;
static bool isTurbineMaxLoaded  = false;

static Sound sndVoiceThree   = { 0 };
static Sound sndVoiceTwo     = { 0 };
static Sound sndVoiceOne     = { 0 };
static Sound sndVoiceGo      = { 0 };
static Sound sndVoiceHalfway = { 0 };

static float currentIdleVol = 0.0f;
static float currentMaxVol  = 0.0f;
static float currentPitch   = 1.0f;

// Efectos sintetizados
static Sound sndCheckpoint  = { 0 };
static Sound sndPerfectGate = { 0 };
static Sound sndSonicBoom   = { 0 };
static Sound sndWarning     = { 0 };
static Sound sndWindRush    = { 0 };
static Sound sndWallImpact  = { 0 };

static float currentWindVol = 0.0f;
static float currentWindPitch = 1.0f;

static bool isAudioReady = false;
static float masterVol = 1.0f;

#define SAMPLE_RATE 44100

static Sound SynthesizeSound(short *samples, int sampleCount) {
    Wave wave = { 0 };
    wave.frameCount = (unsigned int)sampleCount;
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;

    Sound snd = LoadSoundFromWave(wave);
    MemFree(samples);
    return snd;
}

static Sound GenerateWindRushSound(void) {
    int count = (int)(SAMPLE_RATE * 3.2f);
    short *data = (short *)MemAlloc(count * sizeof(short));
    if (!data) return (Sound){ 0 };

    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    for (int i = 0; i < count; i++) {
        float white = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;

        float t = (float)i / (float)count;
        float env = 1.0f;
        if (t < 0.05f) env = t / 0.05f;
        if (t > 0.95f) env = (1.0f - t) / 0.05f;

        float val = pink * env * 0.16f;
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        data[i] = (short)(val * 32767.0f);
    }
    return SynthesizeSound(data, count);
}

static Sound GenerateCheckpointSound(void) {
    int count = (int)(SAMPLE_RATE * 0.28f);
    short *data = (short *)MemAlloc(count * sizeof(short));
    if (!data) return (Sound){ 0 };

    for (int i = 0; i < count; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float freq = (t < 0.12f) ? 1046.50f : 1567.98f;
        float env = expf(-t * 9.0f);

        float tone = sinf(2.0f * PI * freq * t) + sinf(2.0f * PI * freq * 2.0f * t) * 0.3f;
        float val = tone * env * 0.45f;
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        data[i] = (short)(val * 32767.0f);
    }
    return SynthesizeSound(data, count);
}

static Sound GeneratePerfectGateSound(void) {
    int count = (int)(SAMPLE_RATE * 0.45f);
    short *data = (short *)MemAlloc(count * sizeof(short));
    if (!data) return (Sound){ 0 };

    for (int i = 0; i < count; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float freq = 1318.51f;
        if (t > 0.10f) freq = 1760.00f;
        if (t > 0.22f) freq = 2637.02f;
        float env = expf(-t * 6.5f);

        float tone = sinf(2.0f * PI * freq * t) + sinf(2.0f * PI * freq * 2.0f * t) * 0.4f;
        float val = tone * env * 0.50f;
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        data[i] = (short)(val * 32767.0f);
    }
    return SynthesizeSound(data, count);
}

static Sound GenerateSonicBoom(void) {
    int count = (int)(SAMPLE_RATE * 0.70f);
    short *data = (short *)MemAlloc(count * sizeof(short));
    if (!data) return (Sound){ 0 };

    for (int i = 0; i < count; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float env = expf(-t * 3.5f);
        float rumble = sinf(2.0f * PI * 65.0f * t) * 0.6f + sinf(2.0f * PI * 35.0f * t) * 0.4f;
        float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.35f;
        float val = (rumble + noise) * env * 0.75f;
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        data[i] = (short)(val * 32767.0f);
    }
    return SynthesizeSound(data, count);
}

static Sound GenerateWarningSound(void) {
    int count = (int)(SAMPLE_RATE * 0.15f);
    short *data = (short *)MemAlloc(count * sizeof(short));
    if (!data) return (Sound){ 0 };

    for (int i = 0; i < count; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float freq = (t < 0.075f) ? 800.0f : 600.0f;
        float tone = (sinf(2.0f * PI * freq * t) > 0.0f) ? 0.35f : -0.35f;
        data[i] = (short)(tone * 32767.0f);
    }
    return SynthesizeSound(data, count);
}

static Sound GenerateWallImpactSound(void) {
    int count = (int)(SAMPLE_RATE * 0.35f);
    short *data = (short *)MemAlloc(count * sizeof(short));
    if (!data) return (Sound){ 0 };

    for (int i = 0; i < count; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float progress = t / 0.35f;
        float env = expf(-progress * 6.5f);

        float thudFreq = Lerp(110.0f, 45.0f, progress);
        float thud = sinf(2.0f * PI * thudFreq * t);
        thud = Clamp(thud * 1.8f, -1.0f, 1.0f) * 0.55f;

        float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
        float scrapeRes = sinf(2.0f * PI * 1850.0f * t) * noise;
        float scrape = (noise * 0.4f + scrapeRes * 0.6f) * 0.45f;

        float val = (thud + scrape) * env;
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        data[i] = (short)(val * 32767.0f);
    }
    return SynthesizeSound(data, count);
}

void Audio_Init(void) {
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    if (!IsAudioDeviceReady()) return;

    musTurbineIdle = LoadMusicStream(PATH_TURBINE_IDLE);
    if (musTurbineIdle.ctxData != NULL) {
        musTurbineIdle.looping = true;
        PlayMusicStream(musTurbineIdle);
        SetMusicVolume(musTurbineIdle, 0.0f);
        isTurbineIdleLoaded = true;
    }

    musTurbineMax = LoadMusicStream(PATH_TURBINE_MAX);
    if (musTurbineMax.ctxData != NULL) {
        musTurbineMax.looping = true;
        PlayMusicStream(musTurbineMax);
        SetMusicVolume(musTurbineMax, 0.0f);
        isTurbineMaxLoaded = true;
    }

    // Carga de Voces Profesionales del Locutor
    if (FileExists(PATH_VOICE_THREE))   sndVoiceThree   = LoadSound(PATH_VOICE_THREE);
    if (FileExists(PATH_VOICE_TWO))     sndVoiceTwo     = LoadSound(PATH_VOICE_TWO);
    if (FileExists(PATH_VOICE_ONE))     sndVoiceOne     = LoadSound(PATH_VOICE_ONE);
    if (FileExists(PATH_VOICE_GO))      sndVoiceGo      = LoadSound(PATH_VOICE_GO);
    if (FileExists(PATH_VOICE_HALFWAY)) sndVoiceHalfway = LoadSound(PATH_VOICE_HALFWAY);

    sndCheckpoint    = GenerateCheckpointSound();
    sndPerfectGate   = GeneratePerfectGateSound();
    sndSonicBoom     = GenerateSonicBoom();
    sndWarning       = GenerateWarningSound();
    sndWindRush      = GenerateWindRushSound();
    sndWallImpact    = GenerateWallImpactSound();

    isAudioReady = true;
    currentIdleVol = 0.22f;
    currentMaxVol = 0.0f;
    currentPitch = 1.0f;
    currentWindVol = 0.0f;
    currentWindPitch = 1.0f;
}

void Audio_Update(float speedRatio, bool isAfterburner, bool isAirbrake, float altitudeAGL, float dt) {
    if (!isAudioReady) return;

    float targetIdleVol = 0.25f;
    float targetMaxVol  = 0.0f;
    float targetPitch   = 0.92f + speedRatio * 0.28f;

    if (isAirbrake) {
        targetIdleVol = 0.18f;
        targetMaxVol  = 0.0f;
        targetPitch   *= 0.78f;
    } else if (isAfterburner) {
        targetIdleVol = 0.07f;
        targetMaxVol  = 0.44f;
        targetPitch   *= 1.16f;
    } else {
        targetIdleVol = 0.21f + speedRatio * 0.09f;
        targetMaxVol  = (speedRatio > 0.88f) ? ((speedRatio - 0.88f) * 2.5f * 0.175f) : 0.0f;
    }

    currentIdleVol = Lerp(currentIdleVol, targetIdleVol, dt * 5.5f);
    currentMaxVol  = Lerp(currentMaxVol, targetMaxVol, dt * 5.5f);
    currentPitch   = Lerp(currentPitch, targetPitch, dt * 4.8f);

    if (isTurbineIdleLoaded) {
        UpdateMusicStream(musTurbineIdle);
        SetMusicVolume(musTurbineIdle, currentIdleVol * masterVol);
        SetMusicPitch(musTurbineIdle, currentPitch);
    }

    if (isTurbineMaxLoaded) {
        UpdateMusicStream(musTurbineMax);
        SetMusicVolume(musTurbineMax, currentMaxVol * masterVol);
        SetMusicPitch(musTurbineMax, currentPitch * 1.03f);
    }

    float groundProximity = 0.0f;
    if (altitudeAGL < 35.0f) {
        groundProximity = (35.0f - altitudeAGL) / 35.0f;
    }

    float targetWindVol = (speedRatio * 0.12f + groundProximity * 0.22f) * (isAirbrake ? 0.2f : 1.0f);
    float targetWindPitch = 0.85f + speedRatio * 0.35f + groundProximity * 0.25f;

    currentWindVol = Lerp(currentWindVol, targetWindVol, dt * 5.0f);
    currentWindPitch = Lerp(currentWindPitch, targetWindPitch, dt * 4.5f);

    if (currentWindVol > 0.005f) {
        if (!IsSoundPlaying(sndWindRush)) {
            PlaySound(sndWindRush);
        }
        SetSoundVolume(sndWindRush, currentWindVol * masterVol * 0.50f);
        SetSoundPitch(sndWindRush, currentWindPitch);
    } else {
        if (IsSoundPlaying(sndWindRush)) {
            StopSound(sndWindRush);
        }
    }
}

void Audio_PlayCountdownStage(int stage) {
    if (!isAudioReady) return;
    if (stage == 3 && IsSoundValid(sndVoiceThree)) PlaySound(sndVoiceThree);
    else if (stage == 2 && IsSoundValid(sndVoiceTwo)) PlaySound(sndVoiceTwo);
    else if (stage == 1 && IsSoundValid(sndVoiceOne)) PlaySound(sndVoiceOne);
    else if (stage == 0 && IsSoundValid(sndVoiceGo)) PlaySound(sndVoiceGo);
}

void Audio_PlayHalfway(void) {
    if (isAudioReady && IsSoundValid(sndVoiceHalfway)) {
        PlaySound(sndVoiceHalfway);
    }
}

void Audio_PlayCheckpoint(void) {
    if (isAudioReady) PlaySound(sndCheckpoint);
}

void Audio_PlayPerfectGate(void) {
    if (isAudioReady) PlaySound(sndPerfectGate);
}

void Audio_PlaySonicBoom(void) {
    if (isAudioReady) PlaySound(sndSonicBoom);
}

void Audio_PlayWarning(void) {
    if (isAudioReady && !IsSoundPlaying(sndWarning)) PlaySound(sndWarning);
}

void Audio_PlayWallImpact(void) {
    if (isAudioReady) {
        SetSoundPitch(sndWallImpact, 0.90f + ((float)rand() / (float)RAND_MAX) * 0.20f);
        PlaySound(sndWallImpact);
    }
}

void Audio_SetMasterVolume(float volume) {
    masterVol = Clamp(volume, 0.0f, 1.0f);
    SetMasterVolume(masterVol);
}

void Audio_Unload(void) {
    if (!isAudioReady) return;

    if (isTurbineIdleLoaded) {
        StopMusicStream(musTurbineIdle);
        UnloadMusicStream(musTurbineIdle);
        isTurbineIdleLoaded = false;
    }
    if (isTurbineMaxLoaded) {
        StopMusicStream(musTurbineMax);
        UnloadMusicStream(musTurbineMax);
        isTurbineMaxLoaded = false;
    }

    if (IsSoundValid(sndVoiceThree))   UnloadSound(sndVoiceThree);
    if (IsSoundValid(sndVoiceTwo))     UnloadSound(sndVoiceTwo);
    if (IsSoundValid(sndVoiceOne))     UnloadSound(sndVoiceOne);
    if (IsSoundValid(sndVoiceGo))      UnloadSound(sndVoiceGo);
    if (IsSoundValid(sndVoiceHalfway)) UnloadSound(sndVoiceHalfway);

    UnloadSound(sndCheckpoint);
    UnloadSound(sndPerfectGate);
    UnloadSound(sndSonicBoom);
    UnloadSound(sndWarning);
    UnloadSound(sndWindRush);
    UnloadSound(sndWallImpact);

    CloseAudioDevice();
    isAudioReady = false;
}
