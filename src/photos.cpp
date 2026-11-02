#include "photos.h"
#include "config.h"
#include "screens.h"
#include "settings.h"
#include <FS.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define MAX_PHOTOS     60
#define MAX_JPG_BYTES  (90 * 1024)   // tope para fotos descargadas

static String photoNames[MAX_PHOTOS];
static int nPhotos = 0, nextPhoto = 0;
static bool fsOk = false;

static bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bmp) {
  if (y >= tft.height()) return false;     // lo que queda esta fuera de pantalla
  tft.pushImage(x, y, w, h, bmp);          // TFT_eSPI recorta lo que sobresale
  return true;
}

static bool isJpg(String n) {
  n.toLowerCase();
  return n.endsWith(".jpg") || n.endsWith(".jpeg");
}

static void scanDir(const char* dir) {
  File root = LittleFS.open(dir);
  if (!root || !root.isDirectory()) return;
  File f = root.openNextFile();
  while (f && nPhotos < MAX_PHOTOS) {
    String name = f.name();
    if (!f.isDirectory() && isJpg(name)) {
      if (!name.startsWith("/")) name = String(dir) + (strcmp(dir, "/") ? "/" : "") + name;
      photoNames[nPhotos++] = name;
    }
    f = root.openNextFile();
  }
}

void photosBegin() {
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tftOutput);
  fsOk = LittleFS.begin(true);
  photosRescan();
  Serial.printf("[fotos] LittleFS %s, %d fotos propias\n", fsOk ? "ok" : "ERROR", nPhotos);
}

void photosRescan() {
  nPhotos = 0;
  nextPhoto = 0;
  if (!fsOk) return;
  scanDir("/");
  scanDir("/photos");
}

int localPhotoCount() { return nPhotos; }

// Elige la escala (1, 2, 4 u 8) que llena la pantalla recortando lo minimo,
// y devuelve la posicion para centrar la imagen.
static void placeImage(uint16_t w, uint16_t h, int& x, int& y) {
  uint8_t scale = 1;
  for (uint8_t s = 8; s >= 2; s /= 2) {
    if (w / s >= tft.width() && h / s >= tft.height()) { scale = s; break; }
  }
  TJpgDec.setJpgScale(scale);
  int sw = w / scale, sh = h / scale;
  x = (tft.width() - sw) / 2;
  y = (tft.height() - sh) / 2;
  if (sw < tft.width() || sh < tft.height()) tft.fillScreen(TFT_BLACK);
}

static bool showLocal() {
  if (!nPhotos) return false;
  const String& path = photoNames[nextPhoto];
  nextPhoto = (nextPhoto + 1) % nPhotos;
  uint16_t w = 0, h = 0;
  if (TJpgDec.getFsJpgSize(&w, &h, path, LittleFS) != JDR_OK || !w) {
    Serial.printf("[fotos] no se pudo leer %s (JPG progresivo?)\n", path.c_str());
    return false;
  }
  int x, y;
  placeImage(w, h, x, y);
  return TJpgDec.drawFsJpg(x, y, path, LittleFS) == JDR_OK;
}

// picsum.photos entrega solo JPG *progresivos*, que el ESP32 no puede decodificar.
// Por eso esas fotos se piden a traves de images.weserv.nl, que las reconvierte a
// JPG normal (baseline) de 320x170. Cada pedido usa una "semilla" al azar para
// obtener una foto distinta (y evitar que el servicio devuelva siempre la misma).
static String buildPhotoUrl() {
  String u = cfg.photoUrl;
  if (u.indexOf("picsum.photos") >= 0 && u.indexOf("weserv") < 0) {
    return "https://images.weserv.nl/?url=picsum.photos/seed/" + String(esp_random()) +
           "/640/340&w=320&h=170&fit=cover&output=jpg&q=80";
  }
  u.replace("{r}", String(esp_random()));     // "{r}" en la URL = numero al azar
  return u;
}

static bool isProgressive(const uint8_t* b, size_t len) {
  for (size_t i = 0; i + 1 < len; i++)
    if (b[i] == 0xFF && b[i + 1] == 0xC2) return true;   // marcador SOF2
  return false;
}

static bool showOnline() {
  if (WiFi.status() != WL_CONNECTED) return false;
  String url = buildPhotoUrl();
  Serial.printf("[fotos] descargando %s\n", url.c_str());
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[fotos] HTTP %d\n", code);
    http.end();
    return false;
  }
  int expected = http.getSize();
  size_t cap = (expected > 0 && expected <= MAX_JPG_BYTES) ? expected : MAX_JPG_BYTES;
  uint8_t* buf = (uint8_t*)malloc(cap);
  if (!buf) { http.end(); Serial.println("[fotos] sin memoria"); return false; }

  Stream& s = http.getStream();
  size_t len = 0;
  unsigned long deadline = millis() + 20000;
  while (len < cap && millis() < deadline && (client.connected() || client.available())) {
    int avail = s.available();
    if (avail <= 0) { delay(2); continue; }
    len += s.readBytes(buf + len, min((size_t)avail, cap - len));
  }
  http.end();

  bool ok = false;
  uint16_t w = 0, h = 0;
  if (len > 0 && TJpgDec.getJpgSize(&w, &h, buf, len) == JDR_OK && w) {
    int x, y;
    placeImage(w, h, x, y);
    ok = TJpgDec.drawJpg(x, y, buf, len) == JDR_OK;
  }
  if (!ok) Serial.printf("[fotos] foto de internet invalida (%u bytes%s)\n", (unsigned)len,
                         isProgressive(buf, len) ? ", JPG progresivo: no soportado" : "");
  free(buf);
  return ok;
}

bool showNextPhoto() {
  switch (cfg.photoSource) {
    case 0:  return showLocal();
    case 1:  return showOnline();
    default: return nPhotos ? showLocal() : showOnline();
  }
}
