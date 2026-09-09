#ifndef UI_CORE_H
#define UI_CORE_H

#include "raylib.h"
#include "ui_theme.h"
#include <stdbool.h>

// ============================================================================
// AEROSHEAR // CORE VECTOR UI PRIMITIVES (Y2K / ACE COMBAT 4 PRECISION)
// ============================================================================

// Inicializa y descarga recursos GPU del subsistema visual (shaders de fondo continuo)
void UI_Core_Init(void);
void UI_Core_Unload(void);

// Dibuja el fondo atmosférico orbital profundo (fiel a la referencia)
void UI_DrawOrbitalLimb(int screenWidth, int screenHeight, float time);

// Dibuja el fondo orbital profundo con desplazamiento de paralaje y tinte dinámico de bioma
void UI_DrawOrbitalLimbEx(int screenWidth, int screenHeight, float time, Vector2 parallaxOffset, Color biomeTint);

// Dibuja un fondo de hangar/briefing cinematográfico y limpio con rejilla táctica sutil
void UI_DrawTacticalBackdrop(int screenWidth, int screenHeight, float time);

// Dibuja el fondo táctico con tinte dinámico del bioma seleccionado
void UI_DrawTacticalBackdropEx(int screenWidth, int screenHeight, float time, Color biomeTint);

// Dibuja la cabecera superior minimalista unificada (estilo AEROSHEAR::NX)
void UI_DrawScreenHeader(int screenWidth, const char *title, const char *subtitle, const char *categoryTag);

// Dibuja la cabecera táctica líquida con acento de color
void UI_DrawLiquidHeader(int screenWidth, const char *title, const char *subtitle, Color accent);

// Dibuja una fila interactiva minimalista con etiqueta a la izquierda y valor a la derecha
bool UI_DrawMinimalOptionRow(Rectangle bounds, const char *label, const char *value, bool isSelected, Vector2 mousePos);

// Dibuja una barra deslizadora limpia y estilizada sin caja tosca
float UI_DrawCleanSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal, bool isSelected, Vector2 mousePos);

// Dibuja la barra inferior de navegación y ayuda de teclas centrada
void UI_DrawNavHelp(int screenWidth, int screenHeight, const char *hints);

// Dibuja la barra de registro técnico superior con cruces de referencia CAD
void UI_DrawTopRegistrationBar(int screenWidth, int y);

// Dibuja el título estilizado geométrico "A E R O S H E A R" con marca ™ en fuente futurista
void UI_DrawTitleAeroshear(int x, int y, float scale, Color color);

// Dibuja un botón de menú con la barra de degradado horizontal translúcida
bool UI_DrawMenuButton(Rectangle bounds, const char *label, bool isSelected, Vector2 mousePos);

// Dibuja corchetes vectoriales de 1px en las esquinas de un rectángulo
void UI_DrawCornerBrackets(Rectangle bounds, int bracketSize, Color color);

// Dibuja un panel de vidrio táctico estilo Ace Combat con corchetes e insignia
void UI_DrawGlassPanel(Rectangle bounds, const char *titleBadge, Color borderColor, Color bgColor);

// Dibuja un panel de vidrio líquido Y2K con esquinas biseladas y brillo translúcido
void UI_DrawGlassPanelEx(Rectangle bounds, const char *titleBadge, Color borderColor, Color bgColor, float cornerCutSize);

// Dibuja un pod o cápsula de vidrio translúcido dark future blue con brillo líquido especular y borde reactivo
void UI_DrawGlassPod(Rectangle bounds, Color borderGlow, Color bodyTint, float roundness);

// Dibuja un banner de alerta táctico de alta visibilidad centrado horizontalmente
void UI_DrawAlertBanner(float centerX, float y, const char *text, Color bannerColor, float alpha);

// Dibuja una línea de división técnica de 1px con muescas en los extremos
void UI_DrawTechDivider(int x1, int y, int x2, Color color);

// Dibuja un control deslizante (slider) técnico
float UI_DrawSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal, bool isSelected, Vector2 mousePos);

// Dibuja un selector toggle binario [ON / OFF]
bool UI_DrawToggle(Rectangle bounds, const char *label, bool state, bool isSelected, Vector2 mousePos);

// Dibuja un selector de opciones cíclico [ < OPTION > ]
int UI_DrawOptionSelector(Rectangle bounds, const char *label, const char *currentOption, bool isSelected, Vector2 mousePos);

