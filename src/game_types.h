#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "raylib.h"
#include "config.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // FLIGHT CORE - GAME TYPES & STRUCTURES (ALPHA EDITION)
// ============================================================================

#include "ui_theme.h"

// --- Estados Globales del Juego ---
typedef enum GameState {
    GAME_STATE_MAIN_MENU       = 0,
    GAME_STATE_MAP_SELECT      = 1,
    GAME_STATE_AIRCRAFT_SELECT = 2,
    GAME_STATE_PLAYING         = 3,
    GAME_STATE_PAUSED          = 4,
    GAME_STATE_SETTINGS        = 5,
    GAME_STATE_CONTROLS        = 6
} GameState;

typedef enum GameMode {
    MODE_AIR_RACE    = 0,       // Carrera de 18 checkpoints con cronómetro
    MODE_FREE_FLIGHT = 1        // Vuelo libre y exploración sin cronómetro
} GameMode;

// --- Caza del Jugador ---
typedef struct PlayerJet {
    Vector3 position;           // Posición 3D (X, Y, Z)
    Vector3 prevPosition;       // Posición frame anterior
    Vector3 velocity;           // Vector velocidad 3D
    Vector3 forward;            // Vector frontal unitario

    float heading;              // Rumbo horizontal [0 .. 2*PI]
    float pitch;                // Ángulo actual cabeceo
    float targetPitch;          // Objetivo según inputs
    float roll;                 // Ángulo actual alabeo
    float targetRoll;           // Objetivo según inputs

    float forwardSpeed;         // Velocidad de avance
    float throttle;             // Nivel acelerador [0.0 .. 1.0]
    bool isAfterburner;         // Turbo activo
    bool isAirbrake;            // Aerofrenos activos
    float burnerAnimTimer;      // Timer llama postcombustión

    // Boost Energy
    float boostEnergy;          // [0.0 .. 100.0]
    float maxBoostEnergy;
    bool isBoostDepleted;
    float perfectGateTimer;
    bool wasSupersonic;

    // Altímetros y Telemetría
    float altitudeMSL;
    float altitudeAGL;
    float groundHeight;
    float terrainSlope;
    bool isTerrainWarning;
    float wallImpactTimer;
    bool wallImpactTriggered;
    int wallImpactCount;

    float speedKnots;
    float speedKmh;
    float machNumber;

    // Deriva centrífuga y aerofrenos independientes
    float driftAngle;
    float lateralSlip;
    bool airbrakeLeft;
    bool airbrakeRight;
    bool inGroundShear;

    // Entradas normalizadas
    float inputXNorm;
    float inputYNorm;

    // Inercia en pantalla
    Vector2 screenOffset;
    Vector2 targetScreenOffset;

    // Capacidades de vuelo
    float cruiseSpeed;
    float afterburnerSpeed;
    float turnRate;
    float pitchRate;
    float rollRate;
    float acceleration;
    float climbRate;
    float baseScale;

    // Efecto Suelo & Levitación
    float hoverHeight;
    float verticalVelocity;
    float groundEffectRatio;
    Vector3 groundNormal;

    // Energía Cinética (KE)
    float kineticEnergy;
    float maxKineticEnergy;
    float kineticEfficiency;
    float climbCeiling;
    bool isStalling;
    float stallTimer;
} PlayerJet;

// --- Ajustes Gráficos y de Control ---
typedef enum RenderDistance {
    RENDER_DIST_LOW  = 0,
    RENDER_DIST_HIGH = 1
} RenderDistance;

typedef struct GameSettings {
    RenderDistance renderDistance;
    bool pixelFilterEnabled;    // Aperture grille CRT
    bool scanlinesEnabled;      // CRT Scanlines
    bool blueFilterEnabled;     // Color grade azul cinematográfico
    HUDColorTheme hudTheme;     // Paleta del HUD
    float masterVolume;         // Volumen maestro
    bool invertPitch;           // Invertir cabeceo (arriba = picar)
    bool fullscreen;            // Pantalla completa
} GameSettings;

// --- Spritesheet 2D ---
typedef struct SpriteSheet {
    Texture2D texture;
    int cols;
    int rows;
    float frameWidth;
    float frameHeight;
    bool isLoaded;
} SpriteSheet;

// --- Cámara de Vuelo Dinámica ---
typedef struct FlightCamera {
    Camera3D camera;
    float currentRoll;
    float targetRoll;
    float currentFov;
    float targetFov;
    float trauma;
} FlightCamera;

// --- Terreno Procedural ---
typedef enum BiomeType {
    BIOME_DELTASTRAITS = 0,
    BIOME_HOTSANDS     = 1,
    BIOME_COUNT        = 2
} BiomeType;

typedef struct TerrainChunk {
    int chunkX;
    int chunkZ;
    Vector3 worldPosition;
    Mesh mesh;
    Model model;
    bool isLoaded;
} TerrainChunk;

typedef struct TerrainSystem {
    TerrainChunk chunks[TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID];
    Shader terrainShader;
    Shader waterShader;
    Model waterPlaneModel;
    BiomeType currentBiome;

    Texture2D tex1;
    Texture2D tex2;
    Texture2D tex3;
    Texture2D tex4;
    Texture2D tex5;
    Texture2D texWater;

    int centerChunkX;
    int centerChunkZ;

    int locCameraPos;
    int locSunDirection;
    int locFogColor;
    int locFogStart;
    int locFogEnd;
    int locBiomeType;
    int locWaterLevel;
    int locTime;

    int locWaterTime;
    int locWaterCamPos;
    int locWaterSunDir;
    int locWaterFogColor;
    int locWaterFogStart;
    int locWaterFogEnd;
} TerrainSystem;

// --- Vegetación y Props Billboard ---
typedef struct BillboardProp {
    Vector3 position;
    float width;
    float height;
    int textureIndex;
    float randomRotation;
} BillboardProp;

typedef struct SceneryChunk {
    int chunkX;
    int chunkZ;
    BillboardProp props[MAX_PROPS_PER_CHUNK];
    int propCount;
    bool isLoaded;
} SceneryChunk;

typedef struct ScenerySystem {
    SceneryChunk chunks[TERRAIN_CHUNK_GRID * TERRAIN_CHUNK_GRID];
    BiomeType currentBiome;
    Texture2D texTree1;
    Texture2D texTree2;
    Texture2D texTree3;
    Texture2D texBush1;
    Texture2D texBush2;
    int centerChunkX;
    int centerChunkZ;
} ScenerySystem;

#endif // GAME_TYPES_H
