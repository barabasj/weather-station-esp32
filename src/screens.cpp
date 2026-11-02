#include "screens.h"
#include "config.h"
#include "weather.h"
#include "news.h"
#include "icons.h"
#include "photos.h"
#include "fonts.h"
#include "settings.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <time.h>
#include <vector>

TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite iconSpr   = TFT_eSprite(&tft);
static TFT_eSprite tickerSpr = TFT_eSprite(&tft);
static bool iconSprOk = false, tickerSprOk = false;

static constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ---------------------------------------------------------------- paleta
static const uint16_t BG       = rgb(8, 12, 26);
static const uint16_t BG2      = rgb(20, 28, 52);
static const uint16_t FG       = rgb(235, 240, 250);
static const uint16_t DIM      = rgb(130, 140, 165);
static const uint16_t ACCENT   = rgb(90, 200, 250);
static const uint16_t WARM     = rgb(255, 150, 60);
static const uint16_t COLD     = rgb(110, 175, 255);
static const uint16_t RAINC    = rgb(80, 150, 240);
static const uint16_t RAINBAR  = rgb(28, 62, 125);
static const uint16_t GRID     = rgb(38, 46, 72);
static const uint16_t SEPC     = rgb(80, 92, 125);   // separador de noticias
static const uint16_t ALERTC   = rgb(255, 190, 0);
static const uint16_t ALERTBG  = rgb(150, 30, 20);

static const int W = 320, H = 170;
static const int TICKER_Y = 147, TICKER_H = 23;

