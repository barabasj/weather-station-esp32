# Estación de clima — ideaspark ESP32 (ESP-WROOM-32) + TFT 1.9"

Firmware para la placa **ideaspark ESP32 con pantalla TFT 1.9" ST7789 (170×320)** integrada.
Muestra hora y fecha, clima actual con íconos animados, alertas, pronóstico de 5 días, gráfico
de las próximas 24 horas, titulares de noticias y un salvapantallas de fotos. Textos con tildes
y eñes, WiFi configurable desde el celular, una o varias ciudades y una **página web de
configuración** en la propia placa (ajustes, fotos y actualización del firmware por WiFi).

- **Clima:** [Open-Meteo](https://open-meteo.com) (gratis, sin API key)
- **Noticias:** cualquier feed RSS (por defecto Google Noticias Argentina)
- **Hora:** NTP, con zona horaria configurable

## Página web de configuración

Con la placa conectada a tu WiFi, abrí en el navegador del celular o la PC:

- **`http://estacion.local`**, o
- **`http://` + la IP de la placa** (por ejemplo `http://192.168.0.57`).

La IP aparece en la pantalla al arrancar y cada vez que **mantenés apretado BOOT ~1 segundo**
(pantalla *Información*). También se puede ver en el Serial Monitor.
En algunos Android `estacion.local` no funciona: usá la IP.

Desde la página se puede, sin recompilar ni conectar el cable:

| Sección | Qué se puede hacer |
|---------|--------------------|
| **Estado** | Hora, WiFi, clima y alertas de cada ciudad; botones *Siguiente pantalla*, *Ver fotos ahora* y *Mostrar IP en la placa* |
| **Ciudades** | Buscar ciudades por nombre y agregarlas (hasta 4), o cargar coordenadas a mano |
| **Pantallas** | Segundos de cada pantalla (0 = no mostrarla) |
| **Noticias** | Activar / desactivar, cinta en el reloj, elegir el medio (Google Noticias, Clarín, La Nación, Infobae) o cualquier feed RSS |
| **Alertas** | Umbrales de calor, heladas, ráfagas y horas a mirar hacia adelante |
| **Salvapantallas** | Activarlo, minutos de espera, segundos por foto, origen de las fotos, hora y clima sobre la foto |
| **Mis fotos** | Subir fotos desde el celular o la PC (cualquier tamaño: se recortan y achican a 320×170 en el navegador antes de subirlas), verlas y borrarlas. Entran ~100 |
| **Pantalla y hora** | Brillo de día y de noche, horario nocturno, zona horaria, 24 h / Fahrenheit, frecuencia de actualización |
| **Actualizar firmware** | Subir un `firmware.bin` nuevo por WiFi (ver abajo) |
| **Sistema** | Reiniciar, cambiar la red WiFi, volver a valores de fábrica |

Los cambios se aplican al tocar **Guardar cambios** y quedan guardados en la placa aunque se
apague. `config.h` pasa a ser solo los **valores iniciales** (y los de "valores de fábrica").

La página no tiene contraseña: cualquiera conectado a tu misma red WiFi puede abrirla.

### Actualizar el firmware por WiFi

1. En PlatformIO, **Build** (el tilde ✓ de la barra inferior), sin subir.
2. En la página, *Actualizar firmware* → elegir `.pio/build/ideaspark/firmware.bin` → **Actualizar**.
3. La placa muestra una barra de progreso y se reinicia sola. Ajustes, WiFi y fotos se conservan.

## Pantallas

Rotan solas cada tantos segundos. Botón **BOOT**: toque corto = siguiente pantalla; apretado ~1 s = pantalla *Información* con la IP de la página web.

| # | Pantalla | Contenido |
|---|----------|-----------|
| 1 | **Reloj** | Hora grande con segundos, fecha en español, resumen del clima (ícono, temperatura, máx/mín) y **cinta de titulares** desplazándose abajo. Un triángulo amarillo arriba a la derecha avisa si hay alertas |
| 2 | **Ahora** | Ícono **animado** (sol que gira, lluvia que cae, nieve, niebla, rayos que destellan, luna y estrellas de noche), temperatura, descripción, sensación térmica, nubosidad, humedad, viento y presión |
| 3 | **Alerta** | Solo aparece si hay algo para avisar: tormentas, lluvia fuerte o nevadas en las próximas horas, calor extremo, heladas o ráfagas fuertes |
| 4 | **Pronóstico** | 5 días: ícono, máxima, mínima, probabilidad de lluvia e índice UV |
| 5 | **24 horas** | Gráfico de temperatura con barras de probabilidad de lluvia e íconos cada 4 horas |
| 6 | **Noticias** | Todos los titulares completos en una lista que se desplaza hacia arriba, separados por una línea punteada y con el medio en celeste. La pantalla se queda hasta que pasan todos |

Con varias ciudades configuradas, cada vuelta completa de pantallas muestra la siguiente ciudad.

### Salvapantallas de fotos

Después de 10 minutos (configurable) sin tocar BOOT, la pantalla pasa a mostrar fotos, una
cada 20 segundos, con la hora abajo a la derecha y el ícono del tiempo con la temperatura abajo
a la izquierda.
**Apretando BOOT una vez se vuelve a la secuencia de pantallas**, empezando por el reloj.

Las fotos pueden ser:
- **De internet** (sin hacer nada): fotos al azar de picsum.photos. Como picsum entrega JPG
  progresivos (que el ESP32 no puede leer), la placa las pide a través de images.weserv.nl, que
  las convierte a JPG normal de 320×170.
- **Propias**: lo más fácil es subirlas desde la **página web** (sección *Mis fotos*).
  Alternativa con cable: copiar `.jpg` a la carpeta `data/` y en PlatformIO
  *Project Tasks → ideaspark → Platform → Upload Filesystem Image* (esto reemplaza todas las
  fotos). Por ese camino tienen que ser JPG "baseline" de 320×170 idealmente.

## Hardware

| Señal TFT | GPIO |
|-----------|------|
| MOSI | 23 |
| SCLK | 18 |
| CS | 15 |
| DC | 2 |
| RST | 4 |
| Backlight | 32 |

Botón BOOT = GPIO0. No hay que cablear nada: la pantalla ya viene en la placa.

## WiFi desde el celular

No hace falta escribir la clave en el código:
1. Si la placa no encuentra una red conocida, muestra **"Configurar WiFi"** y crea su propia red
   llamada **EstacionClima**.
2. Desde el celular, conectate a esa red: se abre sola una página (si no, entrá a `192.168.4.1`).
3. Tocá *Configure WiFi*, elegí tu red, poné la clave y guardá. La placa se conecta y sigue arrancando.

La red queda guardada en la placa. Para cambiarla, **encendé la placa con BOOT apretado** y
soltalo cuando lo pida: se borra la red guardada y vuelve a abrir la configuración.
(Si preferís, todavía podés dejar una red fija en `WIFI_SSID` / `WIFI_PASSWORD`.)

> ideaspark también vende una versión con pantallita **OLED 0.96" monocromática**. Este
> firmware es para la versión **TFT color de 1.9"**.

## Si venís de la versión anterior

Esta versión cambia la forma en que se divide la memoria de la placa (para poder actualizar
por WiFi), así que **la primera vez hay que subirla con el cable USB**, como siempre.
El WiFi guardado se conserva; las fotos subidas por el método de la carpeta `data/` se borran
(volvé a subirlas desde la página). Desde ahí en adelante, las actualizaciones pueden ser por WiFi.

## Instalación (opción A — recomendada): VS Code + PlatformIO

1. Instalar [VS Code](https://code.visualstudio.com/) y, dentro de VS Code, la extensión **PlatformIO IDE**.
2. Abrir esta carpeta (`ideaspark-weather`) con *File → Open Folder*.
3. Opcional: editar **`src/config.h`** con tu ciudad y preferencias iniciales. Igual todo se
   puede cambiar después desde la página web.
4. Conectar la placa por USB y apretar **→ Upload** (flecha en la barra inferior de PlatformIO).
   PlatformIO descarga solo las librerías y la configuración de pantalla ya está en `platformio.ini`.
5. Opcional: **Serial Monitor** (ícono de enchufe) a 115200 para ver qué está haciendo.

## Instalación (opción B): Arduino IDE

1. *Boards Manager* → instalar **esp32 by Espressif**, versión **2.0.17** (es la versión con la que se probó; TFT_eSPI tiene problemas conocidos con la 3.x).
2. *Library Manager* → instalar **TFT_eSPI** (Bodmer), **ArduinoJson** (Benoit Blanchon, v7),
   **TJpg_Decoder** (Bodmer) y **WiFiManager** (tzapu).
3. Reemplazar `Documentos/Arduino/libraries/TFT_eSPI/User_Setup.h` por el archivo
   **`arduino-ide/User_Setup.h`** de este proyecto.
4. Crear una carpeta llamada `WeatherStation`, copiar ahí **todos** los archivos de `src/`
   y renombrar `main.cpp` a **`WeatherStation.ino`**.
   Copiar también **`partitions.csv`** a esa carpeta.
5. Abrir `WeatherStation.ino`, editar `config.h`, elegir placa **ESP32 Dev Module**,
   *Partition Scheme* → **Minimal SPIFFS (1.9MB APP with OTA)** (el `partitions.csv` de la
   carpeta es el que realmente se usa) y subir. Las fotos se suben desde la página web.

## Valores iniciales (`src/config.h`)

Todo esto se cambia después desde la página web; `config.h` solo define con qué arranca la
placa la primera vez (y a qué vuelve con *valores de fábrica*).

| Ajuste | Para qué |
|--------|----------|
| `WIFI_SSID`, `WIFI_PASSWORD` | Opcional: red fija (si no, se configura desde el celular). Solo 2,4 GHz |
| `CITIES` | Lista de ciudades `{ "Nombre", latitud, longitud }`, hasta 4 (coordenadas: clic derecho en Google Maps) |
| `TIMEZONE` | Zona horaria POSIX; Argentina = `<-03>3` |
| `NEWS_URL`, `NEWS_NAME` | Feed RSS de noticias (hay ejemplos de Clarín, La Nación e Infobae) |
| `NEWS_ENABLED`, `NEWS_TICKER` | Activar noticias / cinta en el reloj |
| `ALERT_*` | Umbrales de las alertas (calor, heladas, ráfagas, horas a mirar hacia adelante) |
| `SCREENSAVER_*`, `PHOTO_*` | Salvapantallas: activarlo, después de cuántos minutos, segundos por foto, origen |
| `SECS_*` | Segundos de cada pantalla; **0** la saca de la rotación |
| `BRIGHTNESS_*`, `NIGHT_*` | Brillo de día y de noche, y horario nocturno |
| `USE_FAHRENHEIT`, `USE_24H` | Unidades |

## Si algo no anda

| Síntoma | Solución |
|---------|----------|
| Pantalla negra pero con luz | Revisar que la placa sea la versión TFT 1.9"; confirmar pines en `platformio.ini` |
| Rojo y azul cambiados | Descomentar `TFT_RGB_ORDER=TFT_BGR` |
| Colores "en negativo" | Descomentar `TFT_INVERSION_OFF` |
| Imagen corrida unos píxeles | Confirmar `TFT_WIDTH=170` y `TFT_HEIGHT=320` |
| Imagen al revés | En `main.cpp`, `tft.setRotation(1)` → `3` |
| "Sin WiFi" / no aparece la red de 5 GHz | El ESP32 solo usa redes de 2,4 GHz |
| Quiero cambiar de red WiFi | Página web → *Sistema → Cambiar red WiFi*, o encender con BOOT apretado |
| No sé la IP de la placa | Mantener BOOT apretado 1 segundo |
| `estacion.local` no abre | Usar la IP (algunos Android no soportan `.local`) |
| Error "Sketch too big" en Arduino IDE | Elegir *Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA)* |
| Una foto de la carpeta `data/` no aparece | Es JPG progresivo: subirla desde la página web, que la convierte |
| No aparecen las fotos de internet | Ver el Serial Monitor (`[fotos] ...`). Si usás otra dirección, tiene que entregar JPG normal, no progresivo |
| No aparecen noticias | Probar otro `NEWS_URL`; ver el Serial Monitor (`[noticias] HTTP ...`) |

Los textos usan la tipografía **Inter** (licencia libre SIL OFL) con tildes, eñes, `°`, `¿¡`
y comillas tipográficas; los caracteres que no existen en la fuente (emojis, etc.) se omiten.
Para cambiar tamaños o tipografía: `python3 tools/gen_vlw.py src/fonts.h` (requiere Pillow;
editar al final del script la lista de fuentes y tamaños).

## Estructura del código

```
platformio.ini        configuración de compilación + pines de la pantalla
src/config.h          valores iniciales (después todo se cambia desde la página web)
src/main.cpp          arranque, WiFi (portal), rotación de pantallas, botón, brillo, salvapantallas
src/weather.*         consulta de Open-Meteo por ciudad; códigos WMO → ícono/descripción; alertas
src/news.*            lectura en streaming del RSS (no carga el feed entero en memoria)
src/icons.*           íconos del clima dibujados con primitivas (escalables y animados)
src/screens.*         las pantallas
src/photos.*          fotos del salvapantallas (LittleFS o internet, decodificación JPG)
src/settings.*        ajustes guardados en la placa (se editan desde la página web)
src/web.*             servidor web: API, subida de fotos, actualización por WiFi
src/web_page.h        la página (generada desde web/index.html con tools/embed_page.py)
web/index.html        código fuente de la página
partitions.csv        memoria: 2 × 1,5 MB de programa (actualización por WiFi) + 896 KB de fotos
src/fonts.h           tipografías con tildes (generadas, no editar a mano)
data/                 fotos propias para el salvapantallas
tools/gen_vlw.py      generador de fonts.h
tools/embed_page.py   convierte web/index.html en src/web_page.h
arduino-ide/          User_Setup.h para quien use Arduino IDE
```

Compilado y verificado con Arduino core ESP32 2.0.17, TFT_eSPI 2.5.43, ArduinoJson 7.2.1,
TJpg_Decoder 1.1.0 y WiFiManager 2.0.17: 1,33 MB de 1,5 MB de programa (84 %) y 58 KB de RAM
global. La página web se probó en Chromium contra una placa simulada.
