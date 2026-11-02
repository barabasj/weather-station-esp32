// =====================================================================
//  Estacion de clima - ideaspark ESP32 (ESP-WROOM-32) + TFT 1.9" 170x320
//  Hora, fecha, clima actual animado, alertas, pronostico 5 dias,
//  grafico 24 h, titulares de noticias y salvapantallas de fotos.
//  Pagina web de configuracion en http://estacion.local
//  Datos: Open-Meteo (sin API key) + RSS.  Valores iniciales en config.h
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include "config.h"
#include "settings.h"
#include "weather.h"
#include "news.h"
#include "photos.h"
#include "screens.h"
#include "web.h"

#define INFO_SECS      20     // tiempo que queda la pantalla de informacion
#define LONG_PRESS_MS  1200   // pulsacion larga de BOOT = pantalla de informacion

static ScreenId current = SCR_CLOCK;
static unsigned long screenStart = 0;
static unsigned long lastWeather = 0, lastNews = 0, lastWifiTry = 0, lastBright = 0;
static unsigned long weatherEvery = 0, newsEvery = 0;
static unsigned long lastInteraction = 0;   // para el salvapantallas

// ---------------------------------------------------------------- brillo
static void backlightBegin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(TFT_BL, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
#endif
}

static void setBrightness(uint8_t v) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(TFT_BL, v);
#else
  ledcWrite(0, v);
#endif
}

static void updateBrightness() {
  struct tm t;
  if (!getLocalTime(&t, 0)) { setBrightness(cfg.brightDay); return; }
  bool night = (cfg.nightFrom > cfg.nightTo)
                 ? (t.tm_hour >= cfg.nightFrom || t.tm_hour < cfg.nightTo)
                 : (t.tm_hour >= cfg.nightFrom && t.tm_hour < cfg.nightTo);
  setBrightness(night ? cfg.brightNight : cfg.brightDay);
}

// ---------------------------------------------------------------- pantallas
static int screenSecs(ScreenId s) {
  switch (s) {
    case SCR_CLOCK:    return cfg.secsClock;
    case SCR_CURRENT:  return cfg.secsCurrent;
    case SCR_ALERT:    return cfg.secsAlert;
    case SCR_FORECAST: return cfg.secsForecast;
    case SCR_HOURLY:   return cfg.secsHourly;
    case SCR_NEWS:     // se queda hasta que pasan todos los titulares
      return cfg.newsEnabled && cfg.secsNews > 0 ? max(cfg.secsNews, newsCycleSecs()) : 0;
    case SCR_INFO:     return INFO_SECS;
    default:           return 0;
  }
}

static bool screenEnabled(ScreenId s) {
  if (screenSecs(s) == 0) return false;
  if (s == SCR_ALERT) return activeAlerts() > 0;
  return true;
}

static void showScreen(ScreenId s) {
  current = s;
  screenStart = millis();
  screenDraw(s);
}

static void nextScreen() {
  int from = current >= SCR_COUNT ? SCR_COUNT - 1 : current;   // desde info: al reloj
  for (int k = 1; k <= SCR_COUNT; k++) {
    int raw = from + k;
    int idx = raw % SCR_COUNT;
    // al completar una vuelta, pasar a la siguiente ciudad
    if (raw == SCR_COUNT && cfg.cityCount > 1) cityIdx = (cityIdx + 1) % cfg.cityCount;
    if (screenEnabled((ScreenId)idx)) { showScreen((ScreenId)idx); return; }
  }
  showScreen(SCR_CLOCK);
}

static void enterScreensaver(const char* why) {
  Serial.printf("[fotos] salvapantallas (%s) a los %lu s de encendida, %lu s desde la ultima interaccion\n",
                why, millis() / 1000, (millis() - lastInteraction) / 1000);
  current = SCR_PHOTO;
  screenDraw(SCR_PHOTO);
}

static void exitScreensaver() {
  lastInteraction = millis();
  showScreen(SCR_CLOCK);
}

