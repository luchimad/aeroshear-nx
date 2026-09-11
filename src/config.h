#ifndef CONFIG_H
#define CONFIG_H

#include "raylib.h"

// ============================================================================
// AEROSHEAR // FLIGHT CORE - ALPHA EDITION CONFIGURATION
// ============================================================================

// --- Ventana y Rendimiento ---
#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           720
#define TARGET_FPS              60
#define GAME_TITLE_NAME         "AEROSHEAR::NX"
#define WINDOW_TITLE            "AEROSHEAR::NX // ALPHA EDITION"

// --- Tipografías del Sistema ---
#define PATH_FONT_LOGO          "assets/Fonts/Neuropolitical Rg.otf"
#define PATH_FONT_HUD           "assets/Fonts/WO3.ttf"
#define PATH_FONT_TITLE         "assets/Fonts/2097.ttf"
#define PATH_FONT_MENU          "assets/Fonts/Fusion.ttf"

// --- Físicas y Dinámica de Vuelo 360° ---
#define SPEED_CRUISE            880.0f      // Velocidad base hacia adelante (m/s) - Duplicada
#define SPEED_AFTERBURNER       1170.0f     // Velocidad máxima con Afterburner (+33%) - Duplicada
#define ACCEL_AFTERBURNER       8.0f        // Aceleración de turbo
#define DECEL_AFTERBURNER       5.0f        // Desaceleración al soltar turbo

#define YAW_TURN_RATE_DEG       50.0f       // Viraje horizontal (grados/s)
#define PITCH_RATE_DEG          38.0f       // Cabeceo vertical (grados/s)
#define MAX_ROLL_ANGLE_DEG      68.0f       // Alabeo máximo
#define MAX_PITCH_ANGLE_DEG     36.0f       // Cabeceo máximo

#define ROLL_SPEED              5.0f
#define PITCH_SPEED             4.5f
#define AUTOLEVEL_ROLL_SPEED    4.6f
#define AUTOLEVEL_PITCH_SPEED   4.2f

// --- Aceleración de Picada y Dinámica Vertical (Solución Bug Dive) ---
#define DIVE_MAX_SINK_RATE      140.0f      // Velocidad máxima de picada voluntaria (m/s, mayor control de cabeceo abajo)
#define DIVE_GRAVITY_ACCEL      65.0f       // Aceleración hacia abajo al empujar morro
#define DIVE_SPEED_CONVERSION   110.0f      // Conversión de altitud a velocidad de avance forward

// --- Sistema de Colchón de Aire y Altitud Mínima ---
#define MIN_GROUND_CLEARANCE    10.0f       // Altura mínima de levitación sobre suelo/agua
#define TERRAIN_WARN_THRESHOLD  20.0f       // Umbral de alerta visual de proximidad
#define CAMERA_GROUND_CLEARANCE 6.0f        // Altura mínima de cámara sobre terreno
#define MAX_FLIGHT_CEILING      750.0f      // Techo máximo de vuelo (metros)

// --- Cámara de Persecución Dinámica ---
#define CAMERA_DISTANCE         25.0f
#define CAMERA_HEIGHT           6.0f
#define CAMERA_LOOKAHEAD_DIST   42.0f
#define CAMERA_FOLLOW_SPEED     3.8f
#define CAMERA_TILT_FACTOR      0.32f
#define CAMERA_TILT_SPEED       3.4f

#define FOV_BRAKE               58.0f
#define FOV_CRUISE              66.0f
#define FOV_AFTERBURNER         88.0f
#define FOV_LERP_SPEED          2.5f

// --- Sistema de Boost Energy Arcade ---
#define PLAYER_MAX_BOOST        100.0f
#define BOOST_DRAIN_RATE        25.5f       // Tasa de consumo aumentada un 50% (~4 segundos a fondo)
#define BOOST_RECHARGE_CRUISE   12.0f       // Recarga pasiva
#define BOOST_RECHARGE_BRAKE    28.0f       // Recarga rápida al frenar
#define BOOST_PERFECT_GATE_BONUS 25.0f      // Bono instantáneo por centro

// --- Sistema de Energía Cinética (Ajustado: +50% duración, +50% recarga) ---
#define KE_DRAIN_CLIMB          50.0f       // Drenaje por segundo al trepar (reducido para durar +50%)
#define KE_DRAIN_OVERDRIVE      37.0f       // Drenaje por segundo en sobremarcha (reducido para durar +50%)
#define KE_RECHARGE_SHEAR       63.0f       // Recarga por vuelo rasante en colchón (+50%)
#define KE_RECHARGE_APEX        48.0f       // Recarga por fuerza G centrífuga en ápice (+50%)
#define KE_RECHARGE_DIVE        67.5f       // Recarga por compresión dinámica al picar (+50%)

