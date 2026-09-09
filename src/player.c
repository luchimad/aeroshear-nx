#include "player.h"
#include "terrain.h"
#include "biome.h"
#include "fx.h"
#include "audio.h"
#include "config.h"
#include "raymath.h"
#include <math.h>

void Player_ApplyAircraft(PlayerJet *player, const AircraftDefinition *aircraft) {
    if (!player || !aircraft) return;

    player->cruiseSpeed = aircraft->cruiseSpeed;
    player->afterburnerSpeed = aircraft->afterburnerSpeed;
    player->turnRate = aircraft->turnRate;
    player->pitchRate = aircraft->pitchRate;
    player->rollRate = aircraft->rollRate;
    player->acceleration = aircraft->acceleration;
    player->climbRate = aircraft->climbRate;
    player->baseScale = (aircraft->baseScale > 0.01f) ? aircraft->baseScale : SPRITE_BASE_SCALE;

    player->maxKineticEnergy = (aircraft->kineticCapacity > 10.0f) ? aircraft->kineticCapacity : 300.0f;
    player->kineticEfficiency = (aircraft->kineticEfficiency > 0.1f) ? aircraft->kineticEfficiency : 1.0f;
    player->hoverHeight = (aircraft->hoverHeight > 2.0f) ? aircraft->hoverHeight : 12.5f;
    player->climbCeiling = (aircraft->climbCeiling > 50.0f) ? aircraft->climbCeiling : 225.0f;
    player->kineticEnergy = player->maxKineticEnergy;

    player->forwardSpeed = player->cruiseSpeed;
    player->forward = (Vector3){ 0.0f, 0.0f, 1.0f };
    player->velocity = Vector3Scale(player->forward, player->forwardSpeed);
}

void Player_Init(PlayerJet *player) {
    player->heading = 0.0f;
    player->pitch = 0.0f;
    player->targetPitch = 0.0f;
    player->roll = 0.0f;
    player->targetRoll = 0.0f;
    player->forward = (Vector3){ 0.0f, 0.0f, 1.0f };

    const AircraftDefinition *defaultJet = Aircraft_Get(0);
    Player_ApplyAircraft(player, defaultJet);

    player->hoverHeight = (player->hoverHeight > 2.0f) ? player->hoverHeight : 12.5f;
    player->verticalVelocity = 0.0f;
    player->groundEffectRatio = 1.0f;
    player->groundNormal = (Vector3){ 0.0f, 1.0f, 0.0f };
    player->kineticEnergy = player->maxKineticEnergy;
    player->isStalling = false;
    player->stallTimer = 0.0f;

    float startGroundY = Terrain_GetHeight(0.0f, 0.0f);
    player->position = (Vector3){ 0.0f, startGroundY + player->hoverHeight, 0.0f };
    player->prevPosition = player->position;

    player->throttle = 0.0f;
    player->isAfterburner = false;
    player->isAirbrake = false;
    player->burnerAnimTimer = 0.0f;

    player->boostEnergy = PLAYER_MAX_BOOST;
    player->maxBoostEnergy = PLAYER_MAX_BOOST;
    player->isBoostDepleted = false;
    player->perfectGateTimer = 0.0f;
    player->wasSupersonic = false;

    player->groundHeight = startGroundY;
    player->altitudeMSL = player->position.y;
    player->altitudeAGL = player->hoverHeight;
    player->isTerrainWarning = false;
    player->wallImpactTimer = 0.0f;
    player->wallImpactTriggered = false;
    player->wallImpactCount = 0;

    player->speedKnots = 900.0f;
    player->speedKmh = 720.0f;
    player->machNumber = 1.36f;
    player->driftAngle = 0.0f;
    player->lateralSlip = 0.0f;
    player->airbrakeLeft = false;
    player->airbrakeRight = false;
    player->inGroundShear = false;
    player->inputXNorm = 0.0f;
    player->inputYNorm = 0.0f;

    player->screenOffset = (Vector2){ 0.0f, 0.0f };
    player->targetScreenOffset = (Vector2){ 0.0f, 0.0f };
}

