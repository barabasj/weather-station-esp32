#include "settings.h"
#include "config.h"
#include <Preferences.h>

Settings cfg;

void settingsDefaults() {
  Settings d;
  d.cityCount = 0;
  for (int i = 0; i < CITY_COUNT && i < MAX_CITIES; i++) {
    d.cities[i].name = CITIES[i].name;
    d.cities[i].lat  = CITIES[i].lat;
    d.cities[i].lon  = CITIES[i].lon;
    d.cityCount++;
  }
  d.timezone = TIMEZONE;
  d.newsEnabled = NEWS_ENABLED;
  d.newsTicker  = NEWS_TICKER;
  d.newsUrl     = NEWS_URL;
  d.newsName    = NEWS_NAME;
  d.secsClock = SECS_CLOCK;       d.secsCurrent = SECS_CURRENT;   d.secsAlert = SECS_ALERT;
  d.secsForecast = SECS_FORECAST; d.secsHourly = SECS_HOURLY;     d.secsNews = SECS_NEWS;
  d.weatherRefreshMin = WEATHER_REFRESH_MIN;
  d.newsRefreshMin    = NEWS_REFRESH_MIN;
  d.alertHeat = ALERT_HEAT_C; d.alertFrost = ALERT_FROST_C;
  d.alertWind = ALERT_WIND_KMH; d.alertHours = ALERT_HOURS;
  d.ssEnabled   = SCREENSAVER_ENABLED;
  d.ssAfterMin  = SCREENSAVER_AFTER_MIN;
  d.photoSecs   = PHOTO_SECS;
  d.photoSource = PHOTO_SOURCE;
  d.photoUrl    = PHOTO_URL;
  d.photoClock  = PHOTO_SHOW_CLOCK;
  d.brightDay = BRIGHTNESS_DAY; d.brightNight = BRIGHTNESS_NIGHT;
  d.nightFrom = NIGHT_FROM_HOUR; d.nightTo = NIGHT_TO_HOUR;
  d.fahrenheit = USE_FAHRENHEIT;
  d.h24 = USE_24H;
  cfg = d;
}

void settingsToJson(JsonDocument& doc) {
  JsonArray cs = doc["cities"].to<JsonArray>();
  for (int i = 0; i < cfg.cityCount; i++) {
    JsonObject c = cs.add<JsonObject>();
    c["name"] = cfg.cities[i].name;
    c["lat"]  = cfg.cities[i].lat;
    c["lon"]  = cfg.cities[i].lon;
  }
  doc["timezone"]     = cfg.timezone;
  doc["newsEnabled"]  = cfg.newsEnabled;
  doc["newsTicker"]   = cfg.newsTicker;
  doc["newsUrl"]      = cfg.newsUrl;
  doc["newsName"]     = cfg.newsName;
  doc["secsClock"]    = cfg.secsClock;
  doc["secsCurrent"]  = cfg.secsCurrent;
  doc["secsAlert"]    = cfg.secsAlert;
  doc["secsForecast"] = cfg.secsForecast;
  doc["secsHourly"]   = cfg.secsHourly;
  doc["secsNews"]     = cfg.secsNews;
  doc["weatherRefreshMin"] = cfg.weatherRefreshMin;
  doc["newsRefreshMin"]    = cfg.newsRefreshMin;
  doc["alertHeat"]    = cfg.alertHeat;
  doc["alertFrost"]   = cfg.alertFrost;
  doc["alertWind"]    = cfg.alertWind;
  doc["alertHours"]   = cfg.alertHours;
  doc["ssEnabled"]    = cfg.ssEnabled;
  doc["ssAfterMin"]   = cfg.ssAfterMin;
  doc["photoSecs"]    = cfg.photoSecs;
  doc["photoSource"]  = cfg.photoSource;
  doc["photoUrl"]     = cfg.photoUrl;
  doc["photoClock"]   = cfg.photoClock;
  doc["brightDay"]    = cfg.brightDay;
  doc["brightNight"]  = cfg.brightNight;
  doc["nightFrom"]    = cfg.nightFrom;
  doc["nightTo"]      = cfg.nightTo;
  doc["fahrenheit"]   = cfg.fahrenheit;
  doc["h24"]          = cfg.h24;
}