// ---------------------------------------------------------------- red
static bool waitWifi(unsigned long timeoutMs) {
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) delay(250);
  return WiFi.status() == WL_CONNECTED;
}

static void portalStarted(WiFiManager*) {
  screenMessage("Configurar WiFi",
                "Desde el celular, conectate a la red WiFi",
                "\"" PORTAL_AP_NAME "\" y elegí tu red");
}

static bool runPortal() {
  WiFiManager wm;
  wm.setDebugOutput(false);
  wm.setTitle("Estación de clima");
  std::vector<const char*> menu = {"wifi", "info", "exit"};   // sin "update"
  wm.setMenu(menu);
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.setAPCallback(portalStarted);
  portalStarted(nullptr);
  return wm.startConfigPortal(PORTAL_AP_NAME);
}

static bool connectWifi(bool forcePortal) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("estacion-clima");
  if (forcePortal) {
    WiFi.disconnect(true, true);            // borra la red guardada
    delay(200);
    return runPortal();
  }
  // 1) red guardada (configurada antes, desde el portal o desde config.h)
  screenMessage("Conectando WiFi", "red guardada");
  WiFi.begin();
  if (waitWifi(10000)) return true;
  // 2) red fija de config.h
  if (strlen(WIFI_SSID) > 0) {
    screenMessage("Conectando WiFi", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (waitWifi(15000)) return true;
  }
  // 3) portal de configuracion desde el celular
  return runPortal();
}

static void refreshWeather() {
  lastWeather = millis();
  bool ok = true;
  for (int i = 0; i < cfg.cityCount; i++) ok &= fetchWeather(i);
  // si falla, reintentar en 1 minuto
  weatherEvery = ok ? cfg.weatherRefreshMin * 60000UL : 60000UL;
  if (current != SCR_PHOTO && current != SCR_NEWS && current != SCR_INFO) screenDraw(current);
}

static void refreshNews() {
  lastNews = millis();
  bool ok = fetchNews();
  newsEvery = ok ? cfg.newsRefreshMin * 60000UL : 2 * 60000UL;
  if (ok) newsChanged();
}

// Ajustes nuevos desde la pagina web: aplicar sin reiniciar
static void applySettings() {
  setenv("TZ", cfg.timezone.c_str(), 1);
  tzset();
  if (cityIdx >= cfg.cityCount) cityIdx = 0;
  for (int i = 0; i < MAX_CITIES; i++) weatherAll[i].valid = false;
  updateBrightness();
  screenMessage("Ajustes guardados", "Actualizando datos…");
  if (WiFi.status() == WL_CONNECTED) {
    refreshWeather();
    if (cfg.newsEnabled) refreshNews();
  }
  lastInteraction = millis();
  showScreen(SCR_CLOCK);
}

// ================================================================ setup
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n== Estacion de clima ideaspark v" FW_VERSION " ==");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  settingsLoad();

  tft.init();
  tft.setRotation(1);              // horizontal 320x170 (3 = girada 180 grados)
  backlightBegin();
  setBrightness(cfg.brightDay);
  screensBegin();
  photosBegin();

  // BOOT apretado al encender: reconfigurar el WiFi
  bool forcePortal = false;
  if (digitalRead(BUTTON_PIN) == LOW) {
    screenMessage("Soltá BOOT para", "configurar el WiFi desde el celular");
    unsigned long t0 = millis();
    while (digitalRead(BUTTON_PIN) == LOW && millis() - t0 < 5000) delay(20);
    forcePortal = millis() - t0 >= 1500;
  }

  if (!connectWifi(forcePortal)) {
    screenMessage("Sin WiFi", "Se reintentará en segundo plano");
    Serial.println("[wifi] sin conexion, se reintentara");
    delay(3000);
  } else {
    String ip = WiFi.localIP().toString();
    Serial.printf("[wifi] conectado a %s, IP %s\n", WiFi.SSID().c_str(), ip.c_str());
    webBegin();
    String url = "http://" + ip;
    screenMessage("Conectado", ("Configuración: " + url).c_str(), "o http://estacion.local");
    delay(4000);
  }

  configTzTime(cfg.timezone.c_str(), "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  if (WiFi.status() == WL_CONNECTED) {
    screenMessage("Sincronizando hora", "NTP");
    struct tm t;
    getLocalTime(&t, 10000);

    screenMessage("Consultando el clima", cfg.cities[0].name.c_str());
    refreshWeather();
    if (cfg.newsEnabled) {
      screenMessage("Cargando noticias", cfg.newsName.c_str());
      refreshNews();
    }
  } else {
    weatherEvery = newsEvery = 30000;   // reintentar pronto
  }

  updateBrightness();
  lastInteraction = millis();
  current = SCR_CLOCK;
  if (!screenEnabled(current)) nextScreen(); else showScreen(current);
}

