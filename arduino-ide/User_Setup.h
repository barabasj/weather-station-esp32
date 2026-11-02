// =====================================================================
//  User_Setup.h para TFT_eSPI - ideaspark ESP32 con TFT 1.9" ST7789 170x320
//  SOLO para Arduino IDE: reemplazar con este archivo el User_Setup.h
//  que esta en  Documentos/Arduino/libraries/TFT_eSPI/User_Setup.h
//  (Con PlatformIO NO hace falta: la configuracion va en platformio.ini)
// =====================================================================
#define USER_SETUP_INFO "ideaspark ESP32 1.9in ST7789"

#define ST7789_DRIVER
#define TFT_WIDTH  170
#define TFT_HEIGHT 320

// Si rojo y azul salen intercambiados, descomentar:
// #define TFT_RGB_ORDER TFT_BGR
// Si los colores salen "en negativo", descomentar:
// #define TFT_INVERSION_OFF

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL   32
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000
#define DISABLE_ALL_LIBRARY_WARNINGS
