// Titulares de noticias desde un feed RSS
#pragma once
#include <Arduino.h>
#include "config.h"

extern String headlines[NEWS_MAX_ITEMS];   // titulo (UTF-8, solo caracteres de la fuente)
extern String sources[NEWS_MAX_ITEMS];     // medio, si el titulo lo traia ("... - Clarín")
extern int    headlineCount;

bool fetchNews();
