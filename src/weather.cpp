#include "weather.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#undef weather   // en este archivo se usa la ciudad indicada, no la actual

WeatherData weatherAll[MAX_CITIES];
int cityIdx = 0;

static String buildUrl(const CityCfg& c) {
  String u = "https://api.open-meteo.com/v1/forecast?latitude=";
  u += String(c.lat, 4);
  u += "&longitude=";
  u += String(c.lon, 4);
  u += "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,"
       "precipitation,weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m"
       "&hourly=temperature_2m,precipitation_probability,weather_code,is_day"
       "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
       "precipitation_probability_max,uv_index_max,sunrise,sunset"
       "&timezone=auto&forecast_days=" + String(FORECAST_DAYS) +
       "&forecast_hours=" + String(FORECAST_HOURS);
  return u;
}

// "2026-11-01T06:12" -> "06:12"
static void copyHHMM(char* dst, const char* iso) {
  if (iso && strlen(iso) >= 16) { memcpy(dst, iso + 11, 5); dst[5] = 0; }
  else strcpy(dst, "--:--");
}

bool fetchWeather(int city) {
  if (WiFi.status() != WL_CONNECTED || city >= cfg.cityCount) return false;
  WeatherData& weather = weatherAll[city];

  WiFiClientSecure client;
  client.setInsecure();               // no validamos el certificado (simple)
  HTTPClient http;
  http.useHTTP10(true);               // evita "chunked" para leer el stream directo
  http.setTimeout(15000);
  if (!http.begin(client, buildUrl(cfg.cities[city]))) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[clima] HTTP %d\n", code);
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    Serial.printf("[clima] JSON: %s\n", err.c_str());
    return false;
  }

  JsonObject cur = doc["current"];
  weather.temp     = cur["temperature_2m"]       | 0.0f;
  weather.feels    = cur["apparent_temperature"] | 0.0f;
  weather.humidity = cur["relative_humidity_2m"] | 0;
  weather.isDay    = (cur["is_day"] | 1) == 1;
  weather.precip   = cur["precipitation"]        | 0.0f;
  weather.code     = cur["weather_code"]         | 0;
  weather.cloud    = cur["cloud_cover"]          | 0;
  weather.pressure = cur["pressure_msl"]         | 0.0f;
  weather.wind     = cur["wind_speed_10m"]       | 0.0f;
  weather.windDir  = cur["wind_direction_10m"]   | 0;
  weather.gusts    = cur["wind_gusts_10m"]       | 0.0f;

  JsonObject d = doc["daily"];
  for (int i = 0; i < FORECAST_DAYS; i++) {
    weather.days[i].code     = d["weather_code"][i]                  | 0;
    weather.days[i].tMax     = d["temperature_2m_max"][i]            | 0.0f;
    weather.days[i].tMin     = d["temperature_2m_min"][i]            | 0.0f;
    weather.days[i].rainProb = d["precipitation_probability_max"][i] | 0;
    weather.days[i].uvMax    = d["uv_index_max"][i]                  | 0.0f;
  }
  copyHHMM(weather.sunrise, d["sunrise"][0] | "");
  copyHHMM(weather.sunset,  d["sunset"][0]  | "");

  JsonObject h = doc["hourly"];
  JsonArray times = h["time"];
  int n = 0;
  for (int i = 0; i < (int)times.size() && n < FORECAST_HOURS; i++, n++) {
    const char* t = times[i] | "";
    weather.hours[n].hour     = strlen(t) >= 13 ? atoi(t + 11) : 0;
    weather.hours[n].temp     = h["temperature_2m"][i]            | 0.0f;
    weather.hours[n].rainProb = h["precipitation_probability"][i] | 0;
    weather.hours[n].code     = h["weather_code"][i]              | 0;
    weather.hours[n].isDay    = (h["is_day"][i] | 1) == 1;
  }
  weather.nHours  = n;
  weather.updated = time(nullptr);
  weather.valid   = true;
  Serial.printf("[clima] %s OK  %.1f C  codigo %d\n", cfg.cities[city].name.c_str(), weather.temp, weather.code);
  return true;
}

