/* Protography phone app — plain JS, no dependencies, served by the camera. */
"use strict";

const $ = (id) => document.getElementById(id);

const MODES = ["HQ", "RAW", "MOSH", "BEND"];
const HINTS = [
  "Clean 5MP JPEG. The glitch slider still applies if you want a subtle touch.",
  "Raw Bayer capture. Download the .DNG from the gallery — no glitching, pristine data.",
  "JPEG bytes corrupted in-camera: smears, tears and macroblocks. Seedable and reproducible.",
  "Circuit-bent sensor: gain/exposure chaos, corrections off, wrong white balance — then moshed.",
];

const state = {
  status: null,
  photos: [],
  busy: false,
};

/* ---------------- helpers ---------------- */

let toastTimer = null;
function toast(msg, ms = 2600) {
  const t = $("toast");
  t.textContent = msg;
  t.classList.remove("hidden");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.add("hidden"), ms);
}

async function api(path, opts) {
  const r = await fetch(path, opts);
  if (!r.ok) throw new Error(`${path}: ${r.status}`);
  return r;
}

async function apiJson(path, opts) {
  return (await api(path, opts)).json();
}

/* ---------------- websocket ---------------- */

let ws = null;
let wsRetryMs = 800;
let liveUrl = null;

function wsConnect() {
  const proto = location.protocol === "https:" ? "wss:" : "ws:";
  ws = new WebSocket(`${proto}//${location.hostname}:81/`);
  ws.binaryType = "arraybuffer";
  ws.onopen = () => {
    wsRetryMs = 800;
    send({ cmd: "hello" });
    send({ cmd: "status" });
  };
  ws.onclose = () => {
    $("ind-ws").className = "dot bad";
    setTimeout(wsConnect, wsRetryMs);
    wsRetryMs = Math.min(wsRetryMs * 1.6, 8000);
  };
  ws.onmessage = (ev) => {
    if (ev.data instanceof ArrayBuffer) onBinary(ev.data);
    else onEvent(JSON.parse(ev.data));
  };
}

function send(obj) {
  if (ws && ws.readyState === 1) ws.send(JSON.stringify(obj));
}

function onBinary(buf) {
  const u8 = new Uint8Array(buf);
  if (u8[0] !== 0x01) return;   // live JPEG frame
  const blob = new Blob([u8.subarray(1)], { type: "image/jpeg" });
  if (liveUrl) URL.revokeObjectURL(liveUrl);
  liveUrl = URL.createObjectURL(blob);
  $("live").src = liveUrl;
}

function onEvent(ev) {
  if (ev.event === "hello") {
    $("ind-ws").className = "dot ok";
    $("about").textContent =
      `Protography ${ev.fw} — ${ev.name}${ev.cam_ok ? "" : " — CAMERA ERROR (check wiring)"}`
      + ` — this page: ${location.host}`;
  } else if (ev.event === "status") {
    state.status = ev;
    renderStatus();
  } else if (ev.event === "photo") {
    state.busy = false;
    $("busy").classList.add("hidden");
    const link = $("last-link");
    link.href = `/api/photo/${ev.id}.jpg`;
    link.download = `protography_${ev.id}.jpg`;
    $("last-shot").classList.remove("hidden");
    toast(`SAVED #${ev.id} (${MODES[ev.mode]})`);
    loadPhotos();
  } else if (ev.event === "error") {
    state.busy = false;
    $("busy").classList.add("hidden");
    toast(`ERROR: ${ev.msg}`);
  }
}

/* ---------------- rendering ---------------- */

