#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SOYOSOURCE Controller</title>
    <style>
        :root {
            --primary: #1b90bb;
            --primary-dark: #147295;
            --danger: #d9534f;
            --danger-dark: #c9302c;
            --success: #5cb85c;
            --success-dark: #449d44;
            --warning: #f0ad4e;
            --bg: #f4f6f9;
            --card-bg: #ffffff;
            --text: #333333;
            --border: #e2e8f0;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background-color: var(--bg);
            color: var(--text);
            line-height: 1.5;
            padding: 15px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        header {
            text-align: center;
            margin-bottom: 20px;
            padding: 15px;
            background: var(--card-bg);
            border-radius: 12px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.06);
            border-top: 4px solid var(--primary);
        }
        h1 { font-size: 1.6rem; color: var(--text); }
        .subtitle { font-size: 0.85rem; color: #718096; margin-top: 2px; }
        .heartbeat {
            display: inline-block;
            font-size: 0.8rem;
            font-weight: bold;
            padding: 3px 10px;
            border-radius: 20px;
            background: #e2e8f0;
            color: #4a5568;
            margin-top: 8px;
            transition: all 0.3s ease;
        }
        .heartbeat.beat {
            background: var(--success);
            color: white;
            box-shadow: 0 0 8px rgba(92,184,92,0.6);
        }
        .card {
            background: var(--card-bg);
            border-radius: 12px;
            padding: 18px;
            margin-bottom: 18px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.06);
            border: 1px solid var(--border);
        }
        .card h2 {
            font-size: 1.15rem;
            margin-bottom: 14px;
            padding-bottom: 6px;
            border-bottom: 2px solid #edf2f7;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }
        .grid-3 { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 12px; }
        .stat-box {
            background: #f8fafc;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid #edf2f7;
            text-align: center;
        }
        .stat-label { font-size: 0.8rem; color: #718096; text-transform: uppercase; }
        .stat-value { font-size: 1.4rem; font-weight: bold; margin-top: 2px; }
        .stat-value.red { color: var(--danger); }
        .stat-value.green { color: var(--success); }
        .stat-value.blue { color: var(--primary); }
        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 9px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 0.95rem;
            font-weight: 600;
            transition: background 0.2s;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 6px;
        }
        .btn:hover { background: var(--primary-dark); }
        .btn-danger { background: var(--danger); }
        .btn-danger:hover { background: var(--danger-dark); }
        .btn-success { background: var(--success); }
        .btn-success:hover { background: var(--success-dark); }
        .btn-block { width: 100%; }
        .btn-sm { padding: 4px 10px; font-size: 0.85rem; }
        .form-group { margin-bottom: 12px; }
        label { display: block; font-size: 0.85rem; font-weight: 600; margin-bottom: 4px; color: #4a5568; }
        input[type="text"], input[type="number"], input[type="password"], select {
            width: 100%;
            padding: 8px 12px;
            border: 1px solid #cbd5e0;
            border-radius: 6px;
            font-size: 0.95rem;
            background: #ffffff;
        }
        input[type="range"] { width: 100%; margin: 10px 0; }
        .checkbox-label {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            margin-right: 15px;
            font-weight: normal;
            cursor: pointer;
        }
        .notaus-banner {
            display: none;
            background: #fff5f5;
            color: var(--danger);
            border: 2px solid var(--danger);
            padding: 12px;
            border-radius: 8px;
            text-align: center;
            font-weight: bold;
            margin-bottom: 15px;
            animation: pulse 1s infinite alternate;
        }
        @keyframes pulse { from { opacity: 0.7; } to { opacity: 1; } }
        .quick-links { font-size: 0.85rem; text-align: center; margin-top: 20px; color: #718096; }
        .quick-links a { color: var(--primary); text-decoration: none; margin: 0 8px; }
        .quick-links a:hover { text-decoration: underline; }
        @media (max-width: 600px) {
            .grid-2, .grid-3 { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1 id="headerTitle">SOYOSOURCE Controller</h1>
            <div class="subtitle" id="subTitle">Open-Source Fork von susesKaninchen &bull; Basierend auf KlausLi / BavarianSuperGuy</div>
            <div id="heartbeatBadge" class="heartbeat">ESP Herzschlag: OK</div>
        </header>

        <div id="notausBanner" class="notaus-banner">
            ⚠️ NOTAUS IST AKTIV! Einspeisung ist gestoppt (0 Watt).
        </div>
        <div id="timeoutBanner" class="notaus-banner" style="background: #fffaf0; color: #c05621; border-color: #dd6b20;">
            ⚠️ SENSOR TIMEOUT! Smartmeter sendet keine Daten mehr. Notabschaltung auf 0W aktiv.
        </div>
        <div id="batteryBanner" class="notaus-banner" style="background: #fff5f5; color: #e53e3e; border-color: #e53e3e;">
            ⚠️ UNTERSPANNUNGSSCHUTZ! Akkuspannung liegt unter der Abschaltschwelle. Einspeisung gestoppt (0W).
        </div>

        <!-- Live Power Status -->
        <div class="card">
            <h2>⚡ Live-Status</h2>
            <div class="grid-3">
                <div class="stat-box">
                    <div class="stat-label">Haus-Netzleistung (L1+L2+L3)</div>
                    <div id="valGrid" class="stat-value">-- W</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">Sollwert SOYO (Ausgang)</div>
                    <div id="valWattsOut" class="stat-value blue">-- W</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">Soyo-Teiler (Inverter)</div>
                    <div id="valSoyoCount" class="stat-value">--</div>
                </div>
            </div>
            <div style="margin-top: 12px; font-size: 0.85rem; color: #718096; display: flex; justify-content: space-between; flex-wrap: wrap; gap: 8px;">
                <span>Phasen: L1: <b id="valL1">--</b> W | L2: <b id="valL2">--</b> W | L3: <b id="valL3">--</b> W</span>
                <span>Akku: <b id="valDcVolts" style="color: var(--primary);">--</b> V | <b id="valDcAmps">--</b> A | <b id="valDcWatts">--</b> W</span>
                <span>Uptime: <b id="valUptime">--</b></span>
            </div>
        </div>

        <!-- Quick Controls -->
        <div class="card">
            <h2>🎛️ Schnellsteuerung & Notaus</h2>
            <div class="grid-2">
                <button id="btnNotAus" class="btn btn-danger btn-block" onclick="setNotAus(true)">🛑 NOTAUS (0W Sofort)</button>
                <button id="btnNotAusAus" class="btn btn-success btn-block" onclick="setNotAus(false)">✅ NOTAUS AUFHEBEN</button>
            </div>
            
            <div style="margin-top: 18px;">
                <label>Statische Einspeisung (Handbetrieb): <b id="sliderValDisplay">0</b> W</label>
                <input type="range" id="staticSlider" min="0" max="1200" step="10" value="0" oninput="updateSliderDisplay(this.value)">
                <div style="display: flex; gap: 8px;">
                    <input type="number" id="staticInput" placeholder="Watt eingeben" min="0" max="1200" style="flex: 1;">
                    <button class="btn" onclick="applyStatic()">Übernehmen</button>
                </div>
            </div>

            <div style="margin-top: 18px;" class="grid-2">
                <div>
                    <label>MaxPower Grenze:</label>
                    <div style="display: flex; gap: 6px; align-items: center;">
                        <button class="btn btn-sm" onclick="adjustMaxPower(-50)">- 50W</button>
                        <b id="valMaxPower" style="flex: 1; text-align: center; font-size: 1.1rem;">-- W</b>
                        <button class="btn btn-sm" onclick="adjustMaxPower(50)">+ 50W</button>
                    </div>
                </div>
                <div>
                    <label>Parallel-Soyos (Teiler):</label>
                    <div style="display: flex; gap: 6px; align-items: center;">
                        <button class="btn btn-sm" onclick="adjustSoyoCount(-1)">- 1</button>
                        <b id="valSoyoDivider" style="flex: 1; text-align: center; font-size: 1.1rem;">--</b>
                        <button class="btn btn-sm" onclick="adjustSoyoCount(1)">+ 1</button>
                    </div>
                </div>
            </div>
        </div>

        <!-- Configuration Settings -->
        <div class="card">
            <h2>⚙️ Betriebsart & Konfiguration</h2>
            <form id="configForm" onsubmit="saveConfiguration(event)">
                <div class="form-group">
                    <label>Controller Name</label>
                    <input type="text" id="cfg_controller_name" name="controller_name" required>
                </div>

                <div class="form-group">
                    <label>Betriebsmodus</label>
                    <select id="cfg_mode" name="mode" onchange="onModeChange()">
                        <option value="0">Auto MQTT (Topic abonnieren)</option>
                        <option value="1">Auto Shelly (3EM / Pro 3EM direkt abfragen)</option>
                        <option value="2">Auto JSON HTTP (Tasmota / Custom Endpoint)</option>
                        <option value="3">Auto HTTP Interface (/L1L2L3Auto?Value=X)</option>
                        <option value="4">Statisch (Handbetrieb WebGUI)</option>
                    </select>
                </div>

                <!-- Shelly Config Block -->
                <div id="blockShelly" style="background: #f8fafc; padding: 12px; border-radius: 8px; margin-bottom: 12px; border: 1px solid #e2e8f0;">
                    <div class="form-group">
                        <label>Shelly IP-Adresse</label>
                        <input type="text" id="cfg_shelly_ip" name="shelly_ip" placeholder="192.168.178.54">
                    </div>
                    <div class="form-group">
                        <label class="checkbox-label">
                            <input type="checkbox" id="cfg_shelly_is_pro" name="shelly_is_pro"> Shelly Pro 3EM (Gen2 RPC)
                        </label>
                    </div>
                    <div class="form-group">
                        <label>Zu erfassende Phasen:</label>
                        <label class="checkbox-label"><input type="checkbox" id="cfg_shelly_l1" name="shelly_l1"> Phase L1</label>
                        <label class="checkbox-label"><input type="checkbox" id="cfg_shelly_l2" name="shelly_l2"> Phase L2</label>
                        <label class="checkbox-label"><input type="checkbox" id="cfg_shelly_l3" name="shelly_l3"> Phase L3</label>
                    </div>
                </div>

                <!-- MQTT Config Block -->
                <div id="blockMqtt" style="background: #f8fafc; padding: 12px; border-radius: 8px; margin-bottom: 12px; border: 1px solid #e2e8f0;">
                    <div class="grid-2">
                        <div class="form-group">
                            <label>MQTT Server / Broker</label>
                            <input type="text" id="cfg_mqtt_server" name="mqtt_server" placeholder="192.168.178.189">
                        </div>
                        <div class="form-group">
                            <label>Port</label>
                            <input type="number" id="cfg_mqtt_port" name="mqtt_port" value="1883">
                        </div>
                    </div>
                    <div class="grid-2">
                        <div class="form-group">
                            <label>Benutzername</label>
                            <input type="text" id="cfg_mqtt_user" name="mqtt_user">
                        </div>
                        <div class="form-group">
                            <label>Passwort</label>
                            <input type="password" id="cfg_mqtt_pass" name="mqtt_pass">
                        </div>
                    </div>
                    <div class="form-group">
                        <label>Abonniertes Power-Topic (Input)</label>
                        <input type="text" id="cfg_mqtt_topic_sub" name="mqtt_topic_sub" placeholder="shellies/shellyem3/emeter/0/power">
                    </div>
                    <div class="form-group">
                        <label>Publish Status-Topic (Output)</label>
                        <input type="text" id="cfg_mqtt_topic_pub" name="mqtt_topic_pub" placeholder="Soyosource/AllJsonData">
                    </div>
                </div>

                <!-- JSON HTTP Block -->
                <div id="blockJson" style="background: #f8fafc; padding: 12px; border-radius: 8px; margin-bottom: 12px; border: 1px solid #e2e8f0;">
                    <div class="form-group">
                        <label>HTTP Get URL</label>
                        <input type="text" id="cfg_json_http_url" name="json_http_url" placeholder="http://192.168.1.141/cm?cmnd=status+10">
                    </div>
                    <div class="form-group">
                        <label>JSON Value Path</label>
                        <input type="text" id="cfg_json_value_path" name="json_value_path" placeholder="StatusSNS.SML.DJ_TPWRCURR">
                    </div>
                </div>

                <!-- Fine Tuning -->
                <div class="grid-3">
                    <div class="form-group">
                        <label>Wartezeit (s)</label>
                        <input type="number" id="cfg_wait_seconds" name="wait_seconds" min="1" max="10" value="2">
                    </div>
                    <div class="form-group">
                        <label>Einspeise-Toleranz (W)</label>
                        <input type="number" id="cfg_tolerance_export" name="tolerance_export" placeholder="-20" value="-20">
                    </div>
                    <div class="form-group">
                        <label>Bezugs-Toleranz (W)</label>
                        <input type="number" id="cfg_tolerance_import" name="tolerance_import" placeholder="5" value="5">
                    </div>
                </div>
                <div class="grid-3">
                    <div class="form-group">
                        <label>Ziel-Offset (W)</label>
                        <input type="number" id="cfg_target_offset" name="target_offset" placeholder="z.B. 0" value="0">
                    </div>
                    <div class="form-group">
                        <label>Sensor-Timeout (s)</label>
                        <input type="number" id="cfg_sensor_timeout_sec" name="sensor_timeout_sec" min="0" max="300" value="30">
                    </div>
                    <div class="form-group">
                        <label>RS485 DE/RE Pin</label>
                        <input type="number" id="cfg_rs485_dere_pin" name="rs485_dere_pin" placeholder="0 für GPIO0/D3, -1 für Auto" value="0">
                    </div>
                </div>
                <div class="grid-3" style="margin-top: 10px;">
                    <div class="form-group">
                        <label>Min. Akkuspannung (V)</label>
                        <input type="number" step="0.1" id="cfg_min_battery_voltage" name="min_battery_voltage" placeholder="z.B. 47.0" value="47.0">
                    </div>
                </div>

                <div class="grid-2" style="margin-top: 15px;">
                    <button type="submit" class="btn btn-success btn-block">💾 Einstellungen Speichern</button>
                    <button type="button" class="btn btn-danger btn-block" onclick="restartDevice()">🔄 Neustart</button>
                </div>
            </form>
        </div>

        <div class="quick-links">
            <a href="/data" target="_blank">📄 JSON Telemetrie (/data)</a> |
            <a href="/update" target="_blank">🚀 Firmware Update (OTA)</a> |
            <a href="/NotAus" target="_blank">🛑 NotAus API</a> |
            <a href="/NotAusAus" target="_blank">✅ NotAus Aufheben</a>
        </div>

        <footer style="text-align: center; margin-top: 25px; padding-bottom: 20px; font-size: 0.8rem; color: #a0aec0;">
            Open-Source Fork gepflegt von <a href="https://github.com/susesKaninchen/Esp-Soyosource-Controller" target="_blank" style="color: #4a5568; text-decoration: underline;">susesKaninchen</a> &bull; Basierend auf Konzepten von KlausLi &amp; BavarianSuperGuy
        </footer>
    </div>

    <script>
        let liveConfig = {};
        let beatState = false;

        function updateSliderDisplay(val) {
            document.getElementById('sliderValDisplay').innerText = val;
            document.getElementById('staticInput').value = val;
        }

        function applyStatic() {
            let val = document.getElementById('staticInput').value;
            fetch('/L1L2L3Static?Value=' + encodeURIComponent(val))
                .then(r => r.json())
                .then(d => {
                    document.getElementById('cfg_mode').value = "4";
                    onModeChange();
                });
        }

        function setNotAus(state) {
            let url = state ? '/NotAus' : '/NotAusAus';
            fetch(url).then(r => fetchTelemetry());
        }

        function adjustMaxPower(delta) {
            let cur = liveConfig.max_power || 900;
            let next = Math.max(50, cur + delta);
            fetch('/MaxPower?Value=' + next).then(r => fetchTelemetry());
        }

        function adjustSoyoCount(delta) {
            let cur = liveConfig.soyo_count || 1;
            let next = Math.max(1, cur + delta);
            fetch('/SoyoCount?Value=' + next).then(r => fetchTelemetry());
        }

        function restartDevice() {
            if (confirm("Möchtest du den Controller wirklich neu starten?")) {
                fetch('/restart').then(() => alert("Neustart ausgelöst. Seite in 10 Sekunden neu laden."));
            }
        }

        function onModeChange() {
            let mode = document.getElementById('cfg_mode').value;
            document.getElementById('blockMqtt').style.display = (mode === "0") ? "block" : "none";
            document.getElementById('blockShelly').style.display = (mode === "1") ? "block" : "none";
            document.getElementById('blockJson').style.display = (mode === "2") ? "block" : "none";
        }

        function fetchTelemetry() {
            fetch('/data')
                .then(r => r.json())
                .then(data => {
                    liveConfig = data;
                    
                    // Header & Device Name
                    if (data.ControllerName) {
                        document.getElementById('headerTitle').innerText = data.ControllerName;
                    }

                    // Live grid & power
                    let grid = parseFloat(data.L1L2L3) || 0;
                    let elGrid = document.getElementById('valGrid');
                    elGrid.innerText = grid.toFixed(1) + " W";
                    elGrid.className = 'stat-value ' + (grid > 0 ? 'red' : 'green');

                    document.getElementById('valWattsOut').innerText = (data.WattsOut || 0) + " W";
                    document.getElementById('valSoyoCount').innerText = data.SoyoCount || 1;
                    document.getElementById('valSoyoDivider').innerText = data.SoyoCount || 1;
                    document.getElementById('valMaxPower').innerText = (data.MaxPower || 900) + " W";
                    document.getElementById('valUptime').innerText = data.StartTime || "--";

                    document.getElementById('valL1').innerText = (parseFloat(data.L1) || 0).toFixed(1);
                    document.getElementById('valL2').innerText = (parseFloat(data.L2) || 0).toFixed(1);
                    document.getElementById('valL3').innerText = (parseFloat(data.L3) || 0).toFixed(1);

                    // NotAus, Timeout & Low-Battery State
                    let isNotAus = (data.NotAus === "1" || data.NotAus === true);
                    document.getElementById('notausBanner').style.display = isNotAus ? "block" : "none";

                    let isTimeout = (data.sensor_timeout === true || data.sensor_timeout === "1");
                    document.getElementById('timeoutBanner').style.display = isTimeout ? "block" : "none";

                    let isBattLow = (data.battery_low === true || data.battery_low === "1");
                    document.getElementById('batteryBanner').style.display = isBattLow ? "block" : "none";

                    // DC Battery values
                    document.getElementById('valDcVolts').innerText = data.DCVolts || "--";
                    document.getElementById('valDcAmps').innerText = data.DCAmps || "--";
                    document.getElementById('valDcWatts').innerText = data.DCWatts || "--";

                    // Heartbeat indicator
                    beatState = !beatState;
                    let badge = document.getElementById('heartbeatBadge');
                    if (beatState) {
                        badge.classList.add('beat');
                    } else {
                        badge.classList.remove('beat');
                    }
                })
                .catch(err => {
                    document.getElementById('heartbeatBadge').innerText = "ESP Herzschlag: FEHLER";
                    document.getElementById('heartbeatBadge').classList.remove('beat');
                });
        }

        function populateForm() {
            fetch('/data')
                .then(r => r.json())
                .then(data => {
                    if (data.ControllerName) document.getElementById('cfg_controller_name').value = data.ControllerName;
                    if (data.ControllerMode !== undefined) document.getElementById('cfg_mode').value = data.ControllerMode;
                    
                    if (data.ShellyIP) document.getElementById('cfg_shelly_ip').value = data.ShellyIP;
                    document.getElementById('cfg_shelly_is_pro').checked = (data.S3emPro === "1" || data.S3emPro === true);
                    document.getElementById('cfg_shelly_l1').checked = (data.use_l1 !== false);
                    document.getElementById('cfg_shelly_l2').checked = (data.use_l2 !== false);
                    document.getElementById('cfg_shelly_l3').checked = (data.use_l3 !== false);

                    if (data.mqtt_server) document.getElementById('cfg_mqtt_server').value = data.mqtt_server;
                    if (data.mqtt_port) document.getElementById('cfg_mqtt_port').value = data.mqtt_port;
                    if (data.mqtt_user) document.getElementById('cfg_mqtt_user').value = data.mqtt_user;
                    if (data.Topic_L1L2L3) document.getElementById('cfg_mqtt_topic_sub').value = data.Topic_L1L2L3;
                    if (data.Topic_AllJsonData) document.getElementById('cfg_mqtt_topic_pub').value = data.Topic_AllJsonData;

                    if (data.HttpGetUrl) document.getElementById('cfg_json_http_url').value = data.HttpGetUrl;
                    if (data.JsonValuePath) document.getElementById('cfg_json_value_path').value = data.JsonValuePath;

                    if (data.WaitSekunden) document.getElementById('cfg_wait_seconds').value = data.WaitSekunden;
                    if (data.tolerance_export !== undefined) document.getElementById('cfg_tolerance_export').value = data.tolerance_export;
                    if (data.tolerance_import !== undefined) document.getElementById('cfg_tolerance_import').value = data.tolerance_import;
                    if (data.target_offset !== undefined) document.getElementById('cfg_target_offset').value = data.target_offset;
                    if (data.sensor_timeout_sec !== undefined) document.getElementById('cfg_sensor_timeout_sec').value = data.sensor_timeout_sec;
                    if (data.rs485_dere_pin !== undefined) document.getElementById('cfg_rs485_dere_pin').value = data.rs485_dere_pin;
                    if (data.min_battery_voltage !== undefined) document.getElementById('cfg_min_battery_voltage').value = data.min_battery_voltage;

                    if (data.static_watt !== undefined) {
                        document.getElementById('staticSlider').value = data.static_watt;
                        updateSliderDisplay(data.static_watt);
                    }

                    onModeChange();
                });
        }

        function saveConfiguration(e) {
            e.preventDefault();
            let form = document.getElementById('configForm');
            let formData = new FormData(form);
            let params = new URLSearchParams();
            for (let pair of formData.entries()) {
                params.append(pair[0], pair[1]);
            }
            params.append('shelly_is_pro', document.getElementById('cfg_shelly_is_pro').checked ? "1" : "0");
            params.append('shelly_l1', document.getElementById('cfg_shelly_l1').checked ? "1" : "0");
            params.append('shelly_l2', document.getElementById('cfg_shelly_l2').checked ? "1" : "0");
            params.append('shelly_l3', document.getElementById('cfg_shelly_l3').checked ? "1" : "0");

            fetch('/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: params.toString()
            })
            .then(r => r.json())
            .then(res => {
                alert("Einstellungen wurden erfolgreich gespeichert!");
                fetchTelemetry();
            })
            .catch(err => alert("Fehler beim Speichern: " + err));
        }

        // Initialize
        populateForm();
        fetchTelemetry();
        setInterval(fetchTelemetry, 1500);
    </script>
</body>
</html>
)rawliteral";
