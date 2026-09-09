#ifndef CAMERA_H
#define CAMERA_H

#include "game_types.h"

// ============================================================================
// MÓDULO DE CÁMARA DE VUELO // SEGUIMIENTO ELÁSTICO E INCLINACIÓN DE HORIZONTE
// ============================================================================

// Inicializa la cámara de persecución 3D alineada detrás del vector de avance del caza
void FlightCamera_Init(FlightCamera *fc, Vector3 initialPlayerPos, Vector3 initialForward);

// Actualiza posición, objetivo (target), FOV dinámico e inclinación del vector UP
void FlightCamera_Update(FlightCamera *fc, const PlayerJet *player, float dt);

// Añade trauma a la cámara para sacudida (Screen Shake) ante impactos
void FlightCamera_AddTrauma(FlightCamera *fc, float amount);

#endif // CAMERA_H