// --- Terreno Procedural Infinito ---
#define TERRAIN_CHUNK_SIZE      525.0f
#define TERRAIN_GRID_RES        20
#define TERRAIN_MAX_HEIGHT      360.0f

#define TERRAIN_CHUNK_GRID_LOW  9
#define TERRAIN_FOG_START_LOW   650.0f
#define TERRAIN_FOG_END_LOW     2100.0f
#define SCENERY_RENDER_DIST_LOW 1900.0f

#define TERRAIN_CHUNK_GRID_HIGH 11
#define TERRAIN_FOG_START_HIGH  975.0f
#define TERRAIN_FOG_END_HIGH    3150.0f
#define SCENERY_RENDER_DIST_HIGH 2850.0f

#define TERRAIN_CHUNK_GRID      TERRAIN_CHUNK_GRID_HIGH
#define TERRAIN_FOG_START       TERRAIN_FOG_START_LOW
#define TERRAIN_FOG_END         TERRAIN_FOG_END_LOW
#define SCENERY_RENDER_DIST     SCENERY_RENDER_DIST_LOW

// --- Texturas Biomas Alpha (Deltastraits & Hotsands) ---
#define PATH_TEX_DS_GRASS_1     "assets/Sprites/Ground/Deltastraits/Grass_1.png"
#define PATH_TEX_DS_GRASS_2     "assets/Sprites/Ground/Deltastraits/Grass_2.png"
#define PATH_TEX_DS_GRASS_3     "assets/Sprites/Ground/Deltastraits/Grass_3.png"
#define PATH_TEX_DS_SAND_2      "assets/Sprites/Ground/Deltastraits/Sand_2.png"
#define PATH_TEX_DS_WATER       "assets/Sprites/Ground/Deltastraits/Water_1.png"

#define PATH_TEX_HS_SAND_1      "assets/Sprites/Ground/Hotsands/Sand_1.png"
#define PATH_TEX_HS_SAND_2      "assets/Sprites/Ground/Hotsands/Sand_2.png"

// --- Sprites de Objetos de Carrera y Vegetación ---
#define PATH_TEX_PYLON          "assets/Sprites/Objects/Pylon_1.png"
#define PATH_TEX_RING           "assets/Sprites/Objects/Ring_1.png"
#define PATH_TEX_TREE_1         "assets/Sprites/Objects/Tree_1.png"
#define PATH_TEX_TREE_2         "assets/Sprites/Objects/Tree_2.png"
#define PATH_TEX_TREE_3         "assets/Sprites/Objects/Tree_3.png"
#define PATH_TEX_DESERT_BUSH_1  "assets/Sprites/Objects/DesertBush_1.png"
#define PATH_TEX_DESERT_BUSH_2  "assets/Sprites/Objects/DesertBush_2.png"
#define PATH_TEX_BUILD_SMALL_1  "assets/Sprites/Objects/building_small_1.png"
#define PATH_TEX_BUILD_SMALL_2  "assets/Sprites/Objects/building_small_2.png"
#define PATH_TEX_BUILD_TALL_1   "assets/Sprites/Objects/building_tall_1.png"
#define PATH_TEX_BUILD_TALL_2   "assets/Sprites/Objects/building_tall_2.png"

// --- Parámetros de Daño y Blindaje ---
#define PLAYER_MAX_HULL         100.0f
#define DAMAGE_TREE_STRIKE      20.0f
#define COLLISION_INVULN_TIME   0.40f

// --- Configuración de Carrera Alpha ---
#define RACE_TOTAL_CHECKPOINTS  18
#define RACE_PYLON_HEIGHT       66.0f
#define RACE_PYLON_GATE_WIDTH   96.0f
#define RACE_RING_DIAMETER      160.0f
#define RACE_COUNTDOWN_SEC      3.5f

#define MAX_PROPS_PER_CHUNK     96
#define SCENERY_MAX_ALTITUDE    140.0f
#define SCENERY_MAX_SLOPE       0.32f

// --- Spritesheet Nave (raceShip1) ---
#define SPRITE_COLS             7
#define SPRITE_ROWS             5
#define SPRITE_BASE_SCALE       0.60f
#define SPRITE_SCREEN_ROLL_DEG  -12.0f
#define SPRITE_SCREEN_LAG_X     45.0f
#define SPRITE_SCREEN_LAG_Y     28.0f
#define SPRITE_Y_OFFSET_PX      15.0f
#define SPRITE_PATH_RACESHIP_1  "assets/Sprites/Vehicles/raceShip1.png"

// --- Gamepad / Controles QoL ---
#define GAMEPAD_DEADZONE_STICK  0.18f
#define GAMEPAD_TRIGGER_THRESH  0.30f

#endif // CONFIG_H