void Player_Update(PlayerJet *player, const GameSettings *settings, float dt) {
    player->prevPosition = player->position;

    // ========================================================================
    // 1. GESTIÓN UNIFICADA DE ENTRADAS: TECLADO + GAMEPAD (XINPUT / DUALSENSE)
    // ========================================================================
    float rollInput = 0.0f;
    float pitchInput = 0.0f;
    bool airbrakeLeft = false;
    bool airbrakeRight = false;
    bool wantAfterburner = false;
    bool wantBrake = false;

    // A. Teclado
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  rollInput -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) rollInput += 1.0f;

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))   pitchInput += 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) pitchInput -= 1.0f;

    if (IsKeyDown(KEY_Q)) airbrakeLeft = true;
    if (IsKeyDown(KEY_E)) airbrakeRight = true;

    if (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) wantAfterburner = true;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) || IsKeyDown(KEY_C)) wantBrake = true;

    // B. Soporte de Mando / Gamepad Analógico
    if (IsGamepadAvailable(0)) {
        float gpStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float gpStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

        // Deadzone circular suave
        if (fabsf(gpStickX) > GAMEPAD_DEADZONE_STICK) {
            float sign = (gpStickX > 0) ? 1.0f : -1.0f;
            float mag = (fabsf(gpStickX) - GAMEPAD_DEADZONE_STICK) / (1.0f - GAMEPAD_DEADZONE_STICK);
            rollInput += sign * mag;
        }

        if (fabsf(gpStickY) > GAMEPAD_DEADZONE_STICK) {
            float sign = (gpStickY > 0) ? 1.0f : -1.0f;
            float mag = (fabsf(gpStickY) - GAMEPAD_DEADZONE_STICK) / (1.0f - GAMEPAD_DEADZONE_STICK);
            // Stick hacia adelante (Y negativo) = Subir por defecto en vuelo estándar arcade
            pitchInput -= sign * mag;
        }

        // D-Pad alternativo
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  rollInput -= 1.0f;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) rollInput += 1.0f;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP))    pitchInput += 1.0f;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  pitchInput -= 1.0f;

        // Gatillos analógicos LT / RT para Aerofrenos Independientes
        float lt = (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER) + 1.0f) * 0.5f;
        float rt = (GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER) + 1.0f) * 0.5f;

        if (lt > GAMEPAD_TRIGGER_THRESH || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) airbrakeLeft = true;
        if (rt > GAMEPAD_TRIGGER_THRESH || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) airbrakeRight = true;

        // Botones de acción (A / Cruz = Turbo, B / Círculo o X / Cuadrado = Freno)
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) wantAfterburner = true;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) wantBrake = true;
    }

    // Inversión de Cabeceo si está activa en Settings (Estilo Palanca de Vuelo Real)
    if (settings && settings->invertPitch) {
        pitchInput = -pitchInput;
    }

    // Aerofrenos independientes aumentan el alabeo de entrada
    player->airbrakeLeft = airbrakeLeft;
    player->airbrakeRight = airbrakeRight;
    if (airbrakeLeft)  rollInput -= 0.65f;
    if (airbrakeRight) rollInput += 0.65f;

    rollInput = Clamp(rollInput, -1.0f, 1.0f);
    pitchInput = Clamp(pitchInput, -1.0f, 1.0f);

    // Suavizado cinemático
    player->inputXNorm = Lerp(player->inputXNorm, rollInput, dt * 14.0f);
    player->inputYNorm = Lerp(player->inputYNorm, pitchInput, dt * 14.0f);

    player->targetRoll = (rollInput != 0.0f) ? (rollInput * (MAX_ROLL_ANGLE_DEG * DEG2RAD)) : 0.0f;
    player->targetPitch = (pitchInput != 0.0f) ? (-pitchInput * (MAX_PITCH_ANGLE_DEG * DEG2RAD)) : 0.0f;

    float rollSpeedDynamic = (player->rollRate > 0.1f) ? player->rollRate : ROLL_SPEED;
    float pitchSpeedDynamic = (player->pitchRate > 0.1f) ? player->pitchRate : PITCH_SPEED;

    player->roll = Lerp(player->roll, player->targetRoll, dt * ((rollInput == 0.0f) ? AUTOLEVEL_ROLL_SPEED : rollSpeedDynamic));
    player->pitch = Lerp(player->pitch, player->targetPitch, dt * ((pitchInput == 0.0f) ? AUTOLEVEL_PITCH_SPEED : pitchSpeedDynamic));

    // Viraje 360°
    float turnRateDynamic = (player->turnRate > 0.1f) ? player->turnRate : 1.65f;
    if (airbrakeLeft || airbrakeRight) turnRateDynamic *= 1.30f;
    float turnRate = (player->roll / (MAX_ROLL_ANGLE_DEG * DEG2RAD)) * turnRateDynamic;
    player->heading += turnRate * dt;

    if (player->heading < 0.0f) player->heading += 2.0f * PI;
    if (player->heading >= 2.0f * PI) player->heading -= 2.0f * PI;

    // Desplazamiento de inercia de sprite en pantalla
    player->targetScreenOffset.x = player->inputXNorm * SPRITE_SCREEN_LAG_X;
    player->targetScreenOffset.y = -player->inputYNorm * SPRITE_SCREEN_LAG_Y;
    player->screenOffset.x = Lerp(player->screenOffset.x, player->targetScreenOffset.x, dt * 5.5f);
    player->screenOffset.y = Lerp(player->screenOffset.y, player->targetScreenOffset.y, dt * 5.5f);

    // ========================================================================
    // 2. SISTEMA DE PROPULSIÓN & AFTERBURNER SLINGSHOT
    // ========================================================================
    player->isAirbrake = wantBrake || (airbrakeLeft && airbrakeRight);

    if (player->isAirbrake) {
        player->isAfterburner = false;
        player->boostEnergy += dt * BOOST_RECHARGE_BRAKE;
        float brakeTargetSpeed = player->cruiseSpeed * 0.45f;
        player->forwardSpeed = Lerp(player->forwardSpeed, brakeTargetSpeed, dt * (player->acceleration * 1.8f));
    } else if (wantAfterburner && !player->isBoostDepleted && player->boostEnergy > 0.0f) {
        player->isAfterburner = true;
        player->boostEnergy -= dt * BOOST_DRAIN_RATE;
        if (player->boostEnergy <= 0.0f) {
            player->boostEnergy = 0.0f;
            player->isBoostDepleted = true;
            player->isAfterburner = false;
        }

        // Kinetic Overdrive: si hay KE, Afterburner alcanza sobremarcha hipersónica consumiendo KE
        float targetSpeed = player->afterburnerSpeed;
        float accelRate = player->acceleration;

        if (player->kineticEnergy > 15.0f) {
            float keRatio = player->kineticEnergy / player->maxKineticEnergy;
            targetSpeed = player->afterburnerSpeed * (1.0f + 0.12f * keRatio);
            accelRate *= 1.40f;
            player->kineticEnergy -= KE_DRAIN_OVERDRIVE * dt; // Drena KE activamente
        }

        player->forwardSpeed = Lerp(player->forwardSpeed, targetSpeed, dt * accelRate);
    } else {
        player->isAfterburner = false;
        player->boostEnergy += dt * BOOST_RECHARGE_CRUISE;
        if (player->forwardSpeed > player->cruiseSpeed) {
            float excess = player->forwardSpeed - player->cruiseSpeed;
            player->forwardSpeed -= excess * 0.30f * dt;
        } else {
            player->forwardSpeed = Lerp(player->forwardSpeed, player->cruiseSpeed, dt * (player->acceleration * 0.65f));
        }
    }

    player->boostEnergy = Clamp(player->boostEnergy, 0.0f, player->maxBoostEnergy);
    if (player->isBoostDepleted && player->boostEnergy > 20.0f) {
        player->isBoostDepleted = false;
    }

    if (player->perfectGateTimer > 0.0f) player->perfectGateTimer -= dt;

    player->burnerAnimTimer += dt * (player->isAfterburner ? 35.0f : (player->isAirbrake ? 0.0f : 15.0f));
    if (player->burnerAnimTimer > 1000.0f) player->burnerAnimTimer -= 1000.0f;

    // ========================================================================
    // 3. TERRENO, COLCHÓN DE AIRE Y DINÁMICA DE ENERGÍA CINÉTICA (SOLUCIÓN KE)
    // ========================================================================
    float groundCurrent = Terrain_GetHeight(player->position.x, player->position.z);
    Vector3 gNorm = Terrain_GetNormal(player->position.x, player->position.z);
    const BiomeDefinition *bDef = Biome_Get(Biome_GetActive());
    if (bDef && bDef->hasWater && groundCurrent < bDef->waterLevel) {
        groundCurrent = bDef->waterLevel;
        gNorm = (Vector3){ 0.0f, 1.0f, 0.0f };
    }
    player->groundHeight = groundCurrent;
    player->groundNormal = gNorm;

    float fwdSpeed = fmaxf(player->forwardSpeed, 200.0f);
    float lookAheadMid = fmaxf(16.0f, fwdSpeed * 0.06f);
    float lookAheadFar = fmaxf(32.0f, fwdSpeed * 0.12f);

    float groundMid = Terrain_GetHeight(player->position.x + player->forward.x * lookAheadMid,
                                        player->position.z + player->forward.z * lookAheadMid);
    float groundFar = Terrain_GetHeight(player->position.x + player->forward.x * lookAheadFar,
                                        player->position.z + player->forward.z * lookAheadFar);
    if (bDef && bDef->hasWater) {
        if (groundMid < bDef->waterLevel) groundMid = bDef->waterLevel;
        if (groundFar < bDef->waterLevel) groundFar = bDef->waterLevel;
    }

    float anticipatedGround = fmaxf(groundCurrent, fmaxf(groundMid * 0.98f, groundFar * 0.94f));
    float hCushion = player->hoverHeight;
    float targetHoverY = anticipatedGround + hCushion;
    float currentAGL = player->position.y - groundCurrent;

    // --- RECARGA POR RIESGO 1: COLCHÓN GROUND-SHEAR (< 18m AGL) ---
    if (currentAGL < hCushion + 6.0f) {
        player->inGroundShear = true;
        float compression = Clamp(1.0f - (currentAGL - hCushion) / 6.0f, 0.0f, 1.0f);
        player->kineticEnergy += KE_RECHARGE_SHEAR * compression * dt;

        if (!player->isAirbrake) {
            player->forwardSpeed += 16.0f * compression * dt;
            float maxShear = player->cruiseSpeed * 1.18f;
            if (!player->isAfterburner && player->forwardSpeed > maxShear) player->forwardSpeed = maxShear;
        }
    } else {
        player->inGroundShear = false;
    }

    // --- RECARGA POR RIESGO 2: APEX G-LOAD (FUERZA CENTRÍFUGA EN VIRAJE) ---
    float turnStress = fabsf(player->roll) / (MAX_ROLL_ANGLE_DEG * DEG2RAD);
    if (turnStress > 0.35f) {
        player->kineticEnergy += KE_RECHARGE_APEX * (turnStress - 0.35f) * dt;
    }

    // --- CONSUMO ACTIVO POR TREPADA ---
    bool isClimbing = (pitchInput > 0.05f);
    if (isClimbing) {
        player->kineticEnergy -= KE_DRAIN_CLIMB * pitchInput * dt;
    }

    player->kineticEnergy = Clamp(player->kineticEnergy, 0.0f, player->maxKineticEnergy);
    player->isStalling = (player->kineticEnergy < 15.0f);

    // ========================================================================
    // 4. PICADA AGRESIVA Y ACELERADA (SOLUCIÓN BUG PICADA / DIVE ACCELERATION)
    // ========================================================================
    bool isDiving = (pitchInput < -0.05f);
    if (isDiving) {
        float diveMag = fabsf(pitchInput);
        // Aceleración descendente contundente (hasta -95 m/s)
        float targetSink = -DIVE_MAX_SINK_RATE * diveMag;
        player->verticalVelocity = Lerp(player->verticalVelocity, targetSink, dt * 5.5f);

        // Slingshot gravitatorio: convierte altitud en velocidad de avance horizontal
        float diveSpeedBoost = DIVE_SPEED_CONVERSION * diveMag * dt;
        player->forwardSpeed += diveSpeedBoost;
        float maxDiveSpeed = player->afterburnerSpeed * 1.15f;
        if (player->forwardSpeed > maxDiveSpeed) player->forwardSpeed = maxDiveSpeed;

        // Recarga de Boost y KE por compresión dinámica
        player->boostEnergy += 35.0f * diveMag * dt;
        player->kineticEnergy += KE_RECHARGE_DIVE * diveMag * dt;
    } else if (isClimbing && player->kineticEnergy > 0.0f) {
        // Trepada activa suave impulsada por KE
        float climbPower = player->climbRate * 0.50f * (player->kineticEnergy / player->maxKineticEnergy);
        player->verticalVelocity = Lerp(player->verticalVelocity, climbPower, dt * 5.0f);
    } else {
        // Sin entrada vertical: seguimiento de colchón
        float heightError = targetHoverY - player->position.y;
        if (heightError > 0.0f) {
            player->verticalVelocity = Lerp(player->verticalVelocity, heightError * 5.2f, dt * 6.5f);
        } else {
            float dropSpeed = heightError * 2.8f;
            if (player->isStalling) dropSpeed = -22.0f;
            player->verticalVelocity = Lerp(player->verticalVelocity, dropSpeed, dt * 4.5f);
        }
    }

    // Techo máximo
    float maxCeilingY = groundCurrent + player->climbCeiling;
    if (player->position.y > maxCeilingY) {
        player->position.y = maxCeilingY;
        if (player->verticalVelocity > 0.0f) player->verticalVelocity = 0.0f;
        player->isStalling = true;
    }

    player->position.y += player->verticalVelocity * dt;

    // Piso de seguridad elástico suave
    const float MIN_FLOOR = 8.5f;
    float minFloorY = groundCurrent + MIN_FLOOR;
    if (player->position.y < minFloorY) {
        player->position.y = Lerp(player->position.y, minFloorY, dt * 22.0f);
        if (player->verticalVelocity < 0.0f) {
            player->verticalVelocity = Lerp(player->verticalVelocity, 8.0f, dt * 16.0f);
        }
    }

    player->altitudeAGL = player->position.y - groundCurrent;
    player->altitudeMSL = player->position.y;

    // ========================================================================
    // 5. MOVIMIENTO 3D, DERRAPE CENTRÍFUGO Y DERIVA AERODINÁMICA
    // ========================================================================
    Vector3 fwd;
    fwd.x = -sinf(player->heading);
    fwd.y = 0.0f;
    fwd.z = cosf(player->heading);
    player->forward = Vector3Normalize(fwd);

    Vector3 targetVelH = Vector3Scale(player->forward, player->forwardSpeed);
    float grip = (airbrakeLeft || airbrakeRight) ? 3.6f : 8.5f;
    player->velocity.x = Lerp(player->velocity.x, targetVelH.x, dt * grip);
    player->velocity.z = Lerp(player->velocity.z, targetVelH.z, dt * grip);
    player->velocity.y = 0.0f;

    Vector3 rightVec = Vector3Normalize(Vector3CrossProduct(player->forward, (Vector3){ 0, 1, 0 }));
    if (Vector3Length(rightVec) < 0.1f) rightVec = (Vector3){ 1, 0, 0 };

    player->lateralSlip = Vector3DotProduct(player->velocity, rightVec);
    Vector2 vH = { player->velocity.x, player->velocity.z };
    float vLen = Vector2Length(vH);
    if (vLen > 2.0f) {
        float dotFwd = Vector3DotProduct(player->forward, (Vector3){ vH.x / vLen, 0.0f, vH.y / vLen });
        player->driftAngle = acosf(Clamp(dotFwd, -1.0f, 1.0f)) * RAD2DEG;
    } else {
        player->driftAngle = 0.0f;
    }

    if (player->driftAngle > 10.0f) {
        player->forwardSpeed -= (player->driftAngle - 10.0f) * 0.15f * dt;
        if (player->forwardSpeed < player->cruiseSpeed * 0.65f) player->forwardSpeed = player->cruiseSpeed * 0.65f;
    }

    player->position.x += player->velocity.x * dt;
    player->position.z += player->velocity.z * dt;

    if (player->wallImpactTimer > 0.0f) player->wallImpactTimer -= dt;

    // Telemetría para HUD
    float speedRatio = player->forwardSpeed / player->cruiseSpeed;
    player->speedKnots = speedRatio * 595.0f;
    player->speedKmh = player->forwardSpeed * 3.6f;
    player->machNumber = player->speedKnots / 661.0f;

    if (!player->wasSupersonic && player->machNumber >= 1.0f) {
        player->wasSupersonic = true;
        Audio_PlaySonicBoom();
    } else if (player->wasSupersonic && player->machNumber < 0.96f) {
        player->wasSupersonic = false;
    }

    player->throttle = (player->forwardSpeed - player->cruiseSpeed) / (player->afterburnerSpeed - player->cruiseSpeed);
    player->throttle = Clamp(player->throttle, 0.0f, 1.0f);
}