static const char* DIAS[]  = {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
static const char* DIAS3[] = {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"};
static const char* MESES[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio",
                              "agosto", "septiembre", "octubre", "noviembre", "diciembre"};

// ---------------------------------------------------------------- fuentes
// FONT_S 13 px, FONT_M 17 px, FONT_N 21 px (con tildes y eñes, ver fonts.h).
// Los numeros grandes usan las fuentes internas 4, 7 y 8 de TFT_eSPI.
static const uint8_t* curFont = nullptr;

static void useFont(const uint8_t* f) {
  if (curFont == f) return;
  if (curFont) tft.unloadFont();
  if (f) tft.loadFont(f);
  curFont = f;
}

static int tw(const String& s, const uint8_t* f) {
  useFont(f);
  return tft.textWidth(s);
}

static int text(const String& s, int x, int y, const uint8_t* f, uint16_t fg,
                uint16_t bg = BG, uint8_t datum = TL_DATUM) {
  useFont(f);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(datum);
  int w = tft.drawString(s, x, y);
  tft.setTextDatum(TL_DATUM);
  return w;
}

// Quita el ultimo caracter UTF-8
static void popChar(String& s) {
  while (s.length() && ((uint8_t)s[s.length() - 1] & 0xC0) == 0x80) s.remove(s.length() - 1);
  if (s.length()) s.remove(s.length() - 1);
}

// Recorta con "…" para que entre en maxW
static String ellipsize(String s, const uint8_t* f, int maxW) {
  if (tw(s, f) <= maxW) return s;
  while (s.length() && tw(s + "…", f) > maxW) popChar(s);
  s.trim();
  return s + "…";
}

static int roundi(float v) { return (int)lroundf(v); }
static String deg(float c) { return String(roundi(toUnit(c))) + "°"; }

// Temperatura con numeros grandes (fuente 7, sin "°": el grado se dibuja)
static void drawBigTemp(float c, int x, int y, uint16_t col) {
  useFont(nullptr);
  float v = toUnit(c);
  String s = String(v, 1);
  tft.setTextColor(col, BG);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(s, x, y, 7);
  int w = tft.textWidth(s, 7);
  tft.drawCircle(x + w + 7, y + 6, 5, col);
  tft.drawCircle(x + w + 7, y + 6, 4, col);
  tft.drawString(cfg.fahrenheit ? "F" : "C", x + w + 15, y, 4);
}

// Triangulo de advertencia
static void warnIcon(int cx, int cy, int s, uint16_t col, uint16_t bg) {
  tft.fillTriangle(cx, cy - s / 2, cx - s * 6 / 10, cy + s / 2, cx + s * 6 / 10, cy + s / 2, col);
  int bw = max(2, s / 8);
  tft.fillRect(cx - bw / 2, cy - s / 5, bw, s * 4 / 10, bg);
  tft.fillRect(cx - bw / 2, cy + s * 3 / 10, bw, bw, bg);
}

static void header(const String& left, const String& right, uint16_t bg = BG2,
                   uint16_t col = ACCENT) {
  tft.fillRect(0, 0, W, 20, bg);
  int w = text(left, 6, 1, FONT_S, col, bg);
  if (bg == BG2 && activeAlerts() > 0) warnIcon(6 + w + 12, 10, 14, ALERTC, bg);
  if (right.length()) text(right, W - 6, 1, FONT_S, bg == BG2 ? DIM : FG, bg, TR_DATUM);
}

static String cityName() { return cfg.cities[cityIdx].name; }

static String updatedStr() {
  if (!weather.updated) return "";
  struct tm t;
  localtime_r(&weather.updated, &t);
  char b[16];
  strftime(b, sizeof(b), "act. %H:%M", &t);
  return b;
}

static void noData() {
  text("Sin datos del clima todavía", W / 2, H / 2 - 10, FONT_M, DIM, BG, TC_DATUM);
}

// Corta un texto en lineas que entren en maxW; si sobra, la ultima lleva "…"
static int wrapText(const String& txt, const uint8_t* f, int maxW, String* lines, int maxLines) {
  int n = 0;
  String line, word;
  bool overflow = false;
  for (size_t i = 0; i <= txt.length() && !overflow; i++) {
    char c = i < txt.length() ? txt[i] : ' ';
    if (c != ' ') { word += c; continue; }
    if (!word.length()) continue;
    String test = line.length() ? line + " " + word : word;
    if (tw(test, f) <= maxW || !line.length()) line = test;
    else if (n < maxLines - 1) { lines[n++] = line; line = word; }
    else { line = test; overflow = true; }
    word = "";
  }
  if (line.length()) lines[n++] = overflow || tw(line, f) > maxW ? ellipsize(line, f, maxW) : line;
  return n;
}

// ---------------------------------------------------------------- estado
static uint32_t frame = 0;
static unsigned long lastAnim = 0, lastTicker = 0, lastPhoto = 0;
static int lastSec = -1, lastMin = -1, lastDay = -1, photoMin = -1;
static int newsIdx = 0;
static int segX[NEWS_MAX_ITEMS], segW[NEWS_MAX_ITEMS];
static int tickerTotal = 0, tickerX = W, sepW = 0;
static const char* SEP = "•";

int activeAlerts() {
  String tmp[MAX_ALERTS];
  return buildAlerts(weather, tmp, MAX_ALERTS);
}

static void buildNewsLines();

void newsChanged() {
  if (newsIdx >= headlineCount) newsIdx = 0;
  if (!tickerSprOk) return;
  buildNewsLines();
  int x = 0;
  sepW = tickerSpr.textWidth(SEP) + 36;
  for (int i = 0; i < headlineCount; i++) {
    segX[i] = x;
    segW[i] = tickerSpr.textWidth(headlines[i]);
    x += segW[i] + sepW;
  }
  tickerTotal = x;
}

void screensBegin() {
  tft.setTextWrap(false, false);
  iconSpr.setColorDepth(16);
  iconSprOk = iconSpr.createSprite(120, 94) != nullptr;
  tickerSpr.setColorDepth(16);
  tickerSprOk = tickerSpr.createSprite(W, TICKER_H) != nullptr;
  if (tickerSprOk) {
    tickerSpr.loadFont(FONT_M);
    tickerSpr.setTextWrap(false, false);
  }
  Serial.printf("[pantalla] sprites: icono=%d cinta=%d  heap libre=%u\n",
                iconSprOk, tickerSprOk, ESP.getFreeHeap());
}

void screenMessage(const char* l1, const char* l2, const char* l3) {
  tft.fillScreen(BG);
  drawWeatherIcon(tft, W / 2, 50, 22, ICON_PARTLY, true, 0, BG);
  text(l1, W / 2, 92, FONT_N, FG, BG, TC_DATUM);
  if (l2) text(l2, W / 2, 122, FONT_S, DIM, BG, TC_DATUM);
  if (l3) text(l3, W / 2, 142, FONT_S, ACCENT, BG, TC_DATUM);
}

// ================================================================ RELOJ
static bool tickerActive() {
  return cfg.newsTicker && cfg.newsEnabled && tickerSprOk && headlineCount > 0;
}

static void clockWeatherRow() {
  tft.fillRect(0, 108, W, 38, BG);
  if (!weather.valid) return;
  drawWeatherIcon(tft, 22, 127, 11, iconForCode(weather.code), weather.isDay, 0, BG);
  int x = 42;
  x += text(deg(weather.temp), x, 113, FONT_N, FG) + 12;

  String l1 = descForCode(weather.code);
  if (cfg.cityCount > 1) l1 = cityName() + " · " + l1;
  text(ellipsize(l1, FONT_S, W - x - 4), x, 108, FONT_S, FG);

  int x2 = x;
  x2 += text("Máx ", x2, 126, FONT_S, DIM);
  x2 += text(deg(weather.days[0].tMax), x2, 126, FONT_S, WARM) + 10;
  x2 += text("Mín ", x2, 126, FONT_S, DIM);
  text(deg(weather.days[0].tMin), x2, 126, FONT_S, COLD);
}

static void clockBottomStatic() {
  tft.fillRect(0, TICKER_Y, W, TICKER_H, BG2);
  if (!weather.valid) return;
  String s = String("Sale el sol ") + weather.sunrise + "   ·   Se pone " + weather.sunset;
  text(s, W / 2, TICKER_Y + 3, FONT_S, DIM, BG2, TC_DATUM);
}

static void tickerFrame() {
  tickerX -= 2;
  if (tickerX <= -tickerTotal) tickerX += tickerTotal;
  tickerSpr.fillSprite(BG2);
  tickerSpr.setTextColor(FG, BG2);
  for (int rep = 0; rep < 2; rep++) {
    int base = tickerX + rep * tickerTotal;
    for (int i = 0; i < headlineCount; i++) {
      int sx = base + segX[i];
      if (sx >= W) break;
      if (sx + segW[i] + sepW < 0) continue;
      tickerSpr.setTextColor(FG, BG2);
      tickerSpr.drawString(headlines[i], sx, 2);
      tickerSpr.setTextColor(ACCENT, BG2);
      tickerSpr.drawString(SEP, sx + segW[i] + 18, 2);
    }
  }
  tickerSpr.drawFastHLine(0, 0, W, ACCENT);
  tickerSpr.pushSprite(0, TICKER_Y);
}

static void clockTick() {
  struct tm t;
  bool ok = getLocalTime(&t, 0);
  useFont(nullptr);

  if (!ok) {
    if (lastMin != -2) {
      tft.setTextColor(DIM, BG);
      tft.setTextDatum(TC_DATUM);
      tft.drawString("--:--", W / 2, 2, 8);
      tft.setTextDatum(TL_DATUM);
      lastMin = -2;
    }
  } else {
    int wTime = tft.textWidth("88:88", 8);
    int wSec  = tft.textWidth("88", 4);
    int x0 = (W - (wTime + 8 + wSec)) / 2;

    if (t.tm_min != lastMin) {
      useFont(nullptr);
      int hh = t.tm_hour;
      if (!cfg.h24) { hh = hh % 12; if (hh == 0) hh = 12; }
      char b[8];
      snprintf(b, sizeof(b), cfg.h24 ? "%02d:%02d" : "%d:%02d", hh, t.tm_min);
      tft.fillRect(0, 0, x0 + wTime + 2, 78, BG);
      tft.setTextColor(FG, BG);
      tft.setTextDatum(TR_DATUM);
      tft.drawString(b, x0 + wTime, 2, 8);
      tft.setTextDatum(TL_DATUM);
      if (!cfg.h24) {
        tft.setTextColor(DIM, BG);
        tft.setTextPadding(wSec);
        tft.drawString(t.tm_hour < 12 ? "am" : "pm", x0 + wTime + 8, 10, 4);
        tft.setTextPadding(0);
      }
      // aviso de alerta arriba a la derecha
      tft.fillRect(W - 28, 0, 28, 26, BG);
      if (activeAlerts() > 0) warnIcon(W - 14, 13, 18, ALERTC, BG);
      lastMin = t.tm_min;
    }
    if (t.tm_sec != lastSec) {
      useFont(nullptr);
      char b[4];
      snprintf(b, sizeof(b), "%02d", t.tm_sec);
      tft.setTextColor(ACCENT, BG);
      tft.setTextPadding(wSec);
      tft.drawString(b, x0 + wTime + 8, 50, 4);
      tft.setTextPadding(0);
      lastSec = t.tm_sec;
    }
    if (t.tm_mday != lastDay) {
      char b[48];
      snprintf(b, sizeof(b), "%s %d de %s", DIAS[t.tm_wday], t.tm_mday, MESES[t.tm_mon]);
      tft.fillRect(0, 78, W, 30, BG);
      text(b, W / 2, 79, FONT_N, WARM, BG, TC_DATUM);
      lastDay = t.tm_mday;
    }
  }

  if (tickerActive() && millis() - lastTicker >= 25) {
    lastTicker = millis();
    tickerFrame();
  }
}

static void drawClock() {
  tft.fillScreen(BG);
  lastSec = lastMin = lastDay = -1;
  clockWeatherRow();
  if (!tickerActive()) clockBottomStatic();
  clockTick();
}

// ================================================================ ACTUAL
static void currentIconFrame() {
  IconType ic = iconForCode(weather.code);
  if (iconSprOk) {
    iconSpr.fillSprite(BG);
    drawWeatherIcon(iconSpr, 60, 50, 32, ic, weather.isDay, frame, BG);
    iconSpr.pushSprite(4, 24);
  } else {
    drawWeatherIcon(tft, 64, 74, 32, ic, weather.isDay, 0, BG);
  }
}

static void infoCell(int x, int y, const char* label, const String& value) {
  text(label, x, y, FONT_S, DIM);
  text(value, x, y + 17, FONT_M, FG);
}

static void drawCurrent() {
  tft.fillScreen(BG);
  header(cityName(), updatedStr());
  if (!weather.valid) { noData(); return; }

  currentIconFrame();

  const int x = 132, maxW = W - x - 4;
  drawBigTemp(weather.temp, x, 25, FG);

  String d = descForCode(weather.code);
  const uint8_t* f = tw(d, FONT_N) <= maxW ? FONT_N : (tw(d, FONT_M) <= maxW ? FONT_M : FONT_S);
  text(ellipsize(d, f, maxW), x, 76, f, ACCENT);

  int xx = x;
  xx += text("Sensación ", xx, 104, FONT_S, DIM);
  xx += text(deg(weather.feels), xx, 104, FONT_S, FG);
  xx += text(" · Nubes ", xx, 104, FONT_S, DIM);
  text(String(weather.cloud) + "%", xx, 104, FONT_S, FG);

  tft.drawFastHLine(6, 125, W - 12, GRID);
  infoCell(8,   128, "Humedad", String(weather.humidity) + " %");
  infoCell(86,  128, "Viento",  String(roundi(weather.wind)) + " km/h " + windDirName(weather.windDir));
  infoCell(206, 128, "Presión", String(roundi(weather.pressure)) + " hPa");
}

// ================================================================ ALERTA
static void drawAlert() {
  String al[MAX_ALERTS];
  int n = buildAlerts(weather, al, MAX_ALERTS);
  tft.fillScreen(BG);
  header("Alerta meteorológica", cityName(), ALERTBG, FG);
  warnIcon(52, 92, 70, ALERTC, BG);

  const int x = 104, maxW = W - x - 6;
  int lines = 0;
  String tmp[2];
  for (int i = 0; i < n; i++) lines += wrapText(al[i], FONT_M, maxW, tmp, 2);
  int y = 32 + max(0, (4 - lines)) * 12;
  for (int i = 0; i < n && y < H - 20; i++) {
    int k = wrapText(al[i], FONT_M, maxW, tmp, 2);
    for (int j = 0; j < k; j++) { text(tmp[j], x, y, FONT_M, FG); y += 22; }
    y += 6;
  }
}

// ================================================================ PRONOSTICO
static void drawForecast() {
  tft.fillScreen(BG);
  header("Pronóstico 5 días", cityName());
  if (!weather.valid) { noData(); return; }

  struct tm t;
  bool ok = getLocalTime(&t, 0);
  const int colW = W / FORECAST_DAYS;

  for (int i = 0; i < FORECAST_DAYS; i++) {
    const DayForecast& d = weather.days[i];
    int cx = colW * i + colW / 2;
    if (i) tft.drawFastVLine(colW * i, 26, H - 32, GRID);

    const char* name = i == 0 ? "Hoy" : (ok ? DIAS3[(t.tm_wday + i) % 7] : "-");
    text(name, cx, 23, FONT_M, i == 0 ? ACCENT : FG, BG, TC_DATUM);

    drawWeatherIcon(tft, cx, 63, 14, iconForCode(d.code), true, 0, BG);

    text(deg(d.tMax), cx, 84, FONT_N, WARM, BG, TC_DATUM);
    text(deg(d.tMin), cx, 108, FONT_M, COLD, BG, TC_DATUM);

    // probabilidad de lluvia con una gotita
    String p = String(d.rainProb) + "%";
    int pw = tw(p, FONT_S);
    int px = cx - (pw + 9) / 2;
    tft.fillCircle(px + 3, 141, 3, RAINC);
    tft.fillTriangle(px, 140, px + 6, 140, px + 3, 134, RAINC);
    text(p, px + 9, 132, FONT_S, RAINC);

    text("UV " + String(roundi(d.uvMax)), cx, 150, FONT_S, DIM, BG, TC_DATUM);
  }
}

// ================================================================ POR HORA
static void drawHourly() {
  tft.fillScreen(BG);
  header("Próximas 24 horas", cityName());
  if (!weather.valid || weather.nHours < 2) { noData(); return; }

  const int n = weather.nHours;
  const int x0 = 30, x1 = W - 8, yTop = 50, yBot = 136;
  const float colW = (float)(x1 - x0) / n;

  float tmin = 999, tmax = -999;
  for (int i = 0; i < n; i++) {
    tmin = min(tmin, weather.hours[i].temp);
    tmax = max(tmax, weather.hours[i].temp);
  }
  if (tmax - tmin < 4) { float m = (tmax + tmin) / 2; tmin = m - 2; tmax = m + 2; }
  tmin -= 1; tmax += 1;
  auto yOf = [&](float v) { return yBot - (v - tmin) / (tmax - tmin) * (yBot - yTop); };

  // grilla + escala
  for (int k = 0; k <= 2; k++) {
    int y = yTop + k * (yBot - yTop) / 2;
    tft.drawFastHLine(x0, y, x1 - x0, GRID);
    text(deg(tmax - k * (tmax - tmin) / 2), x0 - 3, y - 8, FONT_S, DIM, BG, TR_DATUM);
  }

  // barras de probabilidad de lluvia
  for (int i = 0; i < n; i++) {
    int h = weather.hours[i].rainProb * (yBot - yTop) / 100;
    if (h > 0) tft.fillRect(x0 + i * colW + 1, yBot - h, max(1, (int)colW - 2), h, RAINBAR);
  }

  // linea de temperatura
  for (int i = 1; i < n; i++) {
    float xa = x0 + (i - 0.5f) * colW, xb = x0 + (i + 0.5f) * colW;
    tft.drawWideLine(xa, yOf(weather.hours[i - 1].temp), xb, yOf(weather.hours[i].temp),
                     2.5f, WARM, BG);
  }

  // horas e iconos
  for (int i = 0; i < n; i++) {
    int cx = x0 + (i + 0.5f) * colW;
    if (i % 3 == 0) text(String(weather.hours[i].hour) + "h", cx, yBot + 2, FONT_S, DIM, BG, TC_DATUM);
    if (i % 4 == 0) {
      const HourForecast& hf = weather.hours[i];
      drawWeatherIcon(tft, cx, 34, 6, iconForCode(hf.code), hf.isDay, 0, BG);
    }
  }

  // leyenda
  tft.fillRect(x0, 160, 12, 3, WARM);
  text("Temperatura", x0 + 16, 153, FONT_S, DIM);
  tft.fillRect(x0 + 116, 156, 9, 9, RAINBAR);
  text("Prob. de lluvia", x0 + 130, 153, FONT_S, DIM);
}

// ================================================================ NOTICIAS
// Lista con todos los titulares completos (fuente de 17 px), separados por una
// linea punteada, que se desplaza hacia arriba de forma continua.
// Se dibuja en franjas de 23 px usando el sprite de la cinta (sin parpadeo y
// sin memoria extra).
struct NewsLine {
  String  text;
  String  extra;        // medio, en celeste, al final de la ultima linea
  uint16_t color;
  int16_t y;
  bool    sep;
};
static std::vector<NewsLine> newsLines;
static int   newsTotalH = 0;
static float newsOffset = 0;
static unsigned long newsHoldUntil = 0, newsLastStep = 0;
static bool  newsEndDone = false;     // ya se hizo la pausa del final en esta pasada
static const int NEWS_TOP   = 22;     // debajo del encabezado
static const int LINE_H     = TICKER_H;
static const int SEP_H      = 13;
static const int NEWS_SPEED = 22;     // pixeles por segundo
static const int NEWS_PAUSE = 2000;   // pausa al entrar (ms)
static const int NEWS_END_PAUSE = 4000;   // pausa con la ultima noticia en pantalla (ms)

static void buildNewsLines() {
  newsLines.clear();
  const int x0 = 14, maxW = W - x0 - 10;
  int y = 4;
  for (int i = 0; i < headlineCount; i++) {
    std::vector<String> ls;
    String line, word;
    const String& t = headlines[i];
    for (size_t k = 0; k <= t.length(); k++) {
      char c = k < t.length() ? t[k] : ' ';
      if (c != ' ') { word += c; continue; }
      if (!word.length()) continue;
      String test = line.length() ? line + " " + word : word;
      if (tickerSpr.textWidth(test) <= maxW || !line.length()) line = test;
      else { ls.push_back(line); line = word; }
      word = "";
    }
    if (line.length()) ls.push_back(line);
    for (auto& l : ls) { newsLines.push_back({l, "", FG, (int16_t)y, false}); y += LINE_H; }

    if (sources[i].length() && !newsLines.empty()) {
      NewsLine& last = newsLines.back();
      String tag = "  · " + sources[i];
      if (tickerSpr.textWidth(last.text + tag) <= maxW) last.extra = tag;
      else { newsLines.push_back({sources[i], "", ACCENT, (int16_t)y, false}); y += LINE_H; }
    }
    newsLines.push_back({"", "", DIM, (int16_t)y, true});
    y += SEP_H;
  }
  newsTotalH = y;
  if (newsTotalH > 0 && newsOffset >= newsTotalH) newsOffset = 0;
}

// Segundos que tarda en pasar toda la lista (para la duracion de la pantalla)
int newsCycleSecs() {
  int area = H - NEWS_TOP;
  if (!tickerSprOk || newsTotalH <= area) return 0;
  // pausa inicial + recorrido hasta que la ultima noticia queda entera + pausa final
  long ms = NEWS_PAUSE + NEWS_END_PAUSE + (newsTotalH - area) * 1000L / NEWS_SPEED;
  return (ms + 999) / 1000;
}

static void newsRender() {
  const int area = H - NEWS_TOP;
  const bool scrolling = newsTotalH > area;
  const int off = scrolling ? (int)newsOffset : 0;
  for (int bTop = NEWS_TOP; bTop < H; bTop += LINE_H) {
    tickerSpr.fillSprite(BG);
    for (int rep = 0; rep < (scrolling ? 2 : 1); rep++) {
      int base = NEWS_TOP - off + rep * newsTotalH;
      for (const NewsLine& L : newsLines) {
        int ly = base + L.y, lh = L.sep ? SEP_H : LINE_H;
        if (ly >= bTop + LINE_H || ly + lh <= bTop) continue;
        int sy = ly - bTop;
        if (L.sep) {
          for (int x = 14; x < W - 14; x += 9) tickerSpr.drawFastHLine(x, sy + SEP_H / 2, 5, SEPC);
        } else {
          tickerSpr.setTextColor(L.color, BG);
          int w = tickerSpr.drawString(L.text, 14, sy);
          if (L.extra.length()) {
            tickerSpr.setTextColor(ACCENT, BG);
            tickerSpr.drawString(L.extra, 14 + w, sy);
          }
        }
      }
    }
    tickerSpr.pushSprite(0, bTop);
  }
}

static void drawNews() {
  tft.fillScreen(BG);
  String right = cfg.newsName;
  if (headlineCount) right += "  ·  " + String(headlineCount) + " titulares";
  header("Noticias", right);
  newsOffset = 0;
  newsEndDone = false;
  newsLastStep = millis();
  newsHoldUntil = newsLastStep + NEWS_PAUSE;
  if (headlineCount == 0) {
    text("Sin noticias por ahora", W / 2, H / 2 - 10, FONT_M, DIM, BG, TC_DATUM);
    return;
  }
  if (!tickerSprOk) {          // sin memoria para el sprite: lista fija
    int y = NEWS_TOP + 4;
    for (int i = 0; i < headlineCount && y < H - 20; i++) {
      text(ellipsize(headlines[i], FONT_M, W - 24), 14, y, FONT_M, FG);
      y += LINE_H;
    }
    return;
  }
  newsRender();
}

static void newsTick() {
  if (!tickerSprOk || newsTotalH <= H - NEWS_TOP) return;
  unsigned long now = millis();
  if ((long)(now - newsHoldUntil) < 0) { newsLastStep = now; return; }   // en pausa
  if (now - newsLastStep < 40) return;
  float next = newsOffset + NEWS_SPEED * (now - newsLastStep) / 1000.0f;
  newsLastStep = now;
  // Frenar cuando la ultima noticia queda entera abajo, para poder leerla
  const int endPos = newsTotalH - (H - NEWS_TOP);
  if (!newsEndDone && newsOffset < endPos && next >= endPos) {
    next = endPos;
    newsEndDone = true;
    newsHoldUntil = now + NEWS_END_PAUSE + 1500;   // +margen: la pantalla cambia durante la pausa
  }
  if (next >= newsTotalH) { next -= newsTotalH; newsEndDone = false; }
  newsOffset = next;
  newsRender();
}

// ================================================================ FOTOS
// Abajo a la izquierda: icono del tiempo y temperatura actual
static void photoWeather() {
  if (!weather.valid) return;
  String t = deg(weather.temp);
  int w = 36 + tw(t, FONT_N) + 10;
  tft.fillRoundRect(6, H - 34, w, 28, 8, TFT_BLACK);
  drawWeatherIcon(tft, 6 + 19, H - 20, 8, iconForCode(weather.code), weather.isDay, 0, TFT_BLACK);
  text(t, 6 + 36, H - 31, FONT_N, TFT_WHITE, TFT_BLACK);
}

// Abajo a la derecha: hora (y clima a la izquierda)
static void photoClock() {
  if (!cfg.photoClock) return;
  photoWeather();
  struct tm t;
  if (!getLocalTime(&t, 0)) return;
  char b[8];
  snprintf(b, sizeof(b), "%02d:%02d", t.tm_hour, t.tm_min);
  int w = tw(b, FONT_N) + 16;
  tft.fillRoundRect(W - w - 6, H - 34, w, 28, 8, TFT_BLACK);
  text(b, W - 6 - w / 2, H - 31, FONT_N, TFT_WHITE, TFT_BLACK, TC_DATUM);
  photoMin = t.tm_min;
}

static void drawPhoto() {
  if (!showNextPhoto()) {
    // sin fotos disponibles: reloj simple sobre fondo negro
    tft.fillScreen(TFT_BLACK);
    text("Sin fotos disponibles", W / 2, 20, FONT_S, DIM, TFT_BLACK, TC_DATUM);
  }
  photoClock();
  // el tiempo de cada foto se cuenta desde que ya esta en pantalla
  // (no desde que empezo la descarga, que puede tardar varios segundos)
  lastPhoto = millis();
}

// ================================================================ INFO
static void infoLine(int y, const char* label, const String& value, uint16_t col = FG) {
  text(label, 10, y, FONT_M, DIM);
  text(ellipsize(value, FONT_M, W - 112), 104, y, FONT_M, col);
}

static void drawInfo() {
  tft.fillScreen(BG);
  header("Información", "versión " FW_VERSION);
  if (WiFi.status() == WL_CONNECTED) {
    infoLine(28, "Red WiFi", WiFi.SSID() + "  (" + String(WiFi.RSSI()) + " dBm)");
    infoLine(52, "Página", "http://" + WiFi.localIP().toString(), ACCENT);
    infoLine(76, "o también", "http://estacion.local", ACCENT);
  } else {
    infoLine(28, "Red WiFi", "sin conexión", ALERTC);
  }
  infoLine(100, "Fotos", String(localPhotoCount()) + " propias  ·  " +
           String((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024) + " KB libres");
  infoLine(124, "Memoria", String(ESP.getFreeHeap() / 1024) + " KB libres");
  text("Abrí la página desde el navegador del celular o la PC", W / 2, 150, FONT_S, DIM, BG, TC_DATUM);
}

void screenProgress(const char* title, int percent) {
  static int last = -1;
  if (percent <= 0 || last < 0 || percent < last) {
    tft.fillScreen(BG);
    text(title, W / 2, 50, FONT_N, FG, BG, TC_DATUM);
    text("No desconectes la placa", W / 2, 140, FONT_S, DIM, BG, TC_DATUM);
    tft.drawRoundRect(30, 92, W - 60, 18, 6, DIM);
  }
  int w = (W - 64) * constrain(percent, 0, 100) / 100;
  if (w > 0) tft.fillRoundRect(32, 94, w, 14, 5, ACCENT);
  tft.fillRect(W / 2 - 30, 114, 60, 22, BG);
  text(String(percent) + "%", W / 2, 114, FONT_M, FG, BG, TC_DATUM);
  last = percent >= 100 ? -1 : percent;
}

// ================================================================ API
void screenDraw(ScreenId s) {
  frame = 0;
  switch (s) {
    case SCR_CLOCK:    drawClock();    break;
    case SCR_CURRENT:  drawCurrent();  break;
    case SCR_ALERT:    drawAlert();    break;
    case SCR_FORECAST: drawForecast(); break;
    case SCR_HOURLY:   drawHourly();   break;
    case SCR_NEWS:     drawNews();     break;
    case SCR_PHOTO:    drawPhoto();    break;
    case SCR_INFO:     drawInfo();     break;
    default: break;
  }
}

void screenTick(ScreenId s) {
  unsigned long now = millis();
  switch (s) {
    case SCR_CLOCK:
      clockTick();
      break;
    case SCR_CURRENT:
      if (weather.valid && iconSprOk && now - lastAnim >= 60) {
        lastAnim = now;
        frame++;
        currentIconFrame();
      }
      break;
    case SCR_NEWS:
      newsTick();
      break;
    case SCR_PHOTO: {
      if (now - lastPhoto >= cfg.photoSecs * 1000UL) { drawPhoto(); break; }
      struct tm t;
      if (cfg.photoClock && getLocalTime(&t, 0) && t.tm_min != photoMin) photoClock();
      break;
    }
    default:
      break;
  }
}