function renderStatus() {
  const s = state.status;
  if (!s) return;

  $("ind-cam").className = "dot " + (s.cam_ok ? "ok" : "bad");
  const af = s.af || "n/a";
  $("ind-af").className =
    "dot " + (af === "focused" ? "ok" : af === "n/a" ? "warn" : "");

  const pct = s.store.budget ? Math.min(100, (s.store.used / s.store.budget) * 100) : 0;
  $("store-bar").firstElementChild.style.width = pct.toFixed(0) + "%";

  const liveOn = s.live && s.live.on;
  $("live-wrap").classList.toggle("off", !liveOn);
  $("live-toggle").classList.toggle("on", liveOn);
  $("ind-ws").className = "dot " + (liveOn && s.live.clients ? "ok" : s.live.clients ? "ok" : "warn");

  setMode(s.settings ? s.settings.mode : 0, false);
}

function setMode(mode, push = true) {
  document.querySelectorAll("#modes .chip").forEach((b) => {
    b.classList.toggle("active", +b.dataset.mode === mode);
  });
  $("mode-tag").textContent = MODES[mode];
  $("fx-hint").textContent = HINTS[mode];
  if (push) saveSettings({ mode });
}

async function saveSettings(patch) {
  try {
    const s = await apiJson("/api/settings", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(patch),
    });
    state.status = state.status || {};
    state.status.settings = s;
    toast("SAVED");
  } catch (e) {
    toast("could not save settings");
  }
}

async function loadPhotos() {
  try {
    state.photos = await apiJson("/api/photos");
    renderGallery();
  } catch (e) {
    toast("could not load gallery");
  }
}

function fmtKB(n) {
  return n > 1024 * 1024 ? (n / 1048576).toFixed(1) + " MB" : Math.round(n / 1024) + " KB";
}

function renderGallery() {
  const grid = $("gallery");
  grid.innerHTML = "";
  $("gallery-empty").classList.toggle("hidden", state.photos.length > 0);
  for (const p of state.photos) {
    const b = document.createElement("button");
    const img = document.createElement("img");
    img.loading = "lazy";
    img.src = `/api/photo/${p.id}.jpg`;
    const tag = document.createElement("span");
    tag.className = "tag";
    tag.textContent = MODES[p.mode] + (p.sd ? " ·SD" : "") + (!p.in_ram ? " ·COLD" : "");
    b.append(img, tag);
    b.onclick = () => openDetail(p);
    grid.append(b);
  }
}

function openDetail(p) {
  $("detail-img").src = `/api/photo/${p.id}.jpg`;
  $("detail-meta").innerHTML =
    `#${p.id} — ${MODES[p.mode]} — ${p.jw}×${p.jh} — ${fmtKB(p.jlen)}<br>` +
    (p.rlen ? `RAW ${p.rw}×${p.rh} — ${fmtKB(p.rlen)}<br>` : "") +
    (p.seed ? `seed ${p.seed} · intensity ${p.intensity}<br>` : "") +
    `boot ${p.boot} · t+${(p.ts / 1000).toFixed(1)}s<br>` +
    (p.in_ram ? "hot: in camera RAM" : "cold: on SD card");

  const jpg = $("detail-jpg");
  jpg.href = `/api/photo/${p.id}.jpg`;
  jpg.download = `protography_${p.id}.jpg`;

  const dng = $("detail-dng");
  if (p.rlen) {
    dng.classList.remove("hidden");
    dng.href = `/api/photo/${p.id}.dng`;
    dng.download = `protography_${p.id}.dng`;
  } else {
    dng.classList.add("hidden");
  }

  $("detail-del").onclick = async () => {
    try {
      await api(`/api/photo/${p.id}`, { method: "DELETE" });
      $("detail").classList.add("hidden");
      loadPhotos();
    } catch (e) {
      toast("delete failed");
    }
  };
  $("detail").classList.remove("hidden");
}

/* ---------------- shoot controls ---------------- */

