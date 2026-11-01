# ESP-Soyosource-Controller (Open-Source PlatformIO Fork)

Quelloffene, moderne C++ Firmware für den **Soyosource GTN 1200** (und baugleiche Grid-Tie-Inverter mit RS485-Limiter-Schnittstelle) auf Basis von **ESP8266** (Wemos D1 Mini / ESP-12) und **ESP32**, entwickelt mit **PlatformIO**.

<img src="BastelPlan3000_Soyosource_Controller_by_BavarianSuperGuy.png" alt="Soyosource Controller Anschlussplan" width="800"/>

---

## Originalprojekt & Urheber-Anerkennung

> [!NOTE]
> **Credits an den Original-Entwickler:**
> Die grundlegende Idee der RS485-Regelung, die Pionierarbeit und die Hardware-Schaltpläne stammen von **KlausLi** bzw. dem YouTuber **BavarianSuperGuy**. Da er die maßgebliche Entwicklungsarbeit geleistet hat, soll seine Arbeit gebührend honoriert werden:
> - **YouTube-Kanal des Original-Autors:** [BavarianSuperGuy auf YouTube](https://www.youtube.com/c/BavarianSuperGuy)
> - **Spendenlink an den Original-Autor:** [PayPal-Kaffeekasse für BavarianSuperGuy (BSGuy)](https://paypal.me/BSGuy)
>
> **Hinweis zu diesem Fork:**
> Dieses Repository ist ein reines technisches Open-Source Upgrade von **susesKaninchen**. Der Maintainer dieses Forks ist **nicht** der YouTuber "BavarianSuperGuy", hat lediglich den Quellcode auf PlatformIO modernisiert und nimmt für sich selbstverständlich keinerlei Spenden an.

---

## Highlights & Features

- **100% Open Source (C++ / PlatformIO):**
  - Kein Blind-Flashen von closed-source `.bin`-Dateien mehr.
  - Vollständige Offenlegung des Soyosource RS485-Protokolls (4800 Baud, 8N1, Checksummenalgorithmus).
  - Dual-Target-Unterstützung: Kompiliert out-of-the-box für **ESP8266** (`[env:d1_mini]`) und **ESP32** (`[env:esp32dev]`).
- **Akkustand-Erkennung & Unterspannungsschutz:**
  - **RS485-Rückkanal:** Erkennt und decodiert die 10-Byte-Statuspakete des Soyosource (Batteriespannung in Decivolt, Strom in Deciamp und DC-Leistung).
  - **Home Assistant / Externer BMS-Sync:** Kann Akkuspannung und SoC auch direkt von Home Assistant via MQTT (`Soyosource/cmd/battery_voltage`) oder HTTP (`/Battery?Voltage=...`) empfangen.
  - **Unterspannungsschutz:** Fällt die Akkuspannung unter den eingestellten Schwellwert (`min_battery_voltage`, z.B. 47.0V), drosselt der Controller die Einspeisung automatisch auf 0 Watt (Schutz vor Tiefentladung).
- **Echte geschlossene Nulleinspeisungs-Regelung (Closed-Loop):**
  - Asymmetrisches Totband / Hysterese (Standard: `-20W` Netzeinspeisung bis `+5W` Netzbezug) verhindert nervöses Schwingen.
  - Einstellbarer Ziel-Offset (z.B. +10W Netzbezug als Sicherheitspuffer).
- **Sensor-Watchdog (Failsafe-Sicherheitsabschaltung):**
  - Fällt das Smartmeter (Shelly, MQTT, Tasmota oder HTTP) länger als 30 Sekunden aus, drosselt der Controller die Einspeisung sofort auf **0 Watt** herunter. Schutz vor ungewollter Akkuentleerung ins öffentliche Netz bei WLAN- oder Sensorstörungen.
- **Hardware RS485 DE/RE Steuerung:**
  - Unterstützt sowohl automatische Transceiver (nur TX/RX) als auch klassische MAX485-Module mit DE/RE-Pin an GPIO0 (D3 auf D1 Mini) oder frei konfigurierbarem Pin.
- **Multi-Inverter Lastverteilung (Soyo-Teiler):**
  - Unterstützt mehrere parallel am selben RS485-Bus betriebene Soyosource-Wechselrichter mit automatischer gleichmäßiger Leistungsteilung.
- **Home Assistant MQTT Auto-Discovery:**
  - Bei aktiviertem MQTT-Broker registriert sich der Controller automatisch in Home Assistant mit Sensor-Entities (`Einspeiseleistung`, `Netzbilanz`, `Batteriespannung`, `DC-Leistung`) und einem steuerbaren `NotAus`-Switch.
- **Modernes, responsives Web-Dashboard:**
  - Dark-Mode HTML5/CSS3 Oberfläche, optimiert für Desktop und Smartphone.
  - Live-Herzschlag-Indikator, Live-Leistungskarten (Netz, Soyo-Ausgang, Phasen L1/L2/L3, Akkuspannung/Strom).
  - Schieberegler für manuelle Leistungsvorgabe mit Sofort-Feedback.
  - Sofortige 0W-NotAus-Abschaltung per Klick.
  - Vollständige Konfiguration (Shelly, MQTT, Tasmota, Regelparameter) direkt im Browser speicherbar.
- **Web OTA Updater (`/update`):**
  - Firmware-Updates direkt über den Browser ohne USB-Kabel.
- **Abwärtskompatible REST-API:**
  - Alle bekannten Endpunkte (`/data`, `/state`, `/Battery`, `/L1L2L3Auto`, `/L1L2L3Static`, `/NotAus`, `/MaxPower`, `/SoyoCount`) funktionieren nahtlos weiter.

---

## Hardware & Verkabelung

<img src="BastelPlan0_ESP8266_Rs485_Modul.png" alt="ESP8266 RS485 Modul" width="500"/>

### Soyosource RS485-Schnittstelle
- **Baudrate:** 4800 Baud, 8 Datenbits, keine Parität, 1 Stoppbit (8N1).
- **Anschluss:** Klemme **A** des RS485-Adapters an **A** des Soyosource, Klemme **B** an **B**.
- **Sicherheitsprinzip:** Erhält der Soyosource keine gültigen RS485-Pakete mehr, stoppt er die Einspeisung nach kurzer Zeit selbstständig.

### RS485 TTL-Adapter-Varianten
1. **Automatisches Modul (nur RX / TX):**
   - VCC an 3.3V, GND an GND.
   - Modul TX an ESP TX (GPIO1), Modul RX an ESP RX (GPIO3).
   - In den Einstellungen `RS485 DE/RE Pin` auf `-1` setzen.
2. **Klassisches MAX485-Modul (DI / DE / RE / RO):**
   - VCC an 3.3V, GND an GND.
   - DI (Driver Input) an ESP TX (GPIO1).
   - RO (Receiver Output) an ESP RX (GPIO3).
   - DE und RE brücken und an ESP **GPIO0** (Pin D3 beim Wemos D1 Mini) anschließen.
   - In den Einstellungen `RS485 DE/RE Pin` auf `0` belassen.

<img src="rs485.png" alt="MAX485 DI DE RE RO" width="350"/> <img src="rs485_2 .png" alt="RS485 Auto Module" width="350"/>

---

## Kompilieren & Flashen (PlatformIO)

Das Projekt nutzt [PlatformIO](https://platformio.org/).

### 1. Per USB-Kabel flashen
```bash
# In das Projektverzeichnis wechseln
cd C:\Users\mgabr\Desktop\GitProjekte\Esp-Soyosource-Controller

# Für Wemos D1 Mini / ESP8266 kompilieren und per USB aufspielen
pio run -e d1_mini -t upload

# Für ESP32 kompilieren und per USB aufspielen
pio run -e esp32dev -t upload
```

### 2. Kabellos per Web OTA aktualisieren
Wenn auf dem ESP bereits diese Firmware läuft:
1. Projekt kompilieren: `pio run -e d1_mini`
2. Im Browser `http://<IP-des-Controllers>/update` aufrufen.
3. Die erzeugte Binärdatei `.pio/build/d1_mini/firmware.bin` auswählen und auf **Update** klicken.

---

## Erstinbetriebnahme & WLAN

1. Nach dem ersten Start spannt der ESP ein eigenes WLAN auf:
   - **SSID:** `SOYO_<CHIPID>` (z.B. `SOYO_1A2B3C`)
   - **Passwort:** `12345678`
2. Mit dem Smartphone oder PC mit dem WLAN verbinden und im Browser `http://192.168.4.1/` öffnen.
3. Unter **Konfiguration** das eigene Heim-WLAN auswählen und das Passwort eingeben.
4. Nach dem Speichern und Neustart ist der Controller unter seiner neuen Heimnetz-IP im Browser erreichbar.

---

## Betriebsmodi

Die Betriebsart wird im Web-Dashboard gewählt:

1. **Auto Shelly 3EM / Pro 3EM (Modus 1):**
   - IP-Adresse des Shelly im Netzwerk eintragen.
   - Für Shelly Pro 3EM (Gen2) die Checkbox `Shelly Pro 3EM (RPC)` aktivieren.
   - Phasen L1, L2, L3 auswählen (Standard: alle drei). Der Controller summiert die Phasen automatisch und regelt auf Nulleinspeisung.
2. **Auto MQTT (Modus 0):**
   - Broker-IP, Port, optional Benutzer/Passwort eintragen.
   - Subscription-Topic für die Netzleistung eintragen (z.B. `tele/tasmota/SENSOR` oder ein Home-Assistant-Topic).
   - Der Controller liest den aktuellen Watt-Wert ein und regelt nach.
3. **Auto Custom JSON HTTP (Modus 2):**
   - HTTP GET URL angeben (z.B. `http://192.168.1.141/cm?cmnd=status+10` für Tasmota IR-Lesekopf).
   - JSON-Pfad in Punkt-Notation eintragen (z.B. `StatusSNS.SML.DJ_TPWRCURR`).
4. **Auto HTTP-Interface (Modus 3):**
   - Externe Systeme (z.B. ioBroker, Node-RED) senden den Zählerwert per HTTP-Request an den Controller:
     `GET http://<IP>/L1L2L3Auto?Value=250.5`
5. **Manuell / Statisch (Modus 4):**
   - Feste Watt-Vorgabe über den Schieberegler im Web-Dashboard oder per HTTP:
     `GET http://<IP>/L1L2L3Static?Value=300`

---

## REST-API Referenz

| Endpunkt | Methode | Beschreibung | Beispiel |
| :--- | :--- | :--- | :--- |
| `/` | `GET` | Web-Dashboard (HTML5/CSS3) | `http://<IP>/` |
| `/data` | `GET` | Alle Live-Werte und Konfigurationsdaten als JSON | `http://<IP>/data` |
| `/state` | `GET` | Kompakte Text-Statuszeile (`WattsOut:X,Grid:Y,NotAus:Z`) | `http://<IP>/state` |
| `/L1L2L3Auto?Value=...` | `GET` | Übergibt Zählerwert (W) für automatische Regelung | `/L1L2L3Auto?Value=180.5` |
| `/L1L2L3Static?Value=...` | `GET` | Setzt feste Einspeiseleistung (Handbetrieb) | `/L1L2L3Static?Value=350` |
| `/NotAus` | `GET` | Aktiviert NotAus (Sofort 0 Watt Einspeisung) | `/NotAus` |
| `/NotAusAus` | `GET` | Hebt NotAus wieder auf | `/NotAusAus` |
| `/MaxPower?Value=...` | `GET` | Setzt die maximale Einspeisegrenze in Watt | `/MaxPower?Value=850` |
| `/SoyoCount?Value=...` | `GET` | Setzt die Anzahl parallel geschalteter Inverter | `/SoyoCount?Value=2` |
| `/save` | `POST` | Speichert Konfiguration in LittleFS Flash | Form-POST |
| `/restart` | `GET` | Löst einen Software-Neustart des ESP aus | `/restart` |
| `/update` | `GET/POST` | Web-OTA Firmware-Updater | `http://<IP>/update` |

---

## Home Assistant Integration

### 1. MQTT Auto-Discovery
Wenn ein MQTT-Broker konfiguriert ist, meldet sich der Controller selbstständig bei Home Assistant an. Unter *Einstellungen -> Geräte & Dienste -> MQTT* erscheint das Gerät **Soyosource Controller** mit folgenden Entitäten:
- `sensor.soyosource_controller_einspeiseleistung` (W)
- `sensor.soyosource_controller_netzbilanz` (W)
- `switch.soyosource_controller_notaus` (NotAus Schalter)

### 2. Manuelle MQTT-Befehle
- **NotAus schalten:** Topic `Soyosource/cmd/notaus` mit Payload `ON` oder `OFF`.
- **Feste Wattzahl vorgeben:** Topic `Soyosource/cmd/static_watt` mit Payload z.B. `400`.

---

## Lizenz & Haftungsausschluss

Dieses Projekt wird ohne jede Gewährleistung im Ist-Zustand bereitgestellt. Der Betrieb netzparalleler Wechselrichter und Batteriespeicher unterliegt den jeweils geltenden technischen und gesetzlichen Bestimmungen (VDE, TAB, etc.). Jeder Anwender handelt eigenverantwortlich.
