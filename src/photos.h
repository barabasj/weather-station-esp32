// Salvapantallas: fotos propias (LittleFS, carpeta data/) o de internet
#pragma once
#include <Arduino.h>

void photosBegin();          // monta LittleFS y busca las fotos .jpg
int  localPhotoCount();
bool showNextPhoto();        // dibuja la siguiente foto en pantalla completa
void photosRescan();         // volver a leer la lista (despues de subir/borrar)
