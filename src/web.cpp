#include "web.h"
#include "web_page.h"
#include "settings.h"
#include "weather.h"
#include "news.h"
#include "photos.h"
#include "screens.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LittleFS.h>
using fs::File;
#include <Update.h>
#include <ArduinoJson.h>

volatile bool       webSettingsChanged = false;
volatile WebCommand webCommand = CMD_NONE;

static WebServer server(80);
static bool started = false;

// ---------------------------------------------------------------- utilidades
static void sendJson(JsonDocument& doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json", out);
}

static void sendOk() { server.send(200, "application/json", "{\"ok\":true}"); }

static void sendError(const String& msg, int code = 400) {
  JsonDocument d;
  d["ok"] = false;
  d["error"] = msg;
  sendJson(d, code);
}

// Nombre de archivo seguro: letras, numeros, . _ -   (maximo 40 caracteres)
static String safeName(const String& in) {
  String n = in;
  int slash = n.lastIndexOf('/');
  if (slash >= 0) n = n.substring(slash + 1);
  String out;
  for (size_t i = 0; i < n.length() && out.length() < 40; i++) {
    char c = n[i];
    if (isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-') out += c;
    else if (c == ' ') out += '_';
  }
  return out;
}

static bool isJpgName(String n) {
  n.toLowerCase();
  return n.endsWith(".jpg") || n.endsWith(".jpeg");
}

// ---------------------------------------------------------------- ajustes
static void handleGetSettings() {
  JsonDocument doc;
  settingsToJson(doc);
  sendJson(doc);
}

static void handlePostSettings() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok)
    return sendError("JSON inválido");
  if (!settingsFromJson(doc.as<JsonVariantConst>()))
    return sendError("Revisá los datos: hace falta al menos una ciudad con coordenadas válidas");
  if (!settingsSave()) return sendError("No se pudo guardar", 500);
  webSettingsChanged = true;
  handleGetSettings();
}

static void handleDefaults() {
  settingsDefaults();
  settingsSave();
  webSettingsChanged = true;
  handleGetSettings();
}

// ---------------------------------------------------------------- estado
static void handleStatus() {
  JsonDocument d;
  d["version"] = FW_VERSION;
  d["ssid"]    = WiFi.SSID();
  d["rssi"]    = WiFi.RSSI();
  d["ip"]      = WiFi.localIP().toString();
  d["heap"]    = ESP.getFreeHeap();
  d["uptime"]  = millis() / 1000;
  d["fsTotal"] = LittleFS.totalBytes();
  d["fsUsed"]  = LittleFS.usedBytes();
  d["photos"]  = localPhotoCount();
  d["news"]    = headlineCount;
  time_t now = time(nullptr);
  char tb[24];
  strftime(tb, sizeof(tb), "%d/%m/%Y %H:%M", localtime(&now));
  d["time"] = tb;
  JsonArray cs = d["cities"].to<JsonArray>();
  for (int i = 0; i < cfg.cityCount; i++) {
    const WeatherData& w = weatherAll[i];
    JsonObject c = cs.add<JsonObject>();
    c["name"] = cfg.cities[i].name;
    c["ok"]   = w.valid;
    if (w.valid) {
      c["temp"] = roundf(w.temp * 10) / 10;
      c["desc"] = descForCode(w.code);
      String al[MAX_ALERTS];
      int n = buildAlerts(w, al, MAX_ALERTS);
      JsonArray a = c["alerts"].to<JsonArray>();
      for (int k = 0; k < n; k++) a.add(al[k]);
    }
  }
  sendJson(d);
}

// ---------------------------------------------------------------- fotos
static void handleListPhotos() {
  JsonDocument d;
  JsonArray arr = d["photos"].to<JsonArray>();
  const char* dirs[] = {"/photos", "/"};
  for (const char* dir : dirs) {
    File root = LittleFS.open(dir);
    if (!root || !root.isDirectory()) continue;
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      if (f.isDirectory() || !isJpgName(f.name())) continue;
      JsonObject o = arr.add<JsonObject>();
      String path = String(dir) + (strcmp(dir, "/") ? "/" : "") + f.name();
      o["name"] = f.name();
      o["path"] = path;
      o["size"] = f.size();
    }
  }
  d["fsTotal"] = LittleFS.totalBytes();
  d["fsUsed"]  = LittleFS.usedBytes();
  sendJson(d);
}

static String pathArg() {
  String p = server.arg("path");
  if (!p.startsWith("/") || p.indexOf("..") >= 0 || !isJpgName(p)) return "";
  return p;
}

