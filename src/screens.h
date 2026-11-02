// Pantallas de la aplicacion
#pragma once
#include <TFT_eSPI.h>

enum ScreenId {
  SCR_CLOCK, SCR_CURRENT, SCR_ALERT, SCR_FORECAST, SCR_HOURLY, SCR_NEWS,
  SCR_COUNT,
  SCR_PHOTO = SCR_COUNT,       // salvapantallas (fuera de la rotacion)
  SCR_INFO                     // red, IP y direccion web (pulsacion larga de BOOT)
};

extern TFT_eSPI tft;

void screensBegin();                    // crea los sprites (despues de tft.init)
void screenDraw(ScreenId s);            // dibujo completo al entrar a la pantalla
void screenTick(ScreenId s);            // llamar seguido: animaciones, segundos, cinta
void screenMessage(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr);
void newsChanged();                     // regenerar la cinta y la lista de titulares
int  newsCycleSecs();                   // segundos para recorrer toda la lista de noticias
int  activeAlerts();                    // cantidad de alertas de la ciudad actual
void screenProgress(const char* title, int percent);   // barra de progreso (actualizacion)
