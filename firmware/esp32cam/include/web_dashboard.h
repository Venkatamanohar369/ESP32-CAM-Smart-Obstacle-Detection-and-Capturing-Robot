#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

static const char WEB_DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>ESP32-CAM Surveillance Robot</title>
  <style>
    :root { --bg:#0f172a; --card:#1e293b; --border:#334155; --text:#f8fafc; --muted:#94a3b8; --green:#16a34a; --olive:#65a30d; --red:#dc2626; --blue:#2563eb; --amber:#d97706; }
    * { box-sizing:border-box; margin:0; padding:0; }
    body { font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif; background:var(--bg); color:var(--text); padding:12px; max-width:1100px; margin:0 auto; }
    header { display:flex; flex-wrap:wrap; justify-content:space-between; align-items:center; gap:8px; margin-bottom:12px; padding-bottom:10px; border-bottom:1px solid var(--border); }
    h1 { font-size:1.15rem; font-weight:700; }
    .badges { display:flex; flex-wrap:wrap; gap:6px; }
    .badge { font-size:0.78rem; padding:4px 10px; border-radius:999px; background:var(--card); border:1px solid var(--border); color:var(--muted); }
    .badge.ok { border-color:var(--green); color:#86efac; }
    .badge.warn { border-color:var(--amber); color:#fcd34d; }
    .badge.err { border-color:var(--red); color:#fca5a5; }
    .alert-banner { display:none; background:#7f1d1d; border:1px solid var(--red); color:#fecaca; padding:10px 14px; border-radius:8px; font-weight:600; margin-bottom:12px; text-align:center; }
    .alert-banner.active { display:block; }
    .telemetry-bar { display:grid; grid-template-columns:repeat(auto-fit,minmax(140px,1fr)); gap:8px; margin-bottom:12px; }
    .stat { background:var(--card); border:1px solid var(--border); border-radius:8px; padding:10px; }
    .stat-label { font-size:0.72rem; color:var(--muted); text-transform:uppercase; }
    .stat-val { font-size:1.15rem; font-weight:700; margin-top:2px; }
    .media-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(300px,1fr)); gap:12px; margin-bottom:12px; }
    .panel { background:var(--card); border:1px solid var(--border); border-radius:10px; padding:10px; }
    .panel-title { display:flex; justify-content:space-between; align-items:center; font-size:0.85rem; font-weight:700; margin-bottom:8px; color:#e2e8f0; }
    .dist-tag { color:#4ade80; font-weight:700; font-size:0.85rem; }
    .frame-box { width:100%; aspect-ratio:4/3; background:#020617; border-radius:6px; overflow:hidden; display:flex; align-items:center; justify-content:center; color:var(--muted); font-size:0.85rem; }
    .frame-box img { width:100%; height:100%; object-fit:contain; display:block; }
    .controls { background:var(--card); border:1px solid var(--border); border-radius:10px; padding:12px; margin-bottom:12px; }
    .btn-row { display:grid; grid-template-columns:repeat(5,1fr); gap:8px; margin-bottom:10px; }
    @media (max-width:600px) { .btn-row { grid-template-columns:repeat(3,1fr); } }
    button { border:none; border-radius:8px; padding:12px 8px; font-size:0.92rem; font-weight:700; color:#fff; cursor:pointer; touch-action:manipulation; user-select:none; }
    button:active { opacity:0.8; transform:scale(0.98); }
    .btn-fwd { background:var(--green); }
    .btn-left,.btn-right { background:var(--olive); }
    .btn-stop { background:var(--red); }
    .btn-back { background:var(--blue); }
    .btn-sec { background:#334155; padding:9px 12px; font-size:0.84rem; }
    .sub-controls { display:flex; flex-wrap:wrap; align-items:center; justify-content:space-between; gap:10px; }
    .speed-wrap { display:flex; align-items:center; gap:8px; font-size:0.85rem; }
    .action-btns { display:flex; flex-wrap:wrap; gap:8px; }
    .gallery-grid { display:grid; grid-template-columns:repeat(auto-fill,minmax(160px,1fr)); gap:10px; margin-top:8px; }
    .thumb { background:#0f172a; border:1px solid var(--border); border-radius:8px; overflow:hidden; font-size:0.75rem; }
    .thumb img { width:100%; aspect-ratio:4/3; object-fit:cover; display:block; }
    .thumb-meta { padding:6px 8px; color:var(--muted); line-height:1.35; }
    .thumb-meta strong { color:#4ade80; }
  </style>
</head>
<body>
  <header>
    <h1>ESP32-CAM Autonomous Surveillance Robot</h1>
    <div class="badges">
      <span id="bConn" class="badge">Connecting...</span>
      <span id="bCam" class="badge">Camera: --</span>
      <span id="bNano" class="badge">Nano: --</span>
      <span id="bTg" class="badge">Telegram: --</span>
    </div>
  </header>
  <div id="obsBanner" class="alert-banner">OBSTACLE EMERGENCY STOP ACTIVE - Forward motion blocked!</div>
  <div class="telemetry-bar">
    <div class="stat"><div class="stat-label">Ultrasonic Distance</div><div id="vDist" class="stat-val">-- cm</div></div>
    <div class="stat"><div class="stat-label">PIR Motion</div><div id="vPir" class="stat-val">Idle</div></div>
    <div class="stat"><div class="stat-label">Motor State</div><div id="vMotor" class="stat-val">STOP (0)</div></div>
    <div class="stat"><div class="stat-label">Last Capture</div><div id="vCapTime" class="stat-val">None</div></div>
  </div>
  <div class="media-grid">
    <div class="panel">
      <div class="panel-title"><span>LIVE STREAM</span><span style="color:var(--muted);font-weight:400;">Port 81 MJPEG</span></div>
      <div class="frame-box"><img id="liveImg" alt="Live camera stream unavailable" /></div>
    </div>
    <div class="panel">
      <div class="panel-title"><span>CAPTURED IMAGE</span><span id="capDist" class="dist-tag">Distance: --</span></div>
      <div class="frame-box"><img id="capImg" alt="No captured image yet" /></div>
    </div>
  </div>
  <div class="controls">
    <div class="btn-row">
      <button class="btn-fwd" data-dir="F">Forward</button>
      <button class="btn-left" data-dir="L">Left</button>
      <button class="btn-stop" data-dir="S">Stop</button>
      <button class="btn-right" data-dir="R">Right</button>
      <button class="btn-back" data-dir="B">Back</button>
    </div>
    <div class="sub-controls">
      <label class="speed-wrap">Speed:<input id="speedRange" type="range" min="90" max="255" value="180" /><span id="speedVal">180</span></label>
      <div class="action-btns">
        <button class="btn-sec" onclick="triggerCapture()">Capture Photo</button>
        <button class="btn-sec" onclick="beepBuzzer()">Beep Buzzer</button>
        <button class="btn-sec" id="flashBtn" onclick="toggleFlash()">Flash: OFF</button>
      </div>
    </div>
  </div>
  <div class="panel">
    <div class="panel-title"><span>CAPTURED IMAGE GALLERY</span><span style="color:var(--muted);font-weight:400;">Latest 6 in PSRAM</span></div>
    <div id="gallery" class="gallery-grid"></div>
  </div>
  <script>
    const liveImg=document.getElementById('liveImg'),capImg=document.getElementById('capImg'),speedRange=document.getElementById('speedRange'),speedVal=document.getElementById('speedVal');
    let flashOn=false,activeDir='S',holdTimer=null,lastCapId=0;
    if(location.protocol.startsWith('http')&&location.hostname){liveImg.src=`${location.protocol}//${location.hostname}:81/stream`;}
    speedRange.addEventListener('input',()=>{speedVal.textContent=speedRange.value;});
    async function sendMove(d){try{await fetch(`/api/move?dir=${encodeURIComponent(d)}&speed=${encodeURIComponent(speedRange.value)}`,{method:'POST'});}catch(e){}}
    function startHold(d){activeDir=d;sendMove(d);clearInterval(holdTimer);if(d!=='S'){holdTimer=setInterval(()=>sendMove(activeDir),400);}}
    function stopHold(){clearInterval(holdTimer);holdTimer=null;if(activeDir!=='S'){activeDir='S';sendMove('S');}}
    document.querySelectorAll('button[data-dir]').forEach(b=>{const d=b.dataset.dir;b.addEventListener('pointerdown',e=>{e.preventDefault();startHold(d);});b.addEventListener('pointerup',()=>{if(d!=='S')stopHold();});b.addEventListener('pointercancel',()=>{if(d!=='S')stopHold();});});
    document.addEventListener('keydown',e=>{if(e.repeat)return;const m={ArrowUp:'F',w:'F',W:'F',ArrowDown:'B',s:'B',S:'B',ArrowLeft:'L',a:'L',A:'L',ArrowRight:'R',d:'R',D:'R',' ':'S'};if(m[e.key]){e.preventDefault();startHold(m[e.key]);}});
    document.addEventListener('keyup',e=>{if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','w','a','s','d','W','A','S','D'].includes(e.key))stopHold();});
    async function triggerCapture(){await fetch('/api/capture',{method:'POST'});pollStatus();}
    async function beepBuzzer(){await fetch('/api/buzzer?mode=1',{method:'POST'});}
    async function toggleFlash(){flashOn=!flashOn;document.getElementById('flashBtn').textContent=`Flash: ${flashOn?'ON':'OFF'}`;await fetch(`/api/flash?on=${flashOn?1:0}`,{method:'POST'});}
    function fmt(ms){const s=Math.floor(ms/1000);return `${Math.floor(s/60)}m ${s%60}s`;}
    function renderGallery(caps){
      const g=document.getElementById('gallery');g.innerHTML='';
      if(!caps||caps.length===0){g.innerHTML='<div style="color:var(--muted);font-size:0.82rem;">No images captured yet.</div>';return;}
      const l=caps[0];
      if(l.id!==lastCapId){lastCapId=l.id;capImg.src=`/api/photo?id=${l.id}&t=${Date.now()}`;document.getElementById('capDist').textContent=l.distMm>=0?`Distance: ${(l.distMm/10).toFixed(1)} cm`:'Distance: N/A';document.getElementById('vCapTime').textContent=`${l.reason} @ ${fmt(l.tsMs)}`;}
      caps.forEach(c=>{const d=document.createElement('div');d.className='thumb';const ds=c.distMm>=0?`${(c.distMm/10).toFixed(1)} cm`:'N/A';d.innerHTML=`<img src="/api/photo?id=${c.id}" loading="lazy" alt="Capture ${c.id}" /><div class="thumb-meta"><strong>${ds}</strong> | ${c.reason}<br/>Uptime: ${fmt(c.tsMs)}</div>`;g.appendChild(d);});
    }
    async function pollStatus(){
      try{
        const r=await fetch('/api/status');if(!r.ok)throw new Error();
        const st=await r.json();
        const bc=document.getElementById('bConn');bc.textContent=`Wi-Fi: ${st.wifiRssi} dBm`;bc.className='badge ok';
        const bm=document.getElementById('bCam');bm.textContent=st.camOk?'Camera: OV3660 OK':'Camera: ERROR';bm.className='badge '+(st.camOk?'ok':'err');
        const bn=document.getElementById('bNano');bn.textContent=st.nanoOnline?'Nano: ONLINE':'Nano: OFFLINE';bn.className='badge '+(st.nanoOnline?'ok':'warn');
        const bt=document.getElementById('bTg');bt.textContent=st.tgEnabled?`TG: ${st.tgStatus}`:'TG: Disabled';bt.className='badge '+(st.tgEnabled?(st.tgOk?'ok':'warn'):'');
        document.getElementById('vDist').textContent=st.distMm>=0?`${(st.distMm/10).toFixed(1)} cm`:'Timeout';
        document.getElementById('vPir').textContent=st.pir?'MOTION DETECTED':'Idle';
        document.getElementById('vPir').style.color=st.pir?'#fcd34d':'#f8fafc';
        document.getElementById('vMotor').textContent=`${st.motorDir} (${st.pwm})`;
        document.getElementById('obsBanner').className='alert-banner'+(st.obs?' active':'');
        renderGallery(st.captures);
      }catch(e){const bc=document.getElementById('bConn');bc.textContent='Disconnected';bc.className='badge err';}
    }
    setInterval(pollStatus,800);pollStatus();
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
