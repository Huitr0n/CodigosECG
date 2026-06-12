<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ECG Monitor — CardioLink</title>
  <!-- Sin dependencias externas — conexión directa al ESP32 -->
  <style>
    :root {
      --blue:   #38bdf8;
      --green:  #00e676;
      --yellow: #ffea00;
      --red:    #ff1744;
      --bg:     #0a0e1a;
      --panel:  #111827;
      --border: #1f2937;
      --text:   #e2e8f0;
      --dim:    #64748b;
      --accent: #38bdf8;
    }

    * { margin: 0; padding: 0; box-sizing: border-box; }

    body {
      background: var(--bg);
      color: var(--text);
      font-family: 'Segoe UI', system-ui, sans-serif;
      min-height: 100vh;
      overflow-x: hidden;
    }

    body::before {
      content: '';
      position: fixed; inset: 0;
      background:
        repeating-linear-gradient(0deg,   transparent, transparent 39px, rgba(56,189,248,.04) 40px),
        repeating-linear-gradient(90deg,  transparent, transparent 39px, rgba(56,189,248,.04) 40px);
      pointer-events: none; z-index: 0;
    }

    /* ── Header ──────────────────────────────────────────── */
    header {
      display: flex; align-items: center; justify-content: space-between;
      padding: 1rem 2rem;
      background: rgba(17,24,39,.9);
      border-bottom: 1px solid var(--border);
      backdrop-filter: blur(8px);
      position: sticky; top: 0; z-index: 100;
    }

    .logo {
      display: flex; align-items: center; gap: .75rem;
      font-size: 1.4rem; font-weight: 700; letter-spacing: .05em; color: var(--accent);
    }
    .logo svg { width: 32px; height: 32px; }

    .connection-badge {
      display: flex; align-items: center; gap: .5rem;
      font-size: .85rem; padding: .35rem .9rem;
      border-radius: 999px; border: 1px solid var(--border);
      background: var(--panel); cursor: pointer; transition: border-color .2s;
    }
    .connection-badge:hover { border-color: var(--accent); }

    .dot {
      width: 8px; height: 8px; border-radius: 50%;
      background: #ef4444; transition: background .4s;
    }
    .dot.connected  { background: var(--green); box-shadow: 0 0 6px var(--green); }
    .dot.connecting { background: var(--yellow); animation: blink .8s infinite; }

    @keyframes blink { 50% { opacity: .3; } }

    /* ── Layout ──────────────────────────────────────────── */
    main {
      position: relative; z-index: 1;
      max-width: 1200px; margin: 0 auto;
      padding: 1.5rem 1.5rem 3rem;
      display: grid; grid-template-columns: 1fr 320px; gap: 1.5rem;
    }

    /* ── ECG panel ─────────────────────────────────────── */
    .ecg-panel {
      background: var(--panel); border: 1px solid var(--border);
      border-radius: 16px; padding: 1.25rem;
      display: flex; flex-direction: column; gap: 1rem;
    }

    .panel-title {
      font-size: .75rem; font-weight: 600; letter-spacing: .15em;
      text-transform: uppercase; color: var(--dim);
    }

    #ecg-canvas {
      width: 100%; height: 220px; border-radius: 8px;
      background: #070b14; border: 1px solid #1a2235; display: block;
    }

    .ecg-footer {
      display: flex; justify-content: space-between; align-items: center;
      font-size: .78rem; color: var(--dim);
    }

    /* ── Side panel ─────────────────────────────────────── */
    .side-panel { display: flex; flex-direction: column; gap: 1.25rem; }

    /* ── BPM card ──────────────────────────────────────── */
    .bpm-card {
      background: var(--panel); border: 1px solid var(--border);
      border-radius: 16px; padding: 1.5rem; text-align: center;
      position: relative; overflow: hidden;
      transition: border-color .5s, box-shadow .5s;
    }

    .bpm-card::after {
      content: ''; position: absolute; inset: 0;
      background: radial-gradient(circle at 50% 120%, var(--zone-color, transparent) 0%, transparent 70%);
      opacity: .12; pointer-events: none; transition: opacity .5s;
    }

    .bpm-card.zone-brady   { --zone-color: var(--blue);   border-color: rgba(56,189,248,.35); box-shadow: 0 0 30px rgba(56,189,248,.1); }
    .bpm-card.zone-normal  { --zone-color: var(--green);  border-color: rgba(0,230,118,.35);  box-shadow: 0 0 30px rgba(0,230,118,.08); }
    .bpm-card.zone-warning { --zone-color: var(--yellow); border-color: rgba(255,234,0,.35);  box-shadow: 0 0 30px rgba(255,234,0,.08); }
    .bpm-card.zone-danger  { --zone-color: var(--red);    border-color: rgba(255,23,68,.35);  box-shadow: 0 0 30px rgba(255,23,68,.08); }

    /* Heart */
    .heart-wrapper { margin: .25rem auto 1rem; width: 72px; height: 72px; position: relative; }
    .heart {
      width: 100%; height: 100%;
      display: flex; align-items: center; justify-content: center;
      animation: heartbeat var(--bpm-interval, 1s) ease-in-out infinite;
      transform-origin: center;
      filter: drop-shadow(0 0 12px var(--zone-color, #fff));
    }
    @keyframes heartbeat {
      0%,100% { transform: scale(1); }
      15%     { transform: scale(1.25); }
      30%     { transform: scale(1); }
      45%     { transform: scale(1.15); }
      60%     { transform: scale(1); }
    }
    .heart svg { width: 56px; height: 56px; }
    .zone-brady   .heart { color: var(--blue);   }
    .zone-normal  .heart { color: var(--green);  }
    .zone-warning .heart { color: var(--yellow); }
    .zone-danger  .heart { color: var(--red);    }

    .pulse-ring {
      position: absolute; inset: 0; border-radius: 50%;
      border: 2px solid currentColor;
      animation: pulseRing var(--bpm-interval, 1s) ease-out infinite;
      opacity: 0;
    }
    .zone-brady   .pulse-ring { color: var(--blue);   }
    .zone-normal  .pulse-ring { color: var(--green);  }
    .zone-warning .pulse-ring { color: var(--yellow); }
    .zone-danger  .pulse-ring { color: var(--red);    }
    @keyframes pulseRing {
      0%   { transform: scale(.8); opacity: .7; }
      100% { transform: scale(1.9); opacity: 0; }
    }

    .bpm-value {
      font-size: 4rem; font-weight: 800; line-height: 1;
      letter-spacing: -.04em; transition: color .5s;
    }
    .zone-brady   .bpm-value { color: var(--blue);   }
    .zone-normal  .bpm-value { color: var(--green);  }
    .zone-warning .bpm-value { color: var(--yellow); }
    .zone-danger  .bpm-value { color: var(--red);    }

    .bpm-unit { font-size: .9rem; color: var(--dim); margin-top: .25rem; }

    .zone-label {
      margin-top: 1rem; display: inline-flex; align-items: center; gap: .4rem;
      padding: .35rem 1rem; border-radius: 999px; font-size: .8rem; font-weight: 600;
      letter-spacing: .06em; text-transform: uppercase; transition: background .5s, color .5s;
    }
    .zone-brady   .zone-label { background: rgba(56,189,248,.15); color: var(--blue);   }
    .zone-normal  .zone-label { background: rgba(0,230,118,.15);  color: var(--green);  }
    .zone-warning .zone-label { background: rgba(255,234,0,.15);  color: var(--yellow); }
    .zone-danger  .zone-label { background: rgba(255,23,68,.15);  color: var(--red);    }

    /* ── Alert banner ─────────────────────────────────── */
    .alert-banner {
      border-radius: 12px; padding: 1rem 1.2rem; font-size: .85rem;
      display: none; gap: .6rem; align-items: flex-start;
      border: 1px solid transparent; animation: slideIn .3s ease;
    }
    .alert-banner.visible { display: flex; }
    @keyframes slideIn {
      from { opacity: 0; transform: translateY(-8px); }
      to   { opacity: 1; transform: translateY(0); }
    }
    .alert-banner.brady   { background: rgba(56,189,248,.08); border-color: rgba(56,189,248,.3); color: var(--blue);   }
    .alert-banner.warning { background: rgba(255,234,0,.08);  border-color: rgba(255,234,0,.3);  color: var(--yellow); }
    .alert-banner.danger  { background: rgba(255,23,68,.08);  border-color: rgba(255,23,68,.3);  color: var(--red);    }

    /* ── Zones legend ─────────────────────────────────── */
    .zones-card { background: var(--panel); border: 1px solid var(--border); border-radius: 16px; padding: 1.25rem; }
    .zone-row { display: flex; align-items: center; gap: .75rem; padding: .6rem 0; border-bottom: 1px solid var(--border); font-size: .85rem; }
    .zone-row:last-child { border-bottom: none; }
    .zone-dot { width: 12px; height: 12px; border-radius: 50%; flex-shrink: 0; }
    .z-blue   { background: var(--blue);   box-shadow: 0 0 6px var(--blue);   }
    .z-green  { background: var(--green);  box-shadow: 0 0 6px var(--green);  }
    .z-yellow { background: var(--yellow); box-shadow: 0 0 6px var(--yellow); }
    .z-red    { background: var(--red);    box-shadow: 0 0 6px var(--red);    }
    .zone-range { margin-left: auto; color: var(--dim); font-size: .78rem; }

    /* ── Stats ─────────────────────────────────────────── */
    .stats-row { display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; }
    .stat-card { background: var(--panel); border: 1px solid var(--border); border-radius: 12px; padding: 1rem; text-align: center; }
    .stat-label { font-size: .7rem; color: var(--dim); text-transform: uppercase; letter-spacing: .1em; }
    .stat-value { font-size: 1.6rem; font-weight: 700; color: var(--accent); margin-top: .2rem; }
    .stat-unit  { font-size: .7rem; color: var(--dim); }

    /* ── Demo notice ─────────────────────────────────── */
    .demo-notice {
      text-align: center; padding: .5rem; font-size: .75rem;
      color: var(--dim); background: rgba(255,234,0,.06);
      border: 1px dashed rgba(255,234,0,.2); border-radius: 8px;
    }

    /* ── Particle canvases ────────────────────────────── */
    #particle-canvas, #ice-canvas {
      position: fixed; inset: 0; pointer-events: none; z-index: 0;
      opacity: 0; transition: opacity .6s;
    }
    #particle-canvas.active, #ice-canvas.active { opacity: 1; }

    /* ═════════════════════════════════════════════════
       MODAL — sistema de tabs WiFi / Bluetooth
    ═════════════════════════════════════════════════ */
    .modal-bg {
      position: fixed; inset: 0; background: rgba(0,0,0,.7);
      backdrop-filter: blur(4px);
      display: flex; align-items: center; justify-content: center;
      z-index: 200;
    }
    .modal-bg.hidden { display: none; }

    .modal {
      background: var(--panel); border: 1px solid var(--border);
      border-radius: 20px; padding: 2rem; width: 420px; max-width: 95vw;
    }
    .modal h2 { font-size: 1.2rem; margin-bottom: .25rem; }
    .modal > p { color: var(--dim); font-size: .85rem; margin-bottom: 1.25rem; }

    /* Tabs */
    .tab-bar {
      display: flex; gap: .5rem; margin-bottom: 1.25rem;
      background: var(--bg); border-radius: 10px; padding: .3rem;
    }
    .tab-btn {
      flex: 1; padding: .5rem; border: none; border-radius: 7px;
      background: transparent; color: var(--dim); font-size: .85rem;
      font-weight: 600; cursor: pointer; transition: background .2s, color .2s;
      display: flex; align-items: center; justify-content: center; gap: .4rem;
    }
    .tab-btn.active { background: var(--border); color: var(--text); }

    .tab-panel { display: none; }
    .tab-panel.active { display: block; }

    .form-group { margin-bottom: 1rem; }
    .form-group label { font-size: .8rem; color: var(--dim); display: block; margin-bottom: .4rem; }
    .form-group input, .form-group select {
      width: 100%; background: var(--bg); border: 1px solid var(--border);
      border-radius: 8px; padding: .6rem .85rem; color: var(--text);
      font-size: .9rem; outline: none; transition: border-color .2s;
      appearance: none;
    }
    .form-group input:focus, .form-group select:focus { border-color: var(--accent); }

    .form-row { display: flex; gap: .75rem; }
    .form-row .form-group { flex: 1; }

    .btn {
      width: 100%; padding: .7rem; border: none; border-radius: 10px;
      font-size: .95rem; font-weight: 600; cursor: pointer; transition: opacity .2s;
    }
    .btn:hover { opacity: .85; }
    .btn-primary { background: var(--accent); color: #000; }
    .btn-ghost   { background: transparent; color: var(--dim); border: 1px solid var(--border); margin-top: .5rem; }
    .btn-scan    { background: #1e293b; color: var(--accent); border: 1px solid var(--border); margin-bottom: .75rem; }

    /* Scan list */
    .scan-list {
      max-height: 140px; overflow-y: auto; border: 1px solid var(--border);
      border-radius: 8px; margin-bottom: .75rem; display: none;
    }
    .scan-list.visible { display: block; }
    .scan-item {
      padding: .55rem .85rem; font-size: .85rem; cursor: pointer;
      border-bottom: 1px solid var(--border); transition: background .15s;
      display: flex; align-items: center; gap: .6rem;
    }
    .scan-item:last-child { border-bottom: none; }
    .scan-item:hover { background: rgba(56,189,248,.08); }
    .scan-item .dev-name { flex: 1; }
    .scan-item .dev-addr { color: var(--dim); font-size: .75rem; font-family: monospace; }

    .status-msg {
      font-size: .78rem; color: var(--dim); text-align: center;
      min-height: 1.2rem; margin-bottom: .5rem;
    }

    /* BT sub-tabs */
    .bt-mode-bar {
      display: flex; gap: .4rem; margin-bottom: 1rem;
      background: var(--bg); border-radius: 8px; padding: .25rem;
    }
    .bt-mode-btn {
      flex: 1; padding: .4rem; border: none; border-radius: 6px;
      background: transparent; color: var(--dim); font-size: .8rem;
      font-weight: 600; cursor: pointer; transition: background .2s, color .2s;
    }
    .bt-mode-btn.active { background: var(--border); color: var(--text); }
    .bt-sub { display: none; }
    .bt-sub.active { display: block; }

    @media (max-width: 768px) {
      main { grid-template-columns: 1fr; }
      .side-panel { order: -1; }
    }
  </style>
</head>
<body>

<!-- Particle canvases -->
<canvas id="particle-canvas"></canvas>
<canvas id="ice-canvas"></canvas>

<header>
  <div class="logo">
    <svg viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M2 16h5l3-8 4 16 4-10 3 6 3-4h6" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>
    </svg>
    CardioLink
  </div>
  <div class="connection-badge" id="conn-badge" onclick="openModal()">
    <div class="dot" id="conn-dot"></div>
    <span id="conn-text">Desconectado</span>
  </div>
</header>

<main>
  <!-- LEFT -->
  <div style="display:flex; flex-direction:column; gap:1.25rem;">

    <div class="ecg-panel">
      <div class="panel-title">Señal ECG en tiempo real</div>
      <canvas id="ecg-canvas"></canvas>
      <div class="ecg-footer">
        <span>Velocidad: 25 mm/s</span>
        <span id="timestamp">--:--:--</span>
      </div>
    </div>

    <div class="stats-row">
      <div class="stat-card">
        <div class="stat-label">BPM Mínimo</div>
        <div class="stat-value" id="stat-min">--</div>
        <div class="stat-unit">lpm</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">BPM Máximo</div>
        <div class="stat-value" id="stat-max">--</div>
        <div class="stat-unit">lpm</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">Promedio</div>
        <div class="stat-value" id="stat-avg">--</div>
        <div class="stat-unit">lpm</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">Muestras</div>
        <div class="stat-value" id="stat-samples">0</div>
        <div class="stat-unit">pts</div>
      </div>
    </div>

    <div class="demo-notice" id="demo-notice" style="display:none;">
      ⚡ Modo demo activo — conecta tu ESP32 para datos reales
    </div>

  </div>

  <!-- RIGHT -->
  <div class="side-panel">

    <div class="bpm-card zone-normal" id="bpm-card">
      <div class="panel-title">Frecuencia Cardíaca</div>
      <div class="heart-wrapper">
        <div class="pulse-ring"></div>
        <div class="heart">
          <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M12 21.593c-5.63-5.539-11-10.297-11-14.402C1 3.799 3.8 1 7.001 1c1.86 0 3.552.923 4.999 2.757C13.449 1.923 15.14 1 17 1c3.2 0 6 2.799 6 5.191C23 11.296 17.63 16.054 12 21.593z"/>
          </svg>
        </div>
      </div>
      <div class="bpm-value" id="bpm-display">--</div>
      <div class="bpm-unit">latidos por minuto</div>
      <div class="zone-label">
        <span id="zone-icon">⬤</span>
        <span id="zone-text">Sin datos</span>
      </div>
    </div>

    <div class="alert-banner" id="alert-banner">
      <span style="font-size:1.1rem" id="alert-emoji">⚠️</span>
      <div>
        <div style="font-weight:600; margin-bottom:.2rem" id="alert-title">Alerta</div>
        <div id="alert-msg"></div>
      </div>
    </div>

    <div class="zones-card">
      <div class="panel-title" style="margin-bottom:.75rem">Zonas de riesgo</div>
      <div class="zone-row"><div class="zone-dot z-blue"></div><span>Bradicardia</span><span class="zone-range">&lt; 60 BPM</span></div>
      <div class="zone-row"><div class="zone-dot z-green"></div><span>Normal</span><span class="zone-range">60 – 100 BPM</span></div>
      <div class="zone-row"><div class="zone-dot z-yellow"></div><span>Taquicardia leve</span><span class="zone-range">100 – 140 BPM</span></div>
      <div class="zone-row"><div class="zone-dot z-red"></div><span>Taquicardia severa</span><span class="zone-range">140 – 180 BPM</span></div>
    </div>

  </div>
</main>

<!-- ═══════════════════════════════════════
     MODAL DUAL CONEXIÓN
═══════════════════════════════════════ -->
<div class="modal-bg" id="modal">
  <div class="modal">
    <h2>Conectar ESP32</h2>
    <p>Elige el método de conexión. WiFi y BT Serial requieren <strong>Chrome / Edge</strong>.</p>

    <!-- Tabs principales -->
    <div class="tab-bar">
      <button class="tab-btn active" id="tab-wifi-btn"  onclick="switchTab('wifi')">
        📶 WiFi / WebSocket
      </button>
      <button class="tab-btn"        id="tab-bt-btn"    onclick="switchTab('bt')">
        🔵 Bluetooth
      </button>
      <button class="tab-btn"        id="tab-demo-btn"  onclick="startDemo()">
        ⚡ Demo
      </button>
    </div>

    <!-- Tab WiFi -->
    <div class="tab-panel active" id="tab-wifi">
      <div class="form-row">
        <div class="form-group">
          <label>IP del ESP32</label>
          <input type="text" id="ip-input" placeholder="192.168.1.100" value="192.168.1.100" />
        </div>
        <div class="form-group" style="max-width:100px">
          <label>Puerto</label>
          <input type="text" id="port-input" placeholder="81" value="81" />
        </div>
      </div>
      <div class="status-msg" id="wifi-status"></div>
      <button class="btn btn-primary" onclick="connectWifi()">Conectar por WiFi</button>
    </div>

    <!-- Tab Bluetooth -->
    <div class="tab-panel" id="tab-bt">

      <!-- Sub-tabs BT Serial / BLE -->
      <div class="bt-mode-bar">
        <button class="bt-mode-btn active" id="btm-serial" onclick="switchBtMode('serial')">
          🔌 Serial (RFCOMM / COM)
        </button>
        <button class="bt-mode-btn"        id="btm-ble"    onclick="switchBtMode('ble')">
          📡 BLE (Low Energy)
        </button>
      </div>

      <!-- BT Serial -->
      <div class="bt-sub active" id="bts-serial">
        <button class="btn btn-scan" onclick="scanSerial()">🔍 Escanear puertos USB/BT</button>
        <div class="scan-list" id="serial-list"></div>
        <div class="form-group">
          <label>Baudrate</label>
          <select id="serial-baud">
            <option value="9600">9600</option>
            <option value="115200" selected>115200</option>
            <option value="230400">230400</option>
          </select>
        </div>
        <div class="status-msg" id="serial-status"></div>
        <button class="btn btn-primary" onclick="connectSerial()">Conectar Serial</button>
      </div>

      <!-- BLE -->
      <div class="bt-sub" id="bts-ble">
        <button class="btn btn-scan" onclick="scanBLE()">📡 Escanear dispositivos BLE</button>
        <div class="scan-list" id="ble-list"></div>
        <div class="form-group">
          <label>Dirección MAC / UUID</label>
          <input type="text" id="ble-addr" placeholder="D4:E9:F4:E3:3F:C2" />
        </div>
        <div class="status-msg" id="ble-status"></div>
        <button class="btn btn-primary" onclick="connectBLE()">Conectar BLE</button>
      </div>

    </div><!-- /tab-bt -->

    <button class="btn btn-ghost" onclick="disconnectAll()" style="margin-top:.75rem">
      ✖ Desconectar todo
    </button>
    <button class="btn btn-ghost" onclick="closeModal()" style="margin-top:.3rem">Cancelar</button>
  </div>
</div><!-- /modal-bg -->

<script>
/* ═══════════════════════════════════════════
   STATE GLOBAL
═══════════════════════════════════════════ */
let demoInterval  = null;
let isDemo        = false;
let wsConn        = null;   // WebSocket activo
let serialPort    = null;   // Web Serial port
let serialReader  = null;   // Reader activo

const ecgBuffer  = [];
const MAX_ECG    = 800;
let bpmMin = Infinity, bpmMax = -Infinity, bpmSum = 0, bpmCount = 0;
let currentZone  = 'none';
let alarmActive  = false;
let bradyActive  = false;

/* ═══════════════════════════════════════════
   HELPERS UI
═══════════════════════════════════════════ */
function setConnState(state, label) {
  // state: 'connected' | 'connecting' | 'disconnected'
  const dot = document.getElementById('conn-dot');
  const txt = document.getElementById('conn-text');
  dot.className = 'dot' + (state === 'connected' ? ' connected' : state === 'connecting' ? ' connecting' : '');
  txt.textContent = label;
}

/* ═══════════════════════════════════════════
   PACKET HANDLER (común a WiFi y BT)
═══════════════════════════════════════════ */
function handlePacket(d, updateTime = true) {
  if (d.filtered !== undefined) {
    const norm = ((d.filtered / 4095.0) - 0.5) * 2.0;
    pushECG(isNaN(norm) || Math.abs(norm) > 2 ? d.filtered : norm);
  } else if (d.ecg !== undefined) {
    // Normalizar el ADC crudo del ESP32 (0–4095 → -1..1)
    const norm = ((d.ecg / 4095.0) - 0.5) * 2.0;
    pushECG(norm);
  } else if (d.raw !== undefined) {
    pushECG(((d.raw / 4095) - 0.5) * 2);
  }
  if (d.bpm && d.bpm > 0) updateBPM(d.bpm);
  if (updateTime) document.getElementById('timestamp').textContent = new Date().toLocaleTimeString();
}

/* ═══════════════════════════════════════════
   ECG CANVAS
═══════════════════════════════════════════ */
const canvas = document.getElementById('ecg-canvas');
const ctx    = canvas.getContext('2d');

function resizeCanvas() {
  const r = canvas.getBoundingClientRect();
  canvas.width  = r.width  * devicePixelRatio;
  canvas.height = r.height * devicePixelRatio;
  ctx.scale(devicePixelRatio, devicePixelRatio);
}
resizeCanvas();
window.addEventListener('resize', resizeCanvas);

function drawECG() {
  const W = canvas.getBoundingClientRect().width;
  const H = canvas.getBoundingClientRect().height;
  ctx.clearRect(0, 0, W, H);

  // Grid
  ctx.strokeStyle = 'rgba(56,189,248,.07)';
  ctx.lineWidth = 1;
  for (let x = 0; x < W; x += 20) { ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,H); ctx.stroke(); }
  for (let y = 0; y < H; y += 20) { ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke(); }

  if (ecgBuffer.length >= 2) {
    const col = zoneColor();
    [3, 1].forEach((lw, i) => {
      ctx.beginPath();
      ctx.lineWidth   = lw;
      ctx.strokeStyle = i === 0 ? hexAlpha(col, .22) : col;
      ctx.shadowColor = col;
      ctx.shadowBlur  = i === 0 ? 18 : 6;
      const step  = W / MAX_ECG;
      const start = Math.max(0, ecgBuffer.length - MAX_ECG);
      ecgBuffer.slice(start).forEach((v, idx) => {
        const x = idx * step;
        const y = H / 2 - v * H * .42;
        idx === 0 ? ctx.moveTo(x,y) : ctx.lineTo(x,y);
      });
      ctx.stroke();
    });
    ctx.shadowBlur = 0;

    // Cursor glow
    const pts  = Math.min(ecgBuffer.length, MAX_ECG);
    const curX = (pts / MAX_ECG) * W;
    const grad = ctx.createLinearGradient(curX - 30, 0, curX, 0);
    grad.addColorStop(0, 'transparent');
    grad.addColorStop(1, 'rgba(255,255,255,.07)');
    ctx.fillStyle = grad;
    ctx.fillRect(curX - 30, 0, 30, H);
  }

  requestAnimationFrame(drawECG);
}
drawECG();

function zoneColor() {
  if (currentZone === 'brady')   return '#38bdf8';
  if (currentZone === 'normal')  return '#00e676';
  if (currentZone === 'warning') return '#ffea00';
  if (currentZone === 'danger')  return '#ff1744';
  return '#38bdf8';
}
function hexAlpha(hex, a) {
  const r = parseInt(hex.slice(1,3),16), g = parseInt(hex.slice(3,5),16), b = parseInt(hex.slice(5,7),16);
  return `rgba(${r},${g},${b},${a})`;
}

/* ═══════════════════════════════════════════
   BPM + STATS + ZONE
═══════════════════════════════════════════ */
function updateBPM(bpm) {
  bpm = Math.round(bpm);
  document.getElementById('bpm-display').textContent = bpm;
  if (bpm < bpmMin) bpmMin = bpm;
  if (bpm > bpmMax) bpmMax = bpm;
  bpmSum += bpm; bpmCount++;
  document.getElementById('stat-min').textContent     = bpmMin === Infinity ? '--' : bpmMin;
  document.getElementById('stat-max').textContent     = bpmMax === -Infinity ? '--' : bpmMax;
  document.getElementById('stat-avg').textContent     = Math.round(bpmSum / bpmCount);
  document.getElementById('stat-samples').textContent = bpmCount;
  updateZone(bpm);
  playHeartTick();
}

function pushECG(v) {
  ecgBuffer.push(v);
  if (ecgBuffer.length > MAX_ECG * 2) ecgBuffer.splice(0, ecgBuffer.length - MAX_ECG);
}

function updateZone(bpm) {
  const card    = document.getElementById('bpm-card');
  const icon    = document.getElementById('zone-icon');
  const text    = document.getElementById('zone-text');
  const alert   = document.getElementById('alert-banner');
  const aEmoji  = document.getElementById('alert-emoji');
  const fire    = document.getElementById('particle-canvas');
  const ice     = document.getElementById('ice-canvas');

  card.classList.remove('zone-brady','zone-normal','zone-warning','zone-danger');
  alert.classList.remove('visible','brady','warning','danger');
  fire.classList.remove('active');
  ice.classList.remove('active');

  let interval;

  if (bpm < 60) {
    currentZone = 'brady'; card.classList.add('zone-brady');
    icon.textContent = '🔵'; text.textContent = 'Bradicardia';
    interval = 60 / Math.max(bpm, 20);
    alert.classList.add('visible','brady'); aEmoji.textContent = '❄️';
    document.getElementById('alert-title').textContent = 'Frecuencia baja';
    document.getElementById('alert-msg').textContent   = `${bpm} BPM — Ritmo lento detectado. Monitorea al paciente.`;
    ice.classList.add('active');
    playBradySound();

  } else if (bpm <= 100) {
    currentZone = 'normal'; card.classList.add('zone-normal');
    icon.textContent = '🟢'; text.textContent = 'Normal'; interval = 60 / bpm;

  } else if (bpm <= 140) {
    currentZone = 'warning'; card.classList.add('zone-warning');
    icon.textContent = '🟡'; text.textContent = 'Taquicardia Leve'; interval = 60 / bpm;
    alert.classList.add('visible','warning'); aEmoji.textContent = '⚠️';
    document.getElementById('alert-title').textContent = 'Frecuencia elevada';
    document.getElementById('alert-msg').textContent   = `${bpm} BPM — Reposo y monitoreo continuo.`;
    playTone(520, .08, 'sine');

  } else {
    currentZone = 'danger'; card.classList.add('zone-danger');
    icon.textContent = '🔴'; text.textContent = 'Taquicardia Severa'; interval = 60 / bpm;
    alert.classList.add('visible','danger'); aEmoji.textContent = '🚨';
    document.getElementById('alert-title').textContent = '¡Alerta crítica!';
    document.getElementById('alert-msg').textContent   = `${bpm} BPM — Frecuencia peligrosamente alta.`;
    fire.classList.add('active');
    playAlarm();
  }

  document.querySelector('.bpm-card').style.setProperty('--bpm-interval', `${interval}s`);
}

/* ═══════════════════════════════════════════
   AUDIO
═══════════════════════════════════════════ */
let audioCtx = null;
function getAC() { if (!audioCtx) audioCtx = new (window.AudioContext||window.webkitAudioContext)(); return audioCtx; }
function playTone(freq, gain, type, duration) {
  try {
    const ac=getAC(), osc=ac.createOscillator(), gn=ac.createGain();
    osc.type=type||'sine'; osc.frequency.value=freq;
    gn.gain.setValueAtTime(gain, ac.currentTime);
    gn.gain.exponentialRampToValueAtTime(0.0001, ac.currentTime+(duration||.3));
    osc.connect(gn); gn.connect(ac.destination);
    osc.start(); osc.stop(ac.currentTime+(duration||.35));
  } catch(e) {}
}
function playHeartTick() { playTone(80,.05,'sine',.15); }
function playBradySound() {
  if (bradyActive) return; bradyActive=true; setTimeout(()=>{ bradyActive=false; },3000);
  playTone(90,.12,'sine',.4); setTimeout(()=>playTone(60,.08,'sine',.5),300);
}
function playAlarm() {
  if (alarmActive) return; alarmActive=true;
  let count=0;
  const fire=()=>{
    if (currentZone!=='danger'){ alarmActive=false; return; }
    playTone(880,.12,'square',.15);
    setTimeout(()=>playTone(1100,.1,'square',.12),150);
    if (++count<6) setTimeout(fire,550); else alarmActive=false;
  };
  fire();
}

/* ═══════════════════════════════════════════
   DEMO MODE
═══════════════════════════════════════════ */
let demoBPM=45, demoBeatPos=0;
function ecgShape(t) {
  const p=Math.exp(-((t-.15)**2)/.001)*.15, q=Math.exp(-((t-.32)**2)/.0003)*-.1,
        r=Math.exp(-((t-.37)**2)/.00012)*1.0, s=Math.exp(-((t-.43)**2)/.0003)*-.25,
        t_=Math.exp(-((t-.65)**2)/.004)*.35;
  return p+q+r+s+t_+(Math.random()-.5)*.03;
}
function startDemo() {
  closeModal();
  isDemo=true;
  const dot=document.getElementById('conn-dot');
  dot.className='dot connected';
  document.getElementById('conn-text').textContent='Modo demo';
  document.getElementById('demo-notice').style.display='block';

  const seq=[45,45,50,55,60,70,80,90,100,108,120,132,145,158,145,130,115,98,80,65,55,45];
  let seqIdx=0, seqTimer=0;
  const RATE=10;
  demoInterval=setInterval(()=>{
    const period=60/demoBPM, sampBeat=Math.round((period*1000)/RATE);
    const t=(demoBeatPos%sampBeat)/sampBeat;
    pushECG(ecgShape(t)); demoBeatPos++; seqTimer++;
    if (demoBeatPos%sampBeat===0) {
      updateBPM(demoBPM);
      document.getElementById('timestamp').textContent=new Date().toLocaleTimeString();
    }
    if (seqTimer>sampBeat*6) { seqTimer=0; seqIdx=(seqIdx+1)%seq.length; demoBPM=seq[seqIdx]; }
  }, RATE);
}
function stopDemo() { if (demoInterval){ clearInterval(demoInterval); demoInterval=null; } isDemo=false; }

/* ═══════════════════════════════════════════
   PARTICLES — fire (red zone)
═══════════════════════════════════════════ */
const pc=document.getElementById('particle-canvas'), pctx=pc.getContext('2d');
function resizePC(){ pc.width=innerWidth; pc.height=innerHeight; }
resizePC(); window.addEventListener('resize',resizePC);
const fireParticles=Array.from({length:80},()=>newFireP());
function newFireP(){ return {x:Math.random()*innerWidth,y:innerHeight+10,vx:(Math.random()-.5)*1.5,vy:-(Math.random()*2+1),r:Math.random()*3+1,life:Math.random()}; }
function animFire(){
  pctx.clearRect(0,0,pc.width,pc.height);
  if(pc.classList.contains('active')){
    fireParticles.forEach(p=>{
      p.x+=p.vx; p.y+=p.vy; p.life-=.008;
      if(p.life<=0) Object.assign(p,newFireP());
      pctx.beginPath(); pctx.arc(p.x,p.y,p.r,0,Math.PI*2);
      pctx.fillStyle=`rgba(255,23,68,${p.life*.5})`; pctx.fill();
    });
  }
  requestAnimationFrame(animFire);
}
animFire();

/* ═══════════════════════════════════════════
   ICE PARTICLES — brady zone
═══════════════════════════════════════════ */
const ic=document.getElementById('ice-canvas'), ictx=ic.getContext('2d');
function resizeIC(){ ic.width=innerWidth; ic.height=innerHeight; }
resizeIC(); window.addEventListener('resize',resizeIC);
const iceParticles=Array.from({length:60},()=>newIceP());
function newIceP(){ return {x:Math.random()*innerWidth,y:-10,vx:(Math.random()-.5)*.8,vy:Math.random()*.8+.4,r:Math.random()*3+1,life:1,drift:Math.random()*Math.PI*2}; }
let iceFrame=0;
function animIce(){
  ictx.clearRect(0,0,ic.width,ic.height);
  if(ic.classList.contains('active')){
    iceFrame++;
    iceParticles.forEach(p=>{
      p.x+=p.vx+Math.sin(iceFrame*.02+p.drift)*.4; p.y+=p.vy; p.life-=.003;
      if(p.life<=0||p.y>ic.height+10) Object.assign(p,newIceP());
      ictx.save(); ictx.translate(p.x,p.y); ictx.rotate(iceFrame*.01);
      ictx.strokeStyle=`rgba(56,189,248,${p.life*.6})`; ictx.lineWidth=.8;
      for(let i=0;i<6;i++){ ictx.beginPath(); ictx.moveTo(0,0); ictx.lineTo(0,p.r*2); ictx.stroke(); ictx.rotate(Math.PI/3); }
      ictx.restore();
    });
  }
  requestAnimationFrame(animIce);
}
animIce();

/* ═══════════════════════════════════════════
   MODAL helpers
═══════════════════════════════════════════ */
function openModal()  { document.getElementById('modal').classList.remove('hidden'); }
function closeModal() { document.getElementById('modal').classList.add('hidden'); }

function switchTab(tab) {
  ['wifi','bt'].forEach(t=>{
    document.getElementById(`tab-${t}`).classList.toggle('active', t===tab);
    document.getElementById(`tab-${t}-btn`).classList.toggle('active', t===tab);
  });
}

let btMode = 'serial';
function switchBtMode(mode) {
  btMode = mode;
  ['serial','ble'].forEach(m=>{
    document.getElementById(`bts-${m}`).classList.toggle('active', m===mode);
    document.getElementById(`btm-${m}`).classList.toggle('active', m===mode);
  });
}

/* ═══════════════════════════════════════════
   WIFI — WebSocket directo al ESP32 (puerto 81)
═══════════════════════════════════════════ */
function connectWifi() {
  const ip   = document.getElementById('ip-input').value.trim();
  const port = document.getElementById('port-input').value.trim() || '81';
  const st   = document.getElementById('wifi-status');

  // Cerrar conexión anterior si existe
  if (wsConn) { wsConn.close(); wsConn = null; }
  stopDemo();

  st.textContent = 'Conectando…';
  setConnState('connecting', 'Conectando WiFi…');

  const url = `ws://${ip}:${port}`;
  const ws  = new WebSocket(url);

  ws.onopen = () => {
    wsConn = ws;
    st.textContent = '✓ Conectado';
    setConnState('connected', `WiFi · ${ip}`);
    setTimeout(closeModal, 700);
  };

  ws.onmessage = ev => {
    try {
      const d = JSON.parse(ev.data);
      handlePacket(d);
    } catch(e) {}
  };

  ws.onerror = () => {
    st.textContent = '✗ Error — revisa IP y puerto';
    setConnState('disconnected', 'Desconectado');
  };

  ws.onclose = () => {
    if (wsConn === ws) {
      wsConn = null;
      setConnState('disconnected', 'Desconectado');
    }
  };
}

/* ═══════════════════════════════════════════
   BLUETOOTH SERIAL — Web Serial API
   El ESP32 envía tramas de 3 bytes: [0xFF, bpmLo, bpmHi]
   También parsea JSON si el byte no es 0xFF
═══════════════════════════════════════════ */
async function scanSerial() {
  const st   = document.getElementById('serial-status');
  const list = document.getElementById('serial-list');

  if (!('serial' in navigator)) {
    st.textContent = '⚠ Web Serial API no soportada en este browser. Usa Chrome/Edge.';
    return;
  }

  st.textContent = 'Selecciona el puerto en el diálogo…';
  list.innerHTML = '';

  try {
    // El navegador abre el selector nativo — mostramos el resultado
    const port = await navigator.serial.requestPort();
    const info = port.getInfo();
    list.innerHTML = '';
    const el = document.createElement('div');
    el.className = 'scan-item';
    el.innerHTML = `<span class="dev-name">ESP32_ECG</span>
                    <span class="dev-addr">VID:${info.usbVendorId ?? '–'} PID:${info.usbProductId ?? '–'}</span>`;
    // Guardar referencia en el elemento para usarla al conectar
    el._port = port;
    el.onclick = () => {
      el._selected = true;
      list.querySelectorAll('.scan-item').forEach(e => e.style.background = '');
      el.style.background = 'rgba(56,189,248,.15)';
      // Almacenar puerto seleccionado
      scanSerial._selectedPort = port;
      list.classList.remove('visible');
      st.textContent = 'Puerto seleccionado ✓';
    };
    list.appendChild(el);
    list.classList.add('visible');
    st.textContent = '1 puerto encontrado — haz clic para seleccionar';
    scanSerial._selectedPort = port; // preseleccionar
  } catch(e) {
    if (e.name === 'NotFoundError') st.textContent = 'Sin puertos — cancela o intenta de nuevo';
    else st.textContent = `Error: ${e.message}`;
  }
}

async function connectSerial() {
  const st   = document.getElementById('serial-status');
  const baud = parseInt(document.getElementById('serial-baud').value) || 115200;

  if (!('serial' in navigator)) {
    st.textContent = '⚠ Web Serial API no disponible. Usa Chrome/Edge.';
    return;
  }

  // Intentar usar el puerto ya seleccionado, o pedir uno nuevo
  let port = scanSerial._selectedPort;
  if (!port) {
    try { port = await navigator.serial.requestPort(); }
    catch(e) { st.textContent = 'Sin puerto seleccionado'; return; }
  }

  st.textContent = 'Conectando…';
  stopDemo();

  try {
    await port.open({ baudRate: baud });
  } catch(e) {
    st.textContent = `Error al abrir: ${e.message}`;
    return;
  }

  serialPort = port;
  setConnState('connected', 'Bluetooth Serial');
  st.textContent = '✓ Conectado';
  setTimeout(closeModal, 700);

  // Leer bytes en segundo plano
  readSerialLoop(port);
}

async function readSerialLoop(port) {
  const decoder = new TextDecoder();
  let textBuf = '';
  let binBuf  = [];

  try {
    serialReader = port.readable.getReader();
    while (true) {
      const { value, done } = await serialReader.read();
      if (done) break;

      for (const byte of value) {
        // ── Protocolo binario: 0xFF + bpmLo + bpmHi ──
        if (byte === 0xFF) {
          binBuf = [byte];
        } else if (binBuf.length === 1) {
          binBuf.push(byte);
        } else if (binBuf.length === 2) {
          binBuf.push(byte);
          const bpm = binBuf[1] | (binBuf[2] << 8);
          binBuf = [];
          if (bpm > 20 && bpm < 300) handlePacket({ bpm });

        // ── Protocolo JSON (fallback) ──
        } else {
          const ch = String.fromCharCode(byte);
          textBuf += ch;
          if (ch === '\n') {
            const line = textBuf.trim();
            textBuf = '';
            if (line.startsWith('{')) {
              try { handlePacket(JSON.parse(line)); } catch(e) {}
            } else {
              const n = parseInt(line);
              if (!isNaN(n) && n > 20 && n < 300) handlePacket({ bpm: n });
            }
          }
          if (textBuf.length > 256) textBuf = '';
        }
      }
    }
  } catch(e) {
    // Conexión cerrada
  } finally {
    if (serialReader) { try { serialReader.releaseLock(); } catch(e) {} serialReader = null; }
    if (serialPort === port) { serialPort = null; }
    setConnState('disconnected', 'Desconectado');
  }
}

/* BLE — Web Bluetooth API */
async function scanBLE() {
  const st   = document.getElementById('ble-status');
  const list = document.getElementById('ble-list');

  if (!('bluetooth' in navigator)) {
    st.textContent = '⚠ Web Bluetooth no disponible en este browser/SO.';
    return;
  }

  st.textContent = 'Abriendo selector de dispositivos BLE…';
  list.innerHTML = ''; list.classList.remove('visible');

  try {
    const device = await navigator.bluetooth.requestDevice({
      acceptAllDevices: true,
      optionalServices: ['0000ffe0-0000-1000-8000-00805f9b34fb']
    });
    const el = document.createElement('div');
    el.className = 'scan-item';
    el.innerHTML = `<span class="dev-name">${device.name || '(sin nombre)'}</span>
                    <span class="dev-addr">${device.id}</span>`;
    el.onclick = () => {
      document.getElementById('ble-addr').value = device.id;
      scanBLE._device = device;
      list.classList.remove('visible');
      st.textContent = 'Dispositivo seleccionado ✓';
    };
    list.appendChild(el);
    list.classList.add('visible');
    scanBLE._device = device;
    st.textContent = '1 dispositivo encontrado — haz clic para seleccionar';
  } catch(e) {
    st.textContent = e.name === 'NotFoundError' ? 'Sin dispositivos / cancelado' : `Error: ${e.message}`;
  }
}

async function connectBLE() {
  const st = document.getElementById('ble-status');

  if (!('bluetooth' in navigator)) {
    st.textContent = '⚠ Web Bluetooth no disponible.';
    return;
  }

  const device = scanBLE._device;
  if (!device) { st.textContent = '⚠ Primero escanea y selecciona un dispositivo'; return; }

  st.textContent = 'Conectando BLE…';
  stopDemo();

  try {
    const server  = await device.gatt.connect();
    const service = await server.getPrimaryService('0000ffe0-0000-1000-8000-00805f9b34fb');
    const char    = await service.getCharacteristic('0000ffe1-0000-1000-8000-00805f9b34fb');

    await char.startNotifications();
    char.addEventListener('characteristicvaluechanged', ev => {
      const view = ev.target.value;
      // Protocolo binario igual que serial: 0xFF + Lo + Hi
      if (view.getUint8(0) === 0xFF && view.byteLength >= 3) {
        const bpm = view.getUint8(1) | (view.getUint8(2) << 8);
        if (bpm > 20 && bpm < 300) handlePacket({ bpm });
      } else {
        // Intentar parsear como texto
        const txt = new TextDecoder().decode(view).trim();
        try { handlePacket(JSON.parse(txt)); } catch(e) {
          const n = parseInt(txt);
          if (!isNaN(n) && n > 20 && n < 300) handlePacket({ bpm: n });
        }
      }
    });

    setConnState('connected', `BLE · ${device.name || device.id}`);
    st.textContent = '✓ Conectado por BLE';
    setTimeout(closeModal, 700);

    device.addEventListener('gattserverdisconnected', () => {
      setConnState('disconnected', 'Desconectado');
    });
  } catch(e) {
    st.textContent = `Error BLE: ${e.message}`;
    setConnState('disconnected', 'Desconectado');
  }
}

/* ═══════════════════════════════════════════
   DESCONECTAR TODO
═══════════════════════════════════════════ */
async function disconnectAll() {
  stopDemo();

  if (wsConn) { wsConn.close(); wsConn = null; }

  if (serialReader) {
    try { await serialReader.cancel(); serialReader.releaseLock(); } catch(e) {}
    serialReader = null;
  }
  if (serialPort) {
    try { await serialPort.close(); } catch(e) {}
    serialPort = null;
  }

  setConnState('disconnected', 'Desconectado');
  document.getElementById('demo-notice').style.display = 'none';
  closeModal();
}

/* Abrir modal al cargar */
window.addEventListener('load', openModal);
</script>
</body>
</html>