IconType iconForCode(int c) {
  switch (c) {
    case 0: case 1:                 return ICON_CLEAR;
    case 2:                         return ICON_PARTLY;
    case 3:                         return ICON_CLOUDY;
    case 45: case 48:               return ICON_FOG;
    case 51: case 53: case 55:
    case 56: case 57:               return ICON_DRIZZLE;
    case 61: case 63: case 65:
    case 66: case 67:
    case 80: case 81: case 82:      return ICON_RAIN;
    case 71: case 73: case 75:
    case 77: case 85: case 86:      return ICON_SNOW;
    case 95: case 96: case 99:      return ICON_STORM;
    default:                        return ICON_UNKNOWN;
  }
}

const char* descForCode(int c) {
  switch (c) {
    case 0:  return "Despejado";
    case 1:  return "Mayormente despejado";
    case 2:  return "Parcialmente nublado";
    case 3:  return "Nublado";
    case 45: return "Niebla";
    case 48: return "Niebla con escarcha";
    case 51: return "Llovizna leve";
    case 53: return "Llovizna";
    case 55: return "Llovizna intensa";
    case 56: case 57: return "Llovizna helada";
    case 61: return "Lluvia leve";
    case 63: return "Lluvia";
    case 65: return "Lluvia fuerte";
    case 66: case 67: return "Lluvia helada";
    case 71: return "Nevada leve";
    case 73: return "Nevada";
    case 75: return "Nevada fuerte";
    case 77: return "Granizo fino";
    case 80: return "Chaparrones leves";
    case 81: return "Chaparrones";
    case 82: return "Chaparrones fuertes";
    case 85: case 86: return "Chaparrones de nieve";
    case 95: return "Tormenta";
    case 96: case 99: return "Tormenta con granizo";
    default: return "---";
  }
}

const char* windDirName(int deg) {
  static const char* names[] = {"N", "NE", "E", "SE", "S", "SO", "O", "NO"};
  return names[((deg + 22) % 360) / 45];
}

float toUnit(float c) {
  return cfg.fahrenheit ? c * 9.0f / 5.0f + 32.0f : c;
}

// ---------------------------------------------------------------- alertas
static String fmtT(float c) { return String((int)lroundf(toUnit(c))) + "°"; }

int buildAlerts(const WeatherData& w, String* out, int max) {
  if (!w.valid) return 0;
  int n = 0;
  auto add = [&](const String& s) { if (n < max) out[n++] = s; };

  // Fenomenos en las proximas horas (se informa la primera hora en que ocurre)
  int hStorm = -1, hRain = -1, hSnow = -1;
  for (int i = 0; i < w.nHours && i < cfg.alertHours; i++) {
    int c = w.hours[i].code;
    if (hStorm < 0 && c >= 95) hStorm = i;
    if (hRain < 0 && (c == 65 || c == 67 || c == 82)) hRain = i;
    if (hSnow < 0 && ((c >= 71 && c <= 77) || c == 85 || c == 86)) hSnow = i;
  }
  auto when = [&](int i) {
    return i == 0 ? String("ahora") : "a las " + String(w.hours[i].hour) + " h";
  };
  if (hStorm >= 0) add("Tormentas " + when(hStorm));
  if (hRain  >= 0) add("Lluvia fuerte " + when(hRain));
  if (hSnow  >= 0) add("Nevadas " + when(hSnow));
  if (w.days[0].tMax >= cfg.alertHeat) add("Calor extremo: máxima de " + fmtT(w.days[0].tMax));
  float frost = min(w.days[0].tMin, w.days[1].tMin);
  if (frost <= cfg.alertFrost) add("Riesgo de heladas: mínima de " + fmtT(frost));
  if (w.gusts >= cfg.alertWind) add("Ráfagas de " + String((int)lroundf(w.gusts)) + " km/h");
  return n;
}
