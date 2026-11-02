// Datos del clima obtenidos de Open-Meteo (gratis, sin API key)
#pragma once
#include <Arduino.h>
#include "settings.h"

// Tipos de icono (se derivan del codigo WMO que devuelve Open-Meteo)
enum IconType : uint8_t {
  ICON_CLEAR, ICON_PARTLY, ICON_CLOUDY, ICON_FOG,
  ICON_DRIZZLE, ICON_RAIN, ICON_SNOW, ICON_STORM, ICON_UNKNOWN
};

#define FORECAST_DAYS  5
#define FORECAST_HOURS 24

struct DayForecast {
  int   code;
  float tMax, tMin;
  int   rainProb;   // %
  float uvMax;
};

struct HourForecast {
  int   hour;       // 0-23
  float temp;
  int   rainProb;   // %
  int   code;
  bool  isDay;
};

struct WeatherData {
  bool  valid = false;
  time_t updated = 0;
  // actual
  float temp, feels, wind, gusts, pressure, precip;
  int   humidity, windDir, cloud, code;
  bool  isDay;
  char  sunrise[6], sunset[6];   // "HH:MM"
  DayForecast  days[FORECAST_DAYS];
  HourForecast hours[FORECAST_HOURS];
  int nHours = 0;
};

#define MAX_ALERTS 4

extern WeatherData weatherAll[MAX_CITIES];
extern int cityIdx;                       // ciudad que se esta mostrando
#define weather (weatherAll[cityIdx])     // atajo: datos de la ciudad actual

bool        fetchWeather(int city);
int         buildAlerts(const WeatherData& w, String* out, int max);
IconType    iconForCode(int wmo);
const char* descForCode(int wmo);
const char* windDirName(int deg);
float       toUnit(float celsius);   // aplica Fahrenheit si corresponde