function doCapture() {
  if (state.busy) return;
  state.busy = true;
  $("busy").classList.remove("hidden");
  const seed = parseInt($("seed").value, 10) || 0;
  const intensity = parseInt($("intensity").value, 10) || 0;
  const mode = state.status && state.status.settings ? state.status.settings.mode : 0;
  send({ cmd: "capture", mode, seed, intensity });
  // Fallback if the socket died: HTTP capture after a short grace period.
  setTimeout(async () => {
    if (!state.busy) return;
    try {
      await apiJson("/api/capture", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ mode, seed, intensity }),
      });
      state.busy = false;
      $("busy").classList.add("hidden");
      loadPhotos();
    } catch (e) {
      if (state.busy) {
        state.busy = false;
        $("busy").classList.add("hidden");
        toast("capture failed");
      }
    }
  }, 1500);
}

function randomSeed() {
  $("seed").value = Math.floor(Math.random() * 0xFFFFFFFF);
}

/* ---------------- settings form ---------------- */

function fillSettingsForm(s) {
  if (!s) return;
  $("set-quality").value = s.jpeg_quality;
  $("set-quality-out").textContent = s.jpeg_quality;
  $("set-hmirror").checked = !!s.hmirror;
  $("set-vflip").checked = !!s.vflip;
  $("set-livesize").value = s.live_framesize;
  $("set-fps").value = s.live_fps;
  $("set-fps-out").textContent = s.live_fps;
  $("set-sleep").value = s.sleep_min;
  $("set-apname").value = s.ap_name || "";
  $("set-appass").value = "";
}

/* ---------------- boot ---------------- */

function bindUi() {
  document.querySelectorAll("#tabbar button").forEach((b) => {
    b.onclick = () => {
      document.querySelectorAll("#tabbar button").forEach((x) => x.classList.remove("active"));
      b.classList.add("active");
      document.querySelectorAll(".tab").forEach((t) => t.classList.remove("active"));
      $(`view-${b.dataset.tab}`).classList.add("active");
      if (b.dataset.tab === "gallery") loadPhotos();
    };
  });

  document.querySelectorAll("#modes .chip").forEach((b) => {
    b.onclick = () => setMode(+b.dataset.mode);
  });

  $("shutter").onclick = doCapture;
  $("focus").onclick = () => {
    send({ cmd: "focus" });
    toast("FOCUSING…", 1200);
  };
  $("live-toggle").onclick = () => {
    const on = !$("live-toggle").classList.contains("on");
    send({ cmd: "live", on });
  };
  $("dice").onclick = randomSeed;
  $("intensity").oninput = (e) => ($("int-out").textContent = e.target.value);

  $("detail-close").onclick = () => $("detail").classList.add("hidden");

  $("set-quality").oninput = (e) => ($("set-quality-out").textContent = e.target.value);
  $("set-fps").oninput = (e) => ($("set-fps-out").textContent = e.target.value);
  $("set-save").onclick = () => {
    const patch = {
      jpeg_quality: +$("set-quality").value,
      hmirror: $("set-hmirror").checked,
      vflip: $("set-vflip").checked,
      live_framesize: +$("set-livesize").value,
      live_fps: +$("set-fps").value,
      sleep_min: +$("set-sleep").value || 0,
    };
    if ($("set-apname").value.trim() !== "" || $("set-appass").value !== "") {
      // Empty password would reopen the network — only send when the user
      // typed something in either field.
      patch.ap_name = $("set-apname").value.trim();
      if ($("set-appass").value !== "") patch.ap_pass = $("set-appass").value;
    }
    saveSettings(patch).then(() => toast("Saved — Wi-Fi name applies on reboot"));
  };
  $("set-reboot").onclick = async () => {
    await api("/api/reboot", { method: "POST" }).catch(() => {});
    toast("Rebooting… reconnect in ~10 s");
    setTimeout(() => location.reload(), 8000);
  };
  $("set-sleep-now").onclick = () => {
    send({ cmd: "sleep" });
    toast("Camera going to sleep — press the shutter to wake");
  };
}

async function init() {
  bindUi();
  randomSeed();
  try {
    state.status = await apiJson("/api/status");
    fillSettingsForm(state.status.settings);
    renderStatus();
  } catch (e) {
    toast("device not responding — reload");
  }
  await loadPhotos();
  wsConnect();
}

init();
