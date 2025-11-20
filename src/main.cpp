#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

const int LED_PIN = 2;

WebServer server(80);

// Modern single-file HTML/CSS/JS page (embedded)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32 LED Control</title>
  <style>
    :root{--bg:#0f1724;--card:#0b1220;--accent:#7c3aed;--muted:#94a3b8}
    *{box-sizing:border-box;font-family:Inter,Segoe UI,Roboto,Arial}
    html,body{height:100%;margin:0;background:linear-gradient(180deg,#071028 0%,#081226 60%);color:#e6eef8}
    .wrap{min-height:100%;display:flex;align-items:center;justify-content:center;padding:24px}
    .card{background:linear-gradient(180deg,rgba(255,255,255,0.02),rgba(0,0,0,0.06));border:1px solid rgba(255,255,255,0.03);padding:28px;border-radius:14px;max-width:520px;width:100%;box-shadow:0 8px 30px rgba(2,6,23,0.6)}
    h1{margin:0 0 8px;font-size:20px}
    p{margin:0 0 18px;color:var(--muted)}
    .status{display:flex;align-items:center;gap:12px;margin-bottom:18px}
    .led{width:18px;height:18px;border-radius:50%;background:#223244;box-shadow:inset 0 -2px 6px rgba(0,0,0,0.6)}
    .led.on{background:linear-gradient(180deg,var(--accent),#2d9bf0);box-shadow:0 0 18px rgba(124,58,237,0.45)}
    .toggle{display:inline-flex;align-items:center;gap:12px}
    .btn{appearance:none;border:0;padding:10px 16px;border-radius:10px;background:linear-gradient(90deg,var(--accent),#2d9bf0);color:white;font-weight:600;cursor:pointer;box-shadow:0 6px 18px rgba(13,22,40,0.6)}
    .btn.ghost{background:transparent;border:1px solid rgba(255,255,255,0.06);color:var(--muted);box-shadow:none}
    .footer{margin-top:18px;font-size:13px;color:var(--muted);display:flex;justify-content:space-between;align-items:center}
    @media(max-width:420px){.card{padding:18px}}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>ESP32 — Controle do LED</h1>
      <p>Use a interface abaixo para ligar/desligar o LED no pino <strong>GPIO2</strong>. A conexão Wi‑Fi é gerenciada por WiFiManager.</p>

      <div class="status">
        <div id="ledDot" class="led"></div>
        <div>
          <div id="stateText">Carregando...</div>
          <div style="font-size:13px;color:var(--muted)">IP: <span id="ipAddr">—</span></div>
        </div>
      </div>

      <div style="display:flex;gap:12px">
        <button id="toggleBtn" class="btn">Carregar estado...</button>
        <button id="refreshBtn" class="btn ghost">Atualizar</button>
      </div>

      <div class="footer">
        <div>Powered by ESP32</div>
        <div style="opacity:0.9">WiFiManager</div>
      </div>
    </div>
  </div>

  <script>
    const ledDot = document.getElementById('ledDot');
    const stateText = document.getElementById('stateText');
    const toggleBtn = document.getElementById('toggleBtn');
    const refreshBtn = document.getElementById('refreshBtn');
    const ipAddr = document.getElementById('ipAddr');

    async function getState(){
      try{
        const r = await fetch('/state');
        const j = await r.json();
        updateUI(j.state);
      }catch(e){
        stateText.textContent = 'Erro ao obter estado';
        toggleBtn.textContent = 'Recarregar';
      }
    }

    function updateUI(state){
      const on = (state==1 || state==true);
      ledDot.classList.toggle('on', on);
      stateText.textContent = on ? 'LED: Ligado' : 'LED: Desligado';
      toggleBtn.textContent = on ? 'Desligar' : 'Ligar';
    }

    async function toggle(){
      try{
        toggleBtn.disabled = true;
        const r = await fetch('/toggle');
        const j = await r.json();
        updateUI(j.state);
      }catch(e){
        console.error(e);
      }finally{ toggleBtn.disabled = false }
    }

    toggleBtn.addEventListener('click', toggle);
    refreshBtn.addEventListener('click', getState);

    // Try to read IP from server meta endpoint (root will inject it into /state? not required)
    async function getIP(){
      try{ const r = await fetch('/ip'); if(r.ok){ const j = await r.json(); ipAddr.textContent = j.ip || '—' } }
      catch(e){ /* ignore */ }
    }

    // init
    getState(); getIP();
    setInterval(getState, 5000);
  </script>
</body>
</html>
)rawliteral";



void handleRoot() { 
    server.send_P(200, "text/html", INDEX_HTML); 
}

void handleState() {
    int st = digitalRead(LED_PIN) == HIGH ? 1 : 0;
    String payload = String("{\"state\":") + String(st) + String("}");
    server.send(200, "application/json", payload);
}

void handleToggle() {
    int cur = digitalRead(LED_PIN);
    digitalWrite(LED_PIN, cur == HIGH ? LOW : HIGH);
    handleState();
}

void handleIP() {
    IPAddress ip = WiFi.localIP();
    String payload = String("{\"ip\":\"") + ip.toString() + String("\"}");
    server.send(200, "application/json", payload);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    WiFiManager wifiManager;
    // Auto create AP config portal if can't connect to previous WiFi
    if (!wifiManager.autoConnect("ESP32_AP")) {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        ESP.restart();
    }

    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/state", HTTP_GET, handleState);
    server.on("/toggle", HTTP_GET, handleToggle);
    server.on("/ip", HTTP_GET, handleIP);
    server.begin();
}

void loop() { 
    server.handleClient(); 
}