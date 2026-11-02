// Ajustes que se pueden cambiar desde la pagina web de la placa.
// Se guardan en la memoria interna (NVS); los valores iniciales salen de config.h.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#define FW_VERSION  "2.4"
#define MAX_CITIES  4

struct CityCfg {
  String name;
  float lat = 0, lon = 0;
};

struct Settings {
  CityCfg cities[MAX_CITIES];
  int     cityCount = 1;
  String  timezone;

  bool    newsEnabled, newsTicker;
  String  newsUrl, newsName;

  int     secsClock, secsCurrent, secsAlert, secsForecast, secsHourly, secsNews;
  int     weatherRefreshMin, newsRefreshMin;

  int     alertHeat, alertFrost, alertWind, alertHours;

  bool    ssEnabled;
  int     ssAfterMin, photoSecs, photoSource;
  String  photoUrl;
  bool    photoClock;

  int     brightDay, brightNight, nightFrom, nightTo;
  bool    fahrenheit, h24;
};

extern Settings cfg;

void settingsDefaults();
void settingsLoad();                         // desde NVS (o defaults)
bool settingsSave();
void settingsToJson(JsonDocument& doc);
bool settingsFromJson(JsonVariantConst v);   // valida y aplica a cfg
