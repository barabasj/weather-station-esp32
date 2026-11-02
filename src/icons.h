// Iconos del clima dibujados con primitivas (escalables y animables)
#pragma once
#include <TFT_eSPI.h>
#include "weather.h"

// Dibuja el icono centrado en (cx, cy). "s" es la escala (~radio de la nube).
// El icono ocupa aprox. 2.6*s de ancho por 2.6*s de alto.
// "frame" anima el icono (0 = estatico). "bg" es el color de fondo.
void drawWeatherIcon(TFT_eSPI& g, int cx, int cy, int s, IconType type,
                     bool isDay, uint32_t frame, uint16_t bg);
