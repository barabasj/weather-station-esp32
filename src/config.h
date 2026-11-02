// =====================================================================
//  CONFIGURACION DEL USUARIO  -  editar solo este archivo
// =====================================================================
#pragma once

// ---------- WiFi ----------
// El WiFi se puede configurar desde el celular (ver README): si la placa
// no logra conectarse, crea la red "EstacionClima" con una pagina de
// configuracion. Opcionalmente se puede dejar una red fija aca
// (dejar "" para usar solo la configuracion desde el celular).
#define WIFI_SSID      ""
#define WIFI_PASSWORD  ""
#define PORTAL_AP_NAME "EstacionClima"   // red que crea la placa para configurarla
#define PORTAL_TIMEOUT_S 300             // segundos que espera la configuracion

// ---------- Ciudades ----------
// Coordenadas: click derecho sobre el lugar en Google Maps.
// Con mas de una ciudad, se alternan en cada vuelta de pantallas (maximo 4).
struct City { const char* name; float lat; float lon; };
static const City CITIES[] = {
  { "Buenos Aires", -34.6037, -58.3816 },
  // { "Córdoba",      -31.4201, -64.1888 },
  // { "Mar del Plata", -38.0055, -57.5426 },
};

// ---------- Zona horaria ----------
// Argentina: "<-03>3"   Uruguay: "<-03>3"   Chile: "<-04>4<-03>,M9.1.6/24,M4.1.6/24"
// Espana:    "CET-1CEST,M3.5.0,M10.5.0/3"   Mexico (CDMX): "CST6"
#define TIMEZONE    "<-03>3"

// ---------- Noticias (cualquier feed RSS) ----------
// Google Noticias Argentina. Otros ejemplos:
//   Clarin:    "https://www.clarin.com/rss/lo-ultimo/"
//   La Nacion: "https://www.lanacion.com.ar/arc/outboundfeeds/rss/?outputType=xml"
//   Infobae:   "https://www.infobae.com/arc/outboundfeeds/rss/"
#define NEWS_ENABLED    1
#define NEWS_URL        "https://news.google.com/rss?hl=es-419&gl=AR&ceid=AR:es-419"
#define NEWS_NAME       "Google Noticias"
#define NEWS_MAX_ITEMS  12
#define NEWS_TICKER     1      // 1 = cinta de titulares en la pantalla del reloj

// ---------- Alertas ----------
// Se agrega una pantalla de alerta a la rotacion cuando se cumple alguna:
#define ALERT_HEAT_C     35    // maxima de hoy igual o mayor
#define ALERT_FROST_C    0     // minima de hoy o manana igual o menor
#define ALERT_WIND_KMH   55    // rafagas actuales igual o mayores
#define ALERT_HOURS      12    // tormentas / lluvia fuerte / nieve en las proximas N horas

// ---------- Actualizaciones ----------
#define WEATHER_REFRESH_MIN  15   // minutos entre consultas del clima
#define NEWS_REFRESH_MIN     30   // minutos entre consultas de noticias

// ---------- Pantallas ----------
// Segundos que se muestra cada pantalla (0 = no mostrarla en la rotacion)
#define SECS_CLOCK     20
#define SECS_CURRENT   12
#define SECS_ALERT     10   // solo aparece si hay alertas
#define SECS_FORECAST  12
#define SECS_HOURLY    12
#define SECS_NEWS      24   // cada titular se muestra 6 segundos

// Boton BOOT (GPIO0) de la placa: pasa a la siguiente pantalla.
// Mantenerlo apretado al encender: borra el WiFi guardado y abre la configuracion.
#define BUTTON_PIN     0

// ---------- Salvapantallas de fotos ----------
// Despues de SCREENSAVER_AFTER_MIN minutos sin tocar BOOT se muestran fotos.
// Apretando BOOT se vuelve a la secuencia de pantallas.
#define SCREENSAVER_ENABLED    1
#define SCREENSAVER_AFTER_MIN  10
#define PHOTO_SECS             20    // segundos por foto
// Origen de las fotos: 0 = solo las propias (carpeta data/, ver README)
//                      1 = solo de internet
//                      2 = las propias si hay, si no de internet
#define PHOTO_SOURCE           2
#define PHOTO_URL              "https://picsum.photos/320/170"
#define PHOTO_SHOW_CLOCK       1     // hora chiquita sobre la foto

// ---------- Brillo ----------
#define BRIGHTNESS_DAY    255   // 0-255
#define BRIGHTNESS_NIGHT  40
#define NIGHT_FROM_HOUR   23    // desde las 23 hs...
#define NIGHT_TO_HOUR     7     // ...hasta las 7 hs, brillo nocturno

// ---------- Unidades ----------
#define USE_FAHRENHEIT  0
#define USE_24H         1

// (no tocar)
#define CITY_COUNT ((int)(sizeof(CITIES) / sizeof(CITIES[0])))