// Dibuja una barra segmentada de precisión aeroespacial (células discretas con micro-gaps)
void UI_DrawSegmentedBar(int x, int y, int width, int height, float value, float maxValue, int totalSegments, Color activeCol, Color inactiveCol);

// Dibuja una plataforma de pedestal holográfica elíptica con rotación de marcas y columna de luz sutil
void UI_DrawHoloPedestal(Vector2 center, float radiusX, float radiusY, float time, Color color);

// Dibuja una insignia o badge vectorial táctico con fondo translúcido y texto centrado
void UI_DrawPillBadge(float x, float y, const char *text, Color borderCol, Color bgCol, Color textCol, float fontSize);

// Dibuja una insignia táctica y devuelve su ancho total renderizado (para espaciado modular dinámico)
float UI_DrawPillBadgeEx(float x, float y, const char *text, Color borderCol, Color bgCol, Color textCol, float fontSize);

// Dibuja una barra de desplazamiento vertical táctica (Scrollbar interactivo con arrastre y rueda de ratón)
void UI_DrawVerticalScrollbar(Rectangle bounds, float *scrollOffset, float contentHeight, float viewHeight, Vector2 mousePos);

// --- Primitivas de HUD de Vuelo Táctico (CERO MIRAS DE ARMAS / CERO HORIZONTE ARTIFICIAL) ---

// Dibuja el símbolo de trayectoria de vuelo (Flight Path Marker / Aircraft Datum)
void UI_DrawFlightPathMarker(Vector2 screenPos, float rollRad, Color color);

// Dibuja la cinta horizontal de rumbo / brújula en la parte superior
void UI_DrawCompassTape(int centerX, int y, int width, float headingRad, Color color);

// Dibuja la caja de velocidad estilo AC4 (Speed Tape & Mach)
void UI_DrawSpeedBox(int x, int y, float speedKnots, float machNumber, Color color, float pump);

// Dibuja la caja de altitud estilo AC4 (Altitude MSL & Radar AGL)
void UI_DrawAltitudeBox(int x, int y, float altMSL, float altAGL, bool isWarning, Color color, Color alertColor, float pump);

// Dibuja la barra segmentada de potencia y boost / afterburner
void UI_DrawThrustBar(int x, int y, int w, int h, float boostEnergy, float maxBoost, bool isAfterburner, bool isAirbrake, bool isDepleted, Color hudColor, Color warnColor);

// Dibuja la barra segmentada de Energía Cinética (Kinetic Storage & Ground Effect)
void UI_DrawKineticEnergyBar(int x, int y, int w, int h, float kineticEnergy, float maxKinetic, float groundEffectRatio, bool isStalling, Color hudColor, Color warnColor);

// Dibuja el indicador de estado aerodinámico (Ground Effect Cushion & Stall Alert)
void UI_DrawGroundEffectBadge(int x, int y, float groundEffectRatio, bool isStalling, Color hudColor, Color alertColor);

// ============================================================================
// HUD RADICAL THE DESIGNERS REPUBLIC // WIPEOUT 3 MINIMALIST CAD (SPRINT 4)
// ============================================================================

// Dibuja el marco perimétrico CAD con cruces de registro de 1px y marcas técnicas
void UI_DrawTDRCadFrame(int screenWidth, int screenHeight, Color color);

// Dibuja el velocímetro digital tDR en KM/H con barra de empuje segmentada y badges de estado
void UI_DrawTDRSpeedometer(int x, int y, float speedKmh, float machNumber, float boostEnergy, float maxBoost,
                           bool isAfterburner, bool isAirbrake, bool isDepleted, bool isOverdrive,
                           Color hudColor, Color alertColor, float pump);

// Dibuja el módulo de dinámica de vuelo, altimetría de radar AGL y reserva de Energía Cinética (KE)
void UI_DrawTDRDynamics(int x, int y, float altitudeAGL, float altitudeMSL, float kineticEnergy, float maxKE,
                        bool inGroundShear, bool isStalling, Color hudColor, Color alertColor, float pump);

// Dibuja el director de vuelo y retícula central tDR con indicador de deriva (Drift Slip Pip) y aerofrenos
void UI_DrawTDRFlightDirector(Vector2 center, float rollRad, float driftAngle, float lateralSlip,
                              bool airbrakeLeft, bool airbrakeRight, Color hudColor);

#endif // UI_CORE_H