static void handleGetPhoto() {
  String p = pathArg();
  if (!p.length() || !LittleFS.exists(p)) return sendError("No existe", 404);
  File f = LittleFS.open(p, "r");
  server.sendHeader("Cache-Control", "max-age=86400");
  server.streamFile(f, "image/jpeg");
  f.close();
}

static void handleDeletePhoto() {
  String p = pathArg();
  if (!p.length() || !LittleFS.exists(p)) return sendError("No existe", 404);
  LittleFS.remove(p);
  photosRescan();
  sendOk();
}

static File   upFile;
static String upError;

static void handleUploadPhotoDone() {
  photosRescan();
  if (upError.length()) sendError(upError);
  else sendOk();
}

static void handleUploadPhotoData() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    upError = "";
    String name = safeName(u.filename);
    if (!isJpgName(name)) { upError = "Solo se aceptan fotos .jpg"; return; }
    if (LittleFS.totalBytes() - LittleFS.usedBytes() < 40 * 1024) { upError = "No queda espacio"; return; }
    if (!LittleFS.exists("/photos")) LittleFS.mkdir("/photos");
    upFile = LittleFS.open("/photos/" + name, "w");
    if (!upFile) upError = "No se pudo crear el archivo";
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upFile && upFile.write(u.buf, u.currentSize) != u.currentSize) {
      upError = "No queda espacio";
      String n = upFile.path();
      upFile.close();
      LittleFS.remove(n);
    }
  } else if (u.status == UPLOAD_FILE_END) {
    if (upFile) upFile.close();
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    if (upFile) { String n = upFile.path(); upFile.close(); LittleFS.remove(n); }
    upError = "Subida interrumpida";
  }
}

// ---------------------------------------------------------------- firmware
static size_t fwSize = 0;
static int    fwLastPct = -1;

static void handleUpdateDone() {
  bool ok = !Update.hasError() && Update.isFinished();
  if (ok) {
    sendOk();
    screenProgress("Listo, reiniciando…", 100);
    delay(800);
    ESP.restart();
  } else {
    sendError(String("Falló la actualización: ") + Update.errorString());
    webCommand = CMD_INFO;          // volver a una pantalla normal
  }
}

static void handleUpdateData() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    fwSize = server.arg("size").toInt();
    fwLastPct = -1;
    Serial.printf("[ota] %s (%u bytes)\n", u.filename.c_str(), (unsigned)fwSize);
    screenProgress("Actualizando firmware", 0);
    if (!Update.begin(fwSize > 0 ? fwSize : UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (Update.write(u.buf, u.currentSize) != u.currentSize) Update.printError(Serial);
    if (fwSize) {
      int pct = Update.progress() * 100 / fwSize;
      if (pct >= fwLastPct + 2) { fwLastPct = pct; screenProgress("Actualizando firmware", pct); }
    }
  } else if (u.status == UPLOAD_FILE_END) {
    if (!Update.end(true)) Update.printError(Serial);
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
  }
}

// ---------------------------------------------------------------- comandos
static void handleCmd() {
  String c = server.arg("c");
  if (c == "next")        webCommand = CMD_NEXT;
  else if (c == "photos") webCommand = CMD_PHOTOS;
  else if (c == "info")   webCommand = CMD_INFO;
  else if (c == "restart") webCommand = CMD_RESTART;
  else if (c == "wifireset") webCommand = CMD_WIFI_RESET;
  else return sendError("Comando desconocido");
  sendOk();
}

// ---------------------------------------------------------------- arranque
void webBegin() {
  if (started) return;
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", WEB_PAGE); });
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/api/defaults", HTTP_POST, handleDefaults);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/photos", HTTP_GET, handleListPhotos);
  server.on("/api/photos", HTTP_POST, handleUploadPhotoDone, handleUploadPhotoData);
  server.on("/api/photo", HTTP_GET, handleGetPhoto);
  server.on("/api/photo", HTTP_DELETE, handleDeletePhoto);
  server.on("/api/cmd", HTTP_POST, handleCmd);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateData);
  server.onNotFound([]() { server.send(404, "text/plain", "No encontrado"); });
  server.begin();

  if (MDNS.begin("estacion")) MDNS.addService("http", "tcp", 80);
  started = true;
  Serial.printf("[web] http://%s  /  http://estacion.local\n", WiFi.localIP().toString().c_str());
}

void webLoop() {
  if (started) server.handleClient();
}