// ================================================================ loop
void loop() {
  unsigned long now = millis();
  webLoop();

  // Boton BOOT: corto = siguiente pantalla (o salir de las fotos),
  //             largo = pantalla de informacion (IP de la pagina web)
  static bool lastBtn = HIGH, longDone = false;
  static unsigned long btnTime = 0, pressStart = 0;
  bool b = digitalRead(BUTTON_PIN);
  if (b != lastBtn && now - btnTime > 40) {
    btnTime = now;
    lastBtn = b;
    if (b == LOW) { pressStart = now; longDone = false; }
    else if (!longDone) {                       // se solto antes de la pulsacion larga
      lastInteraction = now;
      if (current == SCR_PHOTO) exitScreensaver();
      else nextScreen();
    }
  }
  if (b == LOW && !longDone && now - pressStart >= LONG_PRESS_MS && pressStart) {
    longDone = true;
    lastInteraction = now;
    showScreen(SCR_INFO);
  }

  // Pedidos de la pagina web
  if (webSettingsChanged) { webSettingsChanged = false; applySettings(); }
  if (webCommand != CMD_NONE) {
    WebCommand c = webCommand;
    webCommand = CMD_NONE;
    lastInteraction = now;
    switch (c) {
      case CMD_NEXT:   nextScreen(); break;
      case CMD_PHOTOS: enterScreensaver("pedido desde la pagina web"); break;
      case CMD_INFO:   showScreen(SCR_INFO); break;
      case CMD_RESTART:
        screenMessage("Reiniciando…");
        delay(500);
        ESP.restart();
        break;
      case CMD_WIFI_RESET:
        screenMessage("Olvidando el WiFi…", "Se va a crear la red " PORTAL_AP_NAME);
        delay(800);
        WiFi.disconnect(true, true);
        delay(300);
        ESP.restart();
        break;
      default: break;
    }
  }

  // Las acciones de arriba pueden haber cambiado de pantalla: tomar la hora de nuevo
  // (con la hora vieja, millis() - screenStart da "negativo" y salta enseguida).
  now = millis();
  if (current != SCR_PHOTO) {
    // Rotacion automatica
    if (now - screenStart >= screenSecs(current) * 1000UL) nextScreen();
    if (cfg.ssEnabled && now - lastInteraction >= cfg.ssAfterMin * 60000UL) enterScreensaver("tiempo sin uso");
  }

  // WiFi caido: reintentar cada 30 s
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWifiTry > 30000) {
      lastWifiTry = now;
      Serial.println("[wifi] reconectando...");
      WiFi.reconnect();
    }
  } else {
    webBegin();                                 // por si arranco sin WiFi
    if (now - lastWeather >= weatherEvery) refreshWeather();
    if (cfg.newsEnabled && now - lastNews >= newsEvery) refreshNews();
  }

  if (now - lastBright > 60000) { lastBright = now; updateBrightness(); }

  screenTick(current);
  delay(2);
}
