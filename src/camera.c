#include "camera.h"
#include "terrain.h"
#include "config.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

void FlightCamera_Init(FlightCamera *fc, Vector3 initialPlayerPos, Vector3 initialForward) {
    if (Vector3Length(initialForward) < 0.1f) {
        initialForward = (Vector3){ 0.0f, 0.0f, 1.0f };
    }
    initialForward = Vector3Normalize(initialForward);

    fc->camera.position = Vector3Subtract(initialPlayerPos, Vector3Scale(initialForward, CAMERA_DISTANCE));
    fc->camera.position.y += CAMERA_HEIGHT;

    fc->camera.target = Vector3Add(initialPlayerPos, Vector3Scale(initialForward, CAMERA_LOOKAHEAD_DIST));
    fc->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    fc->camera.fovy = FOV_CRUISE;
    fc->camera.projection = CAMERA_PERSPECTIVE;

    fc->currentRoll = 0.0f;
    fc->targetRoll = 0.0f;
    fc->currentFov = FOV_CRUISE;
    fc->targetFov = FOV_CRUISE;
    fc->trauma = 0.0f;
}

void FlightCamera_Update(FlightCamera *fc, const PlayerJet *player, float dt) {
    // 1. Inclinación Dinámica del Horizonte (Horizon Tilt con camera.up)
    fc->targetRoll = player->roll * CAMERA_TILT_FACTOR;
    fc->currentRoll = Lerp(fc->currentRoll, fc->targetRoll, dt * CAMERA_TILT_SPEED);

    Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(player->forward, worldUp));
    if (Vector3Length(right) < 0.01f) right = (Vector3){ 1.0f, 0.0f, 0.0f };
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, player->forward));

    fc->camera.up = Vector3Normalize(Vector3Add(
        Vector3Scale(up, cosf(fc->currentRoll)),
        Vector3Scale(right, sinf(fc->currentRoll))
    ));

    // 2. FOV Progresivo y Gradual mapeado exclusivamente a la velocidad real del avión
    float speedDelta = (player->forwardSpeed - player->cruiseSpeed);
    float targetFov = FOV_CRUISE;

    if (speedDelta >= 0.0f) {
        float turboFraction = (player->afterburnerSpeed > player->cruiseSpeed) ? 
            (speedDelta / (player->afterburnerSpeed - player->cruiseSpeed)) : 0.0f;
        if (turboFraction > 1.0f) turboFraction = 1.0f;
        targetFov = Lerp(FOV_CRUISE, FOV_AFTERBURNER, turboFraction);
    } else {
        float brakeFraction = (player->cruiseSpeed > 0.0f) ? (-speedDelta / (player->cruiseSpeed * 0.60f)) : 0.0f;
        if (brakeFraction > 1.0f) brakeFraction = 1.0f;
        targetFov = Lerp(FOV_CRUISE, FOV_BRAKE, brakeFraction);
    }

    fc->targetFov = targetFov;
    fc->currentFov = Lerp(fc->currentFov, fc->targetFov, dt * FOV_LERP_SPEED);
    fc->camera.fovy = fc->currentFov;

    // 3. Posición Deseada de la Cámara con retroceso gradual según velocidad
    float distScale = 1.0f;
    if (speedDelta >= 0.0f) {
        float t = (player->afterburnerSpeed > player->cruiseSpeed) ? (speedDelta / (player->afterburnerSpeed - player->cruiseSpeed)) : 0.0f;
        distScale = 1.0f + t * 0.18f;
    } else {
        float b = (-speedDelta / (player->cruiseSpeed * 0.60f));
        distScale = 1.0f - b * 0.12f;
    }

    float camDist = CAMERA_DISTANCE * distScale;
    float camHeight = CAMERA_HEIGHT * distScale;

    Vector3 desiredPos = Vector3Subtract(player->position, Vector3Scale(player->forward, camDist));
    desiredPos.y += camHeight;

    // Vibración suave de alta velocidad sólo en velocidad máxima
    if (player->isAfterburner && player->forwardSpeed > player->cruiseSpeed * 1.15f) {
        float shakeMag = 0.12f;
        desiredPos.x += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMag;
        desiredPos.y += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMag;
        desiredPos.z += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeMag;
    }

    // Anticolisión de cámara con el suelo
    float camGroundY = Terrain_GetHeight(desiredPos.x, desiredPos.z);
    if (desiredPos.y < camGroundY + CAMERA_GROUND_CLEARANCE) {
        desiredPos.y = camGroundY + CAMERA_GROUND_CLEARANCE;
    }

    fc->camera.position = Vector3Lerp(fc->camera.position, desiredPos, dt * CAMERA_FOLLOW_SPEED);

    // 4. Punto de Enfoque hacia la Trayectoria
    float lookaheadDist = CAMERA_LOOKAHEAD_DIST * (1.0f + (distScale - 1.0f) * 0.8f);
    Vector3 desiredTarget = Vector3Add(player->position, Vector3Scale(player->forward, lookaheadDist));
    fc->camera.target = Vector3Lerp(fc->camera.target, desiredTarget, dt * (CAMERA_FOLLOW_SPEED * 1.35f));

    // 5. Sacudida de impacto violenta por colisiones (High-Frequency Unfiltered Screen Shake)
    // Se aplica DIRECTAMENTE sobre la posición final para no ser suavizada por el filtro pasa-bajos del Lerp
    if (fc->trauma > 0.001f) {
        float shake = fc->trauma * fc->trauma;
        float shakePosMag = shake * 2.8f;
        float shakeRotMag = shake * 0.085f;

        fc->camera.position.x += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakePosMag;
        fc->camera.position.y += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakePosMag * 0.70f;
        fc->camera.position.z += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakePosMag;

        fc->camera.target.x += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakePosMag * 0.75f;
        fc->camera.target.y += ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakePosMag * 0.75f;

        // Torsión angular instantánea de horizonte por impacto
        fc->camera.up = Vector3Normalize(Vector3Add(
            fc->camera.up,
            Vector3Scale(right, ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * shakeRotMag)
        ));

        // Decaimiento exponencial del trauma
        fc->trauma -= dt * 2.4f;
        if (fc->trauma < 0.0f) fc->trauma = 0.0f;
    }
}

void FlightCamera_AddTrauma(FlightCamera *fc, float amount) {
    if (!fc) return;
    fc->trauma += amount;
    if (fc->trauma > 1.0f) fc->trauma = 1.0f;
}

