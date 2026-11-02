#include "news.h"
#include "settings.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

String headlines[NEWS_MAX_ITEMS];
String sources[NEWS_MAX_ITEMS];
int    headlineCount = 0;

// ---------------------------------------------------------------- texto
static void appendUtf8(String& s, uint32_t cp) {
  if (cp < 0x80) s += (char)cp;
  else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
  else if (cp < 0x10000) {
    s += (char)(0xE0 | (cp >> 12));
    s += (char)(0x80 | ((cp >> 6) & 0x3F));
    s += (char)(0x80 | (cp & 0x3F));
  }
}

// Caracteres que existen en las fuentes de fonts.h
static bool inFont(uint32_t cp) {
  if (cp >= 0x20 && cp <= 0x7E) return true;
  if (cp >= 0xA1 && cp <= 0xFF) return true;
  switch (cp) {
    case 0x2013: case 0x2014: case 0x2018: case 0x2019: case 0x201C:
    case 0x201D: case 0x2022: case 0x2026: case 0x20AC: return true;
  }
  return false;
}

// Decodifica entidades HTML (&amp; &#233; &#xE9; ...), quita CDATA y etiquetas,
// y deja solo caracteres que la pantalla puede dibujar.
static String cleanText(const String& in) {
  String s = in;
  s.replace("<![CDATA[", "");
  s.replace("]]>", "");

  // 1) entidades -> UTF-8, y fuera etiquetas <...>
  String t;
  t.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '<') {                       // etiqueta HTML dentro del titulo
      int e = s.indexOf('>', i);
      if (e > 0) { i = e; continue; }
    }
    if (c == '&') {
      int e = s.indexOf(';', i);
      if (e > 0 && e - (int)i <= 8) {
        String ent = s.substring(i + 1, e);
        uint32_t cp = 0;
        if (ent.startsWith("#x") || ent.startsWith("#X")) cp = strtoul(ent.c_str() + 2, nullptr, 16);
        else if (ent.startsWith("#")) cp = strtoul(ent.c_str() + 1, nullptr, 10);
        else if (ent == "amp") cp = '&';
        else if (ent == "quot") cp = '"';
        else if (ent == "apos") cp = '\'';
        else if (ent == "lt") cp = '<';
        else if (ent == "gt") cp = '>';
        else if (ent == "nbsp") cp = ' ';
        if (cp) { appendUtf8(t, cp); i = e; continue; }
      }
    }
    t += c;
  }

  // 2) recorrer los codepoints y filtrar
  String out;
  out.reserve(t.length());
  bool lastSpace = true;
  for (size_t i = 0; i < t.length();) {
    uint8_t c = t[i];
    uint32_t cp;
    int len;
    if (c < 0x80)              { cp = c; len = 1; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
    else { i++; continue; }
    if (i + len > t.length()) break;
    for (int k = 1; k < len; k++) cp = (cp << 6) | (t[i + k] & 0x3F);
    i += len;

    if (cp == 0xA0 || cp == '\n' || cp == '\r' || cp == '\t') cp = ' ';
    if (cp == ' ') {
      if (!lastSpace) out += ' ';
      lastSpace = true;
      continue;
    }
    if (!inFont(cp)) continue;            // emojis, etc.
    appendUtf8(out, cp);
    lastSpace = false;
  }
  out.trim();
  return out;
}

// ---------------------------------------------------------------- RSS
bool fetchNews() {
  if (!cfg.newsEnabled || WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("Mozilla/5.0 (ESP32 weather station)");
  if (!http.begin(client, cfg.newsUrl)) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[noticias] HTTP %d\n", code);
    http.end();
    return false;
  }

  // Lectura en streaming: el feed puede pesar >100 KB, no lo guardamos entero.
  Stream& s = http.getStream();
  String found[NEWS_MAX_ITEMS];
  int n = 0;
  char win[8] = {0};           // ventana con los ultimos 7 caracteres
  bool inItem = false, inTitle = false;
  String cur;
  unsigned long deadline = millis() + 20000;

  while (n < NEWS_MAX_ITEMS && millis() < deadline &&
         (client.connected() || client.available())) {
    int c = s.read();
    if (c < 0) { delay(2); continue; }

    if (inTitle) {
      cur += (char)c;
      if (cur.endsWith("</title>")) {
        cur.remove(cur.length() - 8);
        String t = cleanText(cur);
        if (t.length() > 0) found[n++] = t;
        inTitle = false;
        cur = "";
      } else if (cur.length() > 600) {     // titulo demasiado largo / roto
        inTitle = false;
        cur = "";
      }
      continue;
    }

    memmove(win, win + 1, 6);
    win[6] = (char)c;
    win[7] = 0;
    if (!inItem && (strstr(win, "<item>") || strstr(win, "<item ") || strstr(win, "<entry")))
      inItem = true;                       // saltamos el titulo del canal
    else if (inItem && strcmp(win, "<title>") == 0) {
      inTitle = true;
      cur = "";
    }
  }
  http.end();

  if (n == 0) {
    Serial.println("[noticias] no se encontraron titulares");
    return false;
  }
  for (int i = 0; i < n; i++) {
    // Google Noticias agrega el medio al final: "Titulo - Clarín"
    String t = found[i], src = "";
    int p = t.lastIndexOf(" - ");
    if (p > 20 && t.length() - p - 3 <= 40) {
      src = t.substring(p + 3);
      t = t.substring(0, p);
    }
    headlines[i] = t;
    sources[i] = src;
  }
  headlineCount = n;
  Serial.printf("[noticias] %d titulares\n", n);
  return true;
}
