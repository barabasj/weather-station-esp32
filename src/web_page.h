// Generado por tools/embed_page.py a partir de web/index.html - no editar a mano
#pragma once
#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Estación de clima</title>
<style>
:root{--bg:#0b1020;--card:#151d36;--line:#26305080;--fg:#e9eef8;--dim:#8a95b0;--acc:#5ac8fa;--warm:#ff9a3c;--bad:#ff6b5a;--ok:#4cd98a}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
header{position:sticky;top:0;z-index:5;background:#0b1020ee;backdrop-filter:blur(6px);border-bottom:1px solid var(--line);padding:12px 16px;display:flex;align-items:center;gap:12px}
header h1{font-size:18px;margin:0;flex:1}
header .ver{color:var(--dim);font-size:12px}
main{max-width:760px;margin:0 auto;padding:16px;display:grid;gap:16px}
section{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px}
h2{font-size:16px;margin:0 0 12px;color:var(--acc)}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px 14px}
label{display:flex;flex-direction:column;gap:4px;font-size:13px;color:var(--dim)}
label.chk{flex-direction:row;align-items:center;gap:8px;color:var(--fg);font-size:14px}
input,select{background:#0b1020;color:var(--fg);border:1px solid #33406a;border-radius:8px;padding:8px 10px;font:inherit;width:100%}
input[type=checkbox]{width:18px;height:18px;accent-color:var(--acc)}
input:focus,select:focus{outline:2px solid var(--acc);outline-offset:-1px}
button{background:#22305a;color:var(--fg);border:1px solid #33406a;border-radius:8px;padding:8px 14px;font:inherit;cursor:pointer}
button:hover{border-color:var(--acc)}
button.primary{background:var(--acc);color:#04121c;border-color:var(--acc);font-weight:600}
button.danger{border-color:#6a3030;color:#ffb4aa}
button:disabled{opacity:.5;cursor:default}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.hint{color:var(--dim);font-size:13px;margin:8px 0 0}
.status{display:grid;grid-template-columns:auto 1fr;gap:4px 14px;font-size:14px}
.status b{color:var(--dim);font-weight:500}
.alert{color:var(--warm)}
.city{display:grid;grid-template-columns:1fr 100px 100px 36px;gap:8px;margin-bottom:8px}
.city button{padding:6px}
#results{display:grid;gap:4px;margin-top:8px}
#results button{text-align:left;background:#0b1020}
.photos{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:10px;margin-top:12px}
.ph{position:relative;border-radius:8px;overflow:hidden;border:1px solid var(--line);aspect-ratio:320/170;background:#000}
.ph img{width:100%;height:100%;object-fit:cover;display:block}
.ph button{position:absolute;top:4px;right:4px;padding:2px 8px;background:#000a;border-color:#0000}
.ph span{position:absolute;left:0;right:0;bottom:0;font-size:11px;padding:2px 6px;background:#000a;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
progress{width:100%;height:10px;accent-color:var(--acc)}
.savebar{position:sticky;bottom:0;background:#0b1020ee;backdrop-filter:blur(6px);border-top:1px solid var(--line);padding:10px 16px;display:flex;justify-content:center;gap:10px}
#toast{position:fixed;left:50%;bottom:70px;transform:translateX(-50%);background:#22305a;border:1px solid var(--acc);padding:10px 16px;border-radius:10px;opacity:0;transition:opacity .3s;pointer-events:none;max-width:90vw}
#toast.show{opacity:1}
#toast.err{border-color:var(--bad)}
@media (max-width:520px){.city{grid-template-columns:1fr 1fr 36px}.city input:first-child{grid-column:1/-1}}
</style>
</head>
<body>
<header><h1>☀️ Estación de clima</h1><span class="ver" id="ver"></span></header>
<main>

<section>
  <h2>Estado</h2>
  <div class="status" id="status">Cargando…</div>
  <div class="row" style="margin-top:12px">
    <button onclick="cmd('next')">Siguiente pantalla</button>
    <button onclick="cmd('photos')">Ver fotos ahora</button>
    <button onclick="cmd('info')">Mostrar IP en la placa</button>
  </div>
</section>

<section>
  <h2>Ciudades</h2>
  <div id="cities"></div>
  <div class="row">
    <input id="q" placeholder="Buscar ciudad para agregar (ej: Rosario)" style="flex:1;min-width:200px"
           onkeydown="if(event.key==='Enter')searchCity()">
    <button onclick="searchCity()">Buscar</button>
  </div>
  <div id="results"></div>
  <p class="hint">Hasta 4 ciudades. Con más de una, se alternan en cada vuelta de pantallas.</p>
</section>

<section>
  <h2>Pantallas</h2>
  <div class="grid">
    <label>Reloj (s)<input type="number" min="0" id="secsClock"></label>
    <label>Clima actual (s)<input type="number" min="0" id="secsCurrent"></label>
    <label>Alertas (s)<input type="number" min="0" id="secsAlert"></label>
    <label>Pronóstico (s)<input type="number" min="0" id="secsForecast"></label>
    <label>24 horas (s)<input type="number" min="0" id="secsHourly"></label>
    <label>Noticias (s, mínimo)<input type="number" min="0" id="secsNews"></label>
  </div>
  <p class="hint">0 = no mostrar esa pantalla. La de noticias se queda lo necesario para que pasen todos los titulares.</p>
</section>

<section>
  <h2>Noticias</h2>
  <div class="row" style="margin-bottom:10px">
    <label class="chk"><input type="checkbox" id="newsEnabled"> Mostrar noticias</label>
    <label class="chk"><input type="checkbox" id="newsTicker"> Cinta en el reloj</label>
  </div>
  <div class="grid" style="grid-template-columns:1fr">
    <label>Fuente
      <select id="newsPreset" onchange="newsPreset()">
        <option value="">— elegir —</option>
        <option value="Google Noticias|https://news.google.com/rss?hl=es-419&gl=AR&ceid=AR:es-419">Google Noticias Argentina</option>
        <option value="Clarín|https://www.clarin.com/rss/lo-ultimo/">Clarín</option>
        <option value="La Nación|https://www.lanacion.com.ar/arc/outboundfeeds/rss/?outputType=xml">La Nación</option>
        <option value="Infobae|https://www.infobae.com/arc/outboundfeeds/rss/">Infobae</option>
      </select>
    </label>
    <label>Nombre a mostrar<input id="newsName" maxlength="40"></label>
    <label>Dirección del feed RSS<input id="newsUrl"></label>
  </div>
</section>

<section>
  <h2>Alertas</h2>
  <div class="grid">
    <label>Calor: máxima ≥ (°C)<input type="number" id="alertHeat"></label>
    <label>Heladas: mínima ≤ (°C)<input type="number" id="alertFrost"></label>
    <label>Ráfagas ≥ (km/h)<input type="number" id="alertWind"></label>
    <label>Mirar tormentas (horas)<input type="number" min="1" max="24" id="alertHours"></label>
  </div>
</section>

<section>
  <h2>Salvapantallas</h2>
  <div class="row" style="margin-bottom:10px">
    <label class="chk"><input type="checkbox" id="ssEnabled"> Activado</label>
    <label class="chk"><input type="checkbox" id="photoClock"> Hora y clima sobre la foto</label>
  </div>
  <div class="grid">
    <label>Empezar después de (min)<input type="number" min="1" id="ssAfterMin"></label>
    <label>Segundos por foto<input type="number" min="3" id="photoSecs"></label>
    <label>Fotos
      <select id="photoSource">
        <option value="0">Solo las mías</option>
        <option value="1">Solo de internet</option>
        <option value="2">Las mías, o internet si no hay</option>
      </select>
    </label>
  </div>
  <label style="margin-top:10px">Dirección de fotos de internet<input id="photoUrl"></label>
  <p class="hint">Se sale del salvapantallas apretando BOOT una vez. Las fotos de picsum.photos se
    convierten solas a un formato que la placa puede mostrar. Con otra dirección, tiene que entregar
    JPG normal (no progresivo); <code>{r}</code> en la dirección se reemplaza por un número al azar.</p>
</section>

<section>
  <h2>Mis fotos</h2>
  <div class="row">
    <input type="file" id="photoFiles" accept="image/*" multiple style="flex:1;min-width:200px">
    <button class="primary" onclick="uploadPhotos()">Subir</button>
  </div>
  <p class="hint">Cualquier foto sirve: se recorta y achica a 320×170 en el navegador antes de subirla.
    <span id="fsinfo"></span></p>
  <progress id="upProg" value="0" max="1" hidden></progress>
  <div class="photos" id="photos"></div>
</section>

<section>
  <h2>Pantalla y hora</h2>
  <div class="grid">
    <label>Brillo de día (5-255)<input type="number" min="5" max="255" id="brightDay"></label>
    <label>Brillo de noche (0-255)<input type="number" min="0" max="255" id="brightNight"></label>
    <label>Noche desde (h)<input type="number" min="0" max="23" id="nightFrom"></label>
    <label>Noche hasta (h)<input type="number" min="0" max="23" id="nightTo"></label>
    <label>Zona horaria
      <select id="tzPreset" onchange="tzPreset()">
        <option value="<-03>3">Argentina / Uruguay</option>
        <option value="<-04>4<-03>,M9.1.6/24,M4.1.6/24">Chile</option>
        <option value="<-03>3">Paraguay / Brasil (Brasilia)</option>
        <option value="<-04>4">Bolivia / Venezuela</option>
        <option value="<-05>5">Perú / Colombia / Ecuador</option>
        <option value="CST6">México (CDMX)</option>
        <option value="CET-1CEST,M3.5.0,M10.5.0/3">España</option>
        <option value="">Otra…</option>
      </select>
    </label>
    <label>Código de zona (POSIX)<input id="timezone"></label>
  </div>
  <div class="row" style="margin-top:10px">
    <label class="chk"><input type="checkbox" id="h24"> Hora de 24 h</label>
    <label class="chk"><input type="checkbox" id="fahrenheit"> Grados Fahrenheit</label>
  </div>
  <div class="grid" style="margin-top:10px">
    <label>Actualizar clima cada (min)<input type="number" min="5" id="weatherRefreshMin"></label>
    <label>Actualizar noticias cada (min)<input type="number" min="5" id="newsRefreshMin"></label>
  </div>
</section>

<section>
  <h2>Actualizar firmware</h2>
  <div class="row">
    <input type="file" id="fwFile" accept=".bin" style="flex:1;min-width:200px">
    <button class="primary" onclick="uploadFirmware()">Actualizar</button>
  </div>
  <progress id="fwProg" value="0" max="1" hidden></progress>
  <p class="hint" id="fwMsg"></p>
  <p class="hint">Elegí el archivo <code>firmware.bin</code> que genera PlatformIO en
    <code>.pio/build/ideaspark/</code>. La placa se reinicia sola al terminar; los ajustes y las fotos se conservan.</p>
</section>

<section>
  <h2>Sistema</h2>
  <div class="row">
    <button onclick="cmd('restart','¿Reiniciar la placa?')">Reiniciar</button>
    <button class="danger" onclick="cmd('wifireset','La placa va a olvidar la red WiFi y crear la red EstacionClima para configurarla de nuevo. ¿Seguir?')">Cambiar red WiFi</button>
    <button class="danger" onclick="defaults()">Volver a valores de fábrica</button>
  </div>
</section>

</main>
<div class="savebar"><button class="primary" id="saveBtn" onclick="save()">Guardar cambios</button></div>
<div id="toast"></div>

<script>
const $ = id => document.getElementById(id);
const FIELDS = ['timezone','newsEnabled','newsTicker','newsUrl','newsName','secsClock','secsCurrent',
  'secsAlert','secsForecast','secsHourly','secsNews','weatherRefreshMin','newsRefreshMin','alertHeat',
  'alertFrost','alertWind','alertHours','ssEnabled','ssAfterMin','photoSecs','photoSource','photoUrl',
  'photoClock','brightDay','brightNight','nightFrom','nightTo','fahrenheit','h24'];
let cities = [];

function toast(msg, err) {
  const t = $('toast'); t.textContent = msg; t.className = 'show' + (err ? ' err' : '');
  clearTimeout(t._h); t._h = setTimeout(() => t.className = '', 3500);
}
async function api(url, opt) {
  const r = await fetch(url, opt);
  let j = {}; try { j = await r.json(); } catch (e) {}
  if (!r.ok || j.ok === false) throw new Error(j.error || ('Error ' + r.status));
  return j;
}
const esc = s => String(s).replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));

// ---------- ajustes
function fill(s) {
  for (const k of FIELDS) {
    const el = $(k); if (!el) continue;
    if (el.type === 'checkbox') el.checked = !!s[k]; else el.value = s[k];
  }
  cities = s.cities || [];
  renderCities();
  const tz = $('tzPreset');
  tz.value = [...tz.options].some(o => o.value === s.timezone) ? s.timezone : '';
}
function gather() {
  const s = {cities};
  for (const k of FIELDS) {
    const el = $(k); if (!el) continue;
    s[k] = el.type === 'checkbox' ? el.checked : (el.type === 'number' || el.tagName === 'SELECT' && k !== 'timezone') ? Number(el.value) : el.value;
  }
  return s;
}
async function load() {
  try { fill(await api('/api/settings')); } catch (e) { toast('No se pudieron leer los ajustes: ' + e.message, 1); }
  status(); photos();
}
async function save() {
  readCityInputs();
  $('saveBtn').disabled = true;
  try {
    fill(await api('/api/settings', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(gather())}));
    toast('Guardado ✓ La placa ya usa los ajustes nuevos');
    setTimeout(status, 4000);
  } catch (e) { toast(e.message, 1); }
  $('saveBtn').disabled = false;
}
async function defaults() {
  if (!confirm('¿Volver todos los ajustes a los valores de fábrica? (Las fotos y el WiFi no se borran)')) return;
  try { fill(await api('/api/defaults', {method:'POST'})); toast('Valores de fábrica restaurados'); } catch (e) { toast(e.message, 1); }
}
function newsPreset() {
  const v = $('newsPreset').value; if (!v) return;
  const [n, u] = v.split('|'); $('newsName').value = n; $('newsUrl').value = u;
}
function tzPreset() { const v = $('tzPreset').value; if (v) $('timezone').value = v; else $('timezone').focus(); }
async function cmd(c, ask) {
  if (ask && !confirm(ask)) return;
  try { await api('/api/cmd?c=' + c, {method:'POST'}); toast('Listo'); } catch (e) { toast(e.message, 1); }
}

// ---------- estado
async function status() {
  try {
    const s = await api('/api/status');
    $('ver').textContent = 'versión ' + s.version;
    const up = s.uptime > 86400 ? Math.floor(s.uptime / 86400) + ' d' : Math.floor(s.uptime / 3600) + ' h ' + Math.floor(s.uptime % 3600 / 60) + ' min';
    let h = `<b>Hora</b><span>${esc(s.time)}</span>
      <b>WiFi</b><span>${esc(s.ssid)} (${s.rssi} dBm) · ${esc(s.ip)}</span>`;
    for (const c of s.cities) {
      h += `<b>${esc(c.name)}</b><span>${c.ok ? c.temp + '° · ' + esc(c.desc) : 'sin datos todavía'}`;
      for (const a of (c.alerts || [])) h += `<br><span class="alert">⚠ ${esc(a)}</span>`;
      h += '</span>';
    }
    h += `<b>Noticias</b><span>${s.news} titulares</span>
      <b>Encendida hace</b><span>${up} · memoria libre ${Math.round(s.heap / 1024)} KB</span>`;
    $('status').innerHTML = h;
  } catch (e) { $('status').textContent = 'No se pudo leer el estado (' + e.message + ')'; }
}

// ---------- ciudades
function renderCities() {
  $('cities').innerHTML = cities.map((c, i) => `<div class="city">
    <input value="${esc(c.name)}" data-i="${i}" data-k="name" maxlength="40" aria-label="Nombre">
    <input type="number" step="0.0001" value="${c.lat}" data-i="${i}" data-k="lat" aria-label="Latitud">
    <input type="number" step="0.0001" value="${c.lon}" data-i="${i}" data-k="lon" aria-label="Longitud">
    <button title="Quitar" onclick="readCityInputs();cities.splice(${i},1);renderCities()" ${cities.length < 2 ? 'disabled' : ''}>✕</button>
  </div>`).join('') || '<p class="hint">Agregá al menos una ciudad.</p>';
}
function readCityInputs() {
  document.querySelectorAll('#cities input').forEach(el => {
    const c = cities[el.dataset.i]; c[el.dataset.k] = el.dataset.k === 'name' ? el.value : Number(el.value);
  });
}
async function searchCity() {
  const q = $('q').value.trim(); if (!q) return;
  $('results').innerHTML = '<span class="hint">Buscando…</span>';
  try {
    const r = await fetch('https://geocoding-api.open-meteo.com/v1/search?count=6&language=es&name=' + encodeURIComponent(q));
    const j = await r.json();
    const res = j.results || [];
    $('results').innerHTML = res.length ? res.map((c, i) =>
      `<button data-i="${i}">${esc(c.name)}${c.admin1 ? ', ' + esc(c.admin1) : ''}${c.country ? ', ' + esc(c.country) : ''}
       <span class="hint">(${c.latitude.toFixed(2)}, ${c.longitude.toFixed(2)})</span></button>`).join('')
      : '<span class="hint">No se encontró. Probá con otro nombre o cargá las coordenadas a mano.</span>';
    document.querySelectorAll('#results button').forEach(b => b.onclick = () => {
      const c = res[b.dataset.i];
      readCityInputs();
      if (cities.length >= 4) return toast('Máximo 4 ciudades', 1);
      cities.push({name: c.name, lat: +c.latitude.toFixed(4), lon: +c.longitude.toFixed(4)});
      renderCities(); $('results').innerHTML = ''; $('q').value = '';
      toast('Agregada. Tocá "Guardar cambios" para aplicarla');
    });
  } catch (e) { $('results').innerHTML = '<span class="hint">Sin acceso a la búsqueda (' + esc(e.message) + ')</span>'; }
}

// ---------- fotos
async function photos() {
  try {
    const j = await api('/api/photos');
    $('fsinfo').textContent = `Espacio libre: ${Math.round((j.fsTotal - j.fsUsed) / 1024)} KB de ${Math.round(j.fsTotal / 1024)} KB.`;
    $('photos').innerHTML = j.photos.map(p => `<div class="ph">
      <img loading="lazy" src="/api/photo?path=${encodeURIComponent(p.path)}" alt="">
      <button title="Borrar" data-p="${esc(p.path)}">🗑</button><span>${esc(p.name)} · ${Math.round(p.size / 1024)} KB</span></div>`).join('')
      || '<p class="hint">Todavía no hay fotos propias.</p>';
    document.querySelectorAll('.ph button').forEach(b => b.onclick = async () => {
      if (!confirm('¿Borrar esta foto?')) return;
      try { await api('/api/photo?path=' + encodeURIComponent(b.dataset.p), {method:'DELETE'}); photos(); } catch (e) { toast(e.message, 1); }
    });
  } catch (e) { $('photos').innerHTML = '<p class="hint">No se pudo leer la lista de fotos.</p>'; }
}
// Recorta al centro y achica a 320x170, JPG normal (baseline)
function toScreenJpg(file) {
  return new Promise((ok, fail) => {
    const img = new Image();
    img.onload = () => {
      const W = 320, H = 170, c = document.createElement('canvas');
      c.width = W; c.height = H;
      const s = Math.max(W / img.width, H / img.height);
      const w = img.width * s, h = img.height * s;
      const g = c.getContext('2d'); g.imageSmoothingQuality = 'high';
      g.drawImage(img, (W - w) / 2, (H - h) / 2, w, h);
      URL.revokeObjectURL(img.src);
      c.toBlob(b => b ? ok(b) : fail(new Error('no se pudo convertir')), 'image/jpeg', 0.85);
    };
    img.onerror = () => fail(new Error('no es una imagen válida'));
    img.src = URL.createObjectURL(file);
  });
}
async function uploadPhotos() {
  const files = [...$('photoFiles').files];
  if (!files.length) return toast('Elegí una o más fotos', 1);
  const prog = $('upProg'); prog.hidden = false; prog.max = files.length; prog.value = 0;
  let ok = 0;
  for (const f of files) {
    try {
      const blob = await toScreenJpg(f);
      const name = (f.name.replace(/\.[^.]+$/, '').normalize('NFD').replace(/[^\w-]+/g, '_').slice(0, 32) || 'foto') + '.jpg';
      const fd = new FormData(); fd.append('file', blob, name);
      await api('/api/photos', {method:'POST', body: fd});
      ok++;
    } catch (e) { toast(f.name + ': ' + e.message, 1); }
    prog.value++;
  }
  prog.hidden = true; $('photoFiles').value = '';
  if (ok) toast(ok + (ok > 1 ? ' fotos subidas' : ' foto subida'));
  photos();
}

// ---------- firmware
function uploadFirmware() {
  const f = $('fwFile').files[0];
  const msg = $('fwMsg');
  if (!f) { msg.textContent = 'Primero elegí el archivo firmware.bin'; return toast('Elegí el archivo firmware.bin', 1); }
  if (!/\.bin$/i.test(f.name)) { msg.textContent = 'El archivo tiene que terminar en .bin'; return; }
  const btn = $('fwFile').nextElementSibling; btn.disabled = true;
  const prog = $('fwProg'); prog.hidden = false; prog.value = 0;
  msg.textContent = 'Subiendo ' + f.name + '… no cierres esta página.';
  const x = new XMLHttpRequest();
  x.open('POST', '/update?size=' + f.size);
  x.upload.onprogress = e => {
    if (!e.lengthComputable) return;
    prog.value = e.loaded / e.total;
    msg.textContent = 'Subiendo… ' + Math.round(100 * e.loaded / e.total) + '%';
  };
  x.onload = () => {
    let j = {}; try { j = JSON.parse(x.responseText); } catch (e) {}
    if (x.status === 200 && j.ok !== false) {
      msg.textContent = 'Actualizado ✓ La placa se está reiniciando; esta página se recarga sola en 15 segundos.';
      toast('Actualizado ✓ Reiniciando…');
      setTimeout(() => location.reload(), 15000);
    } else {
      prog.hidden = true; btn.disabled = false;
      msg.textContent = j.error || ('Falló la actualización (error ' + x.status + ')');
      toast(msg.textContent, 1);
    }
  };
  x.onerror = () => { prog.hidden = true; btn.disabled = false; msg.textContent = 'Se cortó la conexión con la placa.'; toast(msg.textContent, 1); };
  const fd = new FormData(); fd.append('firmware', f, f.name); x.send(fd);
}

load();
setInterval(status, 30000);
</script>
</body>
</html>
)html";
