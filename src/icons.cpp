#include "icons.h"
#include <math.h>

static constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static const uint16_t C_SUN      = rgb(255, 200, 40);
static const uint16_t C_SUN_CORE = rgb(255, 230, 120);
static const uint16_t C_RAY      = rgb(255, 160, 0);
static const uint16_t C_MOON     = rgb(240, 235, 200);
static const uint16_t C_CLOUD    = rgb(235, 240, 248);
static const uint16_t C_CLOUD2   = rgb(150, 160, 178);
static const uint16_t C_CLOUD_DK = rgb(95, 100, 118);
static const uint16_t C_RAIN     = rgb(80, 165, 255);
static const uint16_t C_SNOW     = rgb(245, 250, 255);
static const uint16_t C_FOG      = rgb(175, 182, 195);
static const uint16_t C_BOLT     = rgb(255, 220, 0);

// ---------------------------------------------------------------- piezas
static void cloud(TFT_eSPI& g, float x, float y, float s, uint16_t col, uint16_t bg) {
  g.fillSmoothCircle(x - 0.65f * s, y + 0.15f * s, 0.5f * s, col, bg);
  g.fillSmoothCircle(x + 0.70f * s, y + 0.15f * s, 0.5f * s, col, bg);
  g.fillSmoothCircle(x,             y - 0.20f * s, 0.7f * s, col, bg);
  g.fillRect(x - 0.65f * s, y + 0.15f * s, 1.35f * s + 1, 0.5f * s, col);
}

static void sun(TFT_eSPI& g, float x, float y, float r, uint32_t frame, uint16_t bg) {
  float rot = frame * 0.05f;
  float w = r / 5.0f; if (w < 1.5f) w = 1.5f;
  for (int i = 0; i < 8; i++) {
    float a = rot + i * (float)M_PI / 4.0f;
    float c = cosf(a), sn = sinf(a);
    g.drawWideLine(x + c * r * 1.3f, y + sn * r * 1.3f,
                   x + c * r * 1.75f, y + sn * r * 1.75f, w, C_RAY, bg);
  }
  g.fillSmoothCircle(x, y, r, C_SUN, bg);
  g.fillSmoothCircle(x - r * 0.2f, y - r * 0.2f, r * 0.55f, C_SUN_CORE, C_SUN);
}

static void moon(TFT_eSPI& g, float x, float y, float r, uint16_t bg) {
  g.fillSmoothCircle(x, y, r, C_MOON, bg);
  g.fillSmoothCircle(x + r * 0.5f, y - r * 0.35f, r * 0.85f, bg, C_MOON);
}

static void stars(TFT_eSPI& g, float cx, float cy, float s, uint32_t frame) {
  const float pos[3][2] = {{0.55f, -0.75f}, {0.95f, 0.1f}, {0.25f, 0.75f}};
  for (int i = 0; i < 3; i++) {
    uint8_t lvl = 120 + (uint8_t)(120 * (0.5f + 0.5f * sinf(frame * 0.15f + i * 2.1f)));
    uint16_t col = rgb(lvl, lvl, lvl - 20);
    int x = cx + pos[i][0] * s, y = cy + pos[i][1] * s;
    int l = s > 20 ? 4 : 2;
    g.drawFastHLine(x - l, y, 2 * l + 1, col);
    g.drawFastVLine(x, y - l, 2 * l + 1, col);
  }
}

static void sunOrMoon(TFT_eSPI& g, float x, float y, float r, bool day,
                      uint32_t frame, uint16_t bg) {
  if (day) sun(g, x, y, r, frame, bg);
  else     moon(g, x, y, r * 1.15f, bg);
}

