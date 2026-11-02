// Pagina web de configuracion (http://IP-de-la-placa o http://estacion.local)
#pragma once
#include <Arduino.h>

enum WebCommand : uint8_t { CMD_NONE, CMD_NEXT, CMD_PHOTOS, CMD_INFO, CMD_RESTART, CMD_WIFI_RESET };

extern volatile bool       webSettingsChanged;   // main.cpp aplica los ajustes nuevos
extern volatile WebCommand webCommand;           // acciones pedidas desde la pagina

void webBegin();
void webLoop();