// ============================================================================
// DIBUJADO DE SPRITE REACTIVO A PICADA, CAÍDA POR GRAVEDAD Y ALABEO
// ============================================================================
void DrawPlayerSprite(const PlayerJet *player, const SpriteSheet *sheet, const Camera3D *camera, int viewWidth, int viewHeight) {
    if (!sheet || !sheet->isLoaded) return;
    if (viewWidth <= 0) viewWidth = 1280;
    if (viewHeight <= 0) viewHeight = 720;

    // 1. Columna horizontal: mapeada a la entrada de viraje
    float normX = (player->inputXNorm + 1.0f) * 0.5f;
    normX = Clamp(normX, 0.0f, 1.0f);
    int cols = (sheet->cols > 1) ? sheet->cols : 1;
    int col = (int)roundf(normX * (float)(cols - 1));
    col = Clamp(col, 0, cols - 1);

    // 2. Fila vertical (SOLUCIÓN BUG PICADA POR CAÍDA):
    // Combina inputYNorm con la velocidad vertical real. Si la nave está cayendo (verticalVelocity < -6m/s),
    // se inclina progresivamente hacia la pose de picada (fila 3 o 4) aunque no se presione S.
    float effectivePitch = player->inputYNorm;
    if (player->verticalVelocity < -6.0f) {
        float sinkTilt = Clamp((-player->verticalVelocity - 6.0f) / 38.0f, 0.0f, 1.0f);
        effectivePitch = Lerp(effectivePitch, -sinkTilt, 0.70f);
    } else if (player->verticalVelocity > 10.0f) {
        float climbTilt = Clamp((player->verticalVelocity - 10.0f) / 45.0f, 0.0f, 1.0f);
        effectivePitch = Lerp(effectivePitch, climbTilt, 0.55f);
    }

    float normY = (1.0f - effectivePitch) * 0.5f;
    normY = Clamp(normY, 0.0f, 1.0f);
    int rows = (sheet->rows > 1) ? sheet->rows : 1;
    int row = (int)roundf(normY * (float)(rows - 1));
    row = Clamp(row, 0, rows - 1);

    // 3. Proyección en pantalla
    Vector2 screenPos = GetWorldToScreenEx(player->position, *camera, viewWidth, viewHeight);

    float resScale = (float)viewHeight / 720.0f;
    float baseScale = (player->baseScale > 0.01f) ? player->baseScale : SPRITE_BASE_SCALE;
    float scale = baseScale * resScale;
    float drawW = sheet->frameWidth * scale;
    float drawH = sheet->frameHeight * scale;

    Rectangle sourceRec = {
        (float)col * sheet->frameWidth,
        (float)row * sheet->frameHeight,
        sheet->frameWidth,
        sheet->frameHeight
    };

    float lagX = player->screenOffset.x * resScale;
    float lagY = player->screenOffset.y * resScale;
    float yOffset = SPRITE_Y_OFFSET_PX * resScale;

    Rectangle destRec = {
        screenPos.x + lagX,
        screenPos.y + yOffset + lagY,
        drawW,
        drawH
    };

    Vector2 origin = { drawW * 0.5f, drawH * 0.5f };
    float driftTilt = Clamp(player->lateralSlip * -0.05f, -8.0f, 8.0f);
    float screenRoll = (player->inputXNorm * -12.0f) + driftTilt;

    DrawTexturePro(sheet->texture, sourceRec, destRec, origin, screenRoll, WHITE);
}