// Gotas/copos: caen dentro de [top, top+h], en 4 columnas
static void precipitation(TFT_eSPI& g, float cx, float top, float s, IconType t,
                          uint32_t frame, uint16_t bg) {
  float h = 1.3f * s;
  const float xs[4] = {-0.7f, -0.22f, 0.26f, 0.74f};
  float speed = (t == ICON_SNOW) ? 0.6f : (t == ICON_DRIZZLE ? 1.2f : 2.2f);
  speed *= s / 12.0f;
  for (int i = 0; i < 4; i++) {
    for (int k = 0; k < 2; k++) {                     // 2 gotas por columna
      float phase = (i * 0.37f + k * 0.5f) * h;
      float y = top + fmodf(frame * speed + phase, h);
      float x = cx + xs[i] * s;
      if (t == ICON_SNOW) {
        x += sinf(frame * 0.12f + i + k) * 0.08f * s;
        float r = s / 11.0f; if (r < 1.5f) r = 1.5f;
        g.fillSmoothCircle(x, y, r, C_SNOW, bg);
      } else if (t == ICON_DRIZZLE) {
        float r = s / 14.0f; if (r < 1.2f) r = 1.2f;
        g.fillSmoothCircle(x, y, r, C_RAIN, bg);
      } else {
        float w = s / 10.0f; if (w < 1.5f) w = 1.5f;
        g.drawWideLine(x, y, x - 0.1f * s, y + 0.28f * s, w, C_RAIN, bg);
      }
    }
  }
}

static void bolt(TFT_eSPI& g, float x, float y, float s, uint16_t col) {
  g.fillTriangle(x + 0.20f * s, y, x - 0.25f * s, y + 0.60f * s, x + 0.15f * s, y + 0.50f * s, col);
  g.fillTriangle(x - 0.10f * s, y + 0.45f * s, x + 0.25f * s, y + 0.45f * s,
                 x - 0.20f * s, y + 1.15f * s, col);
}

// ---------------------------------------------------------------- publico
void drawWeatherIcon(TFT_eSPI& g, int cx, int cy, int s, IconType type,
                     bool isDay, uint32_t frame, uint16_t bg) {
  float drift = sinf(frame * 0.06f) * 0.08f * s;

  switch (type) {
    case ICON_CLEAR:
      if (isDay) sun(g, cx, cy, 0.62f * s, frame, bg);
      else { moon(g, cx - 0.15f * s, cy, 0.8f * s, bg); stars(g, cx, cy, s, frame); }
      break;

    case ICON_PARTLY:
      sunOrMoon(g, cx - 0.4f * s, cy - 0.45f * s, 0.45f * s, isDay, frame, bg);
      cloud(g, cx + 0.25f * s + drift, cy + 0.35f * s, 0.8f * s, C_CLOUD, bg);
      break;

    case ICON_CLOUDY:
      cloud(g, cx + 0.35f * s - drift, cy - 0.35f * s, 0.7f * s, C_CLOUD2, bg);
      cloud(g, cx - 0.1f * s + drift, cy + 0.25f * s, 0.9f * s, C_CLOUD, bg);
      break;

    case ICON_FOG: {
      cloud(g, cx, cy - 0.5f * s, 0.85f * s, C_CLOUD2, bg);
      float w = s / 8.0f; if (w < 1.5f) w = 1.5f;
      for (int i = 0; i < 3; i++) {
        float off = sinf(frame * 0.08f + i * 1.7f) * 0.15f * s;
        float y = cy + (0.45f + 0.3f * i) * s;
        g.drawWideLine(cx - 1.0f * s + off, y, cx + 1.0f * s + off, y, w, C_FOG, bg);
      }
      break;
    }

    case ICON_DRIZZLE:
    case ICON_RAIN:
    case ICON_SNOW:
      precipitation(g, cx, cy - 0.1f * s, s, type, frame, bg);
      cloud(g, cx + drift, cy - 0.45f * s, 0.9f * s,
            type == ICON_RAIN ? C_CLOUD2 : C_CLOUD, bg);
      break;

    case ICON_STORM: {
      bool flash = frame > 0 && (frame % 30) < 2;
      precipitation(g, cx - 0.1f * s, cy - 0.1f * s, s, ICON_RAIN, frame, bg);
      bolt(g, cx + 0.05f * s, cy + 0.05f * s, s, flash ? TFT_WHITE : C_BOLT);
      cloud(g, cx + drift, cy - 0.45f * s, 0.9f * s, flash ? C_CLOUD2 : C_CLOUD_DK, bg);
      break;
    }

    default:
      cloud(g, cx, cy, 0.9f * s, C_CLOUD2, bg);
      break;
  }
}