// Lee un entero del JSON (si viene) y lo limita a [lo, hi]
static void getInt(JsonVariantConst v, const char* k, int& dst, int lo, int hi) {
  if (!v[k].isNull()) dst = constrain(v[k].as<int>(), lo, hi);
}
static void getBool(JsonVariantConst v, const char* k, bool& dst) {
  if (!v[k].isNull()) dst = v[k].as<bool>();
}
static void getStr(JsonVariantConst v, const char* k, String& dst, size_t maxLen) {
  if (v[k].is<const char*>()) {
    String s = v[k].as<const char*>();
    s.trim();
    if (s.length() <= maxLen) dst = s;
  }
}

bool settingsFromJson(JsonVariantConst v) {
  if (!v.is<JsonObjectConst>()) return false;
  Settings s = cfg;

  if (v["cities"].is<JsonArrayConst>()) {
    int n = 0;
    for (JsonObjectConst c : v["cities"].as<JsonArrayConst>()) {
      if (n >= MAX_CITIES) break;
      String name = c["name"] | "";
      name.trim();
      float lat = c["lat"] | 999.0f, lon = c["lon"] | 999.0f;
      if (!name.length() || name.length() > 40 || fabsf(lat) > 90 || fabsf(lon) > 180) continue;
      s.cities[n].name = name;
      s.cities[n].lat = lat;
      s.cities[n].lon = lon;
      n++;
    }
    if (n == 0) return false;            // al menos una ciudad valida
    s.cityCount = n;
  }
  getStr(v, "timezone", s.timezone, 60);
  getBool(v, "newsEnabled", s.newsEnabled);
  getBool(v, "newsTicker", s.newsTicker);
  getStr(v, "newsUrl", s.newsUrl, 300);
  getStr(v, "newsName", s.newsName, 40);
  getInt(v, "secsClock",    s.secsClock,    0, 3600);
  getInt(v, "secsCurrent",  s.secsCurrent,  0, 3600);
  getInt(v, "secsAlert",    s.secsAlert,    0, 3600);
  getInt(v, "secsForecast", s.secsForecast, 0, 3600);
  getInt(v, "secsHourly",   s.secsHourly,   0, 3600);
  getInt(v, "secsNews",     s.secsNews,     0, 3600);
  getInt(v, "weatherRefreshMin", s.weatherRefreshMin, 5, 720);
  getInt(v, "newsRefreshMin",    s.newsRefreshMin,    5, 720);
  getInt(v, "alertHeat",  s.alertHeat,  20, 60);
  getInt(v, "alertFrost", s.alertFrost, -30, 15);
  getInt(v, "alertWind",  s.alertWind,  20, 200);
  getInt(v, "alertHours", s.alertHours, 1, 24);
  getBool(v, "ssEnabled", s.ssEnabled);
  getInt(v, "ssAfterMin", s.ssAfterMin, 1, 1440);
  getInt(v, "photoSecs",  s.photoSecs,  3, 3600);
  getInt(v, "photoSource", s.photoSource, 0, 2);
  getStr(v, "photoUrl", s.photoUrl, 300);
  getBool(v, "photoClock", s.photoClock);
  getInt(v, "brightDay",   s.brightDay,   5, 255);
  getInt(v, "brightNight", s.brightNight, 0, 255);
  getInt(v, "nightFrom",   s.nightFrom,   0, 23);
  getInt(v, "nightTo",     s.nightTo,     0, 23);
  getBool(v, "fahrenheit", s.fahrenheit);
  getBool(v, "h24", s.h24);

  // que no queden todas las pantallas apagadas
  if (s.secsClock + s.secsCurrent + s.secsForecast + s.secsHourly +
      (s.newsEnabled ? s.secsNews : 0) == 0) s.secsClock = 20;
  if (!s.timezone.length()) s.timezone = TIMEZONE;
  cfg = s;
  return true;
}

void settingsLoad() {
  settingsDefaults();
  Preferences p;
  if (!p.begin("estacion", true)) return;      // primera vez: no existe todavia
  String js = p.getString("cfg", "");
  p.end();
  if (!js.length()) return;
  JsonDocument doc;
  if (deserializeJson(doc, js) == DeserializationError::Ok) settingsFromJson(doc.as<JsonVariantConst>());
  Serial.printf("[ajustes] cargados (%u bytes)\n", js.length());
}

bool settingsSave() {
  JsonDocument doc;
  settingsToJson(doc);
  String js;
  serializeJson(doc, js);
  Preferences p;
  if (!p.begin("estacion", false)) return false;
  bool ok = p.putString("cfg", js) == js.length();
  p.end();
  Serial.printf("[ajustes] guardados: %s\n", ok ? "ok" : "ERROR");
  return ok;
}
