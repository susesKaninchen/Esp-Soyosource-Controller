#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266HTTPUpdateServer.h>
  #include <WiFiClient.h>
  typedef ESP8266WebServer WebServerClass;
  typedef ESP8266HTTPUpdateServer HTTPUpdateServerClass;
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <HTTPClient.h>
  #include <HTTPUpdateServer.h>
  #include <WiFiClient.h>
  typedef WebServer WebServerClass;
  typedef HTTPUpdateServer HTTPUpdateServerClass;
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "soyo_protocol.h"
#include "web_ui.h"

// System instances
SystemConfig config;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServerClass server(80);
HTTPUpdateServerClass httpUpdater;

// Timing variables
unsigned long lastRegulationTime = 0;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastPollTime = 0;

// Helper: Format Uptime string
String getUptimeStr() {
    unsigned long sec = millis() / 1000;
    unsigned long days = sec / 86400;
    sec %= 86400;
    unsigned long hours = sec / 3600;
    sec %= 3600;
    unsigned long mins = sec / 60;
    sec %= 60;

    char buf[32];
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %02lu:%02lu:%02lu", days, hours, mins, sec);
    } else {
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, mins, sec);
    }
    return String(buf);
}

// Helper: Get unique Chip ID across ESP8266 & ESP32
uint32_t getDeviceChipId() {
#if defined(ESP8266)
    return ESP.getChipId();
#elif defined(ESP32)
    return (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
#else
    return 0;
#endif
}

// Forward declarations
void setupWebServer();
void setupMQTT();
void pollShelly();
void pollCustomJson();
void executeRegulation();
void publishTelemetry();

// ==========================================
// Setup WiFi & SoftAP Fallback
// ==========================================
void setupWiFi() {
    WiFi.mode(WIFI_STA);

    if (config.wifi_ssid.length() > 0) {
        Serial.print(F("[WiFi] Connecting to "));
        Serial.println(config.wifi_ssid);
        WiFi.begin(config.wifi_ssid.c_str(), config.wifi_pass.c_str());

        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 12000) {
            delay(500);
            Serial.print(F("."));
        }
        Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F("[WiFi] Connected! IP: "));
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(F("[WiFi] Starting Fallback SoftAP..."));
        WiFi.mode(WIFI_AP_STA);
        String apName = "SOYO_" + String(getDeviceChipId(), HEX);
        WiFi.softAP(apName.c_str(), "12345678");
        Serial.print(F("[WiFi] AP IP: "));
        Serial.println(WiFi.softAPIP());
    }
}

// ==========================================
// WebServer Route Handlers
// ==========================================
void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleData() {
    StaticJsonDocument<1536> doc;

    doc["Version"] = "1.0.4-opensource";
    doc["ControllerName"] = config.controller_name;
    doc["SOYOID"] = "SOYO_" + String(getDeviceChipId(), HEX);
    doc["WIFI_RSSI"] = (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) : "N/A";
    doc["WIFI_SSID"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "AP-Mode";
    doc["IP"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["StartTime"] = getUptimeStr();

    doc["L1L2L3"] = String(config.current_grid_power, 2);
    doc["L1"] = String(config.current_l1, 2);
    doc["L2"] = String(config.current_l2, 2);
    doc["L3"] = String(config.current_l3, 2);
    doc["WattsOut"] = config.current_watts_out;

    doc["NotAus"] = config.not_aus ? "1" : "0";
    doc["WaitSekunden"] = config.wait_seconds;
    doc["MaxPower"] = config.max_power;
    doc["SoyoCount"] = config.soyo_count;
    doc["ControllerMode"] = config.mode;
    doc["static_watt"] = config.static_watt;
    doc["deadband"] = config.deadband;
    doc["target_offset"] = config.target_offset;
    doc["tolerance_export"] = config.tolerance_export;
    doc["tolerance_import"] = config.tolerance_import;
    doc["rs485_dere_pin"] = config.rs485_dere_pin;
    doc["sensor_timeout_sec"] = config.sensor_timeout_sec;
    doc["sensor_timeout"] = config.sensor_timeout;

    // Battery / DC status
    doc["DCVolts"] = config.has_dc_feedback ? String(config.dc_volts, 1) : (config.ext_battery_volts > 0 ? String(config.ext_battery_volts, 1) : "0.0");
    doc["DCAmps"] = config.has_dc_feedback ? String(config.dc_amps, 1) : "0.0";
    doc["DCWatts"] = config.has_dc_feedback ? String(config.dc_watts, 1) : "0.0";
    doc["min_battery_voltage"] = config.min_battery_voltage;
    doc["ext_battery_volts"] = config.ext_battery_volts;
    doc["battery_soc"] = config.battery_soc;
    doc["battery_low"] = config.battery_low;
    doc["has_dc_feedback"] = config.has_dc_feedback;

    doc["ShellyIP"] = config.shelly_ip;
    doc["S3emPro"] = config.shelly_is_pro ? "1" : "0";
    doc["use_l1"] = config.shelly_l1;
    doc["use_l2"] = config.shelly_l2;
    doc["use_l3"] = config.shelly_l3;

    doc["mqtt_server"] = config.mqtt_server;
    doc["mqtt_port"] = config.mqtt_port;
    doc["mqtt_user"] = config.mqtt_user;
    doc["Topic_L1L2L3"] = config.mqtt_topic_sub;
    doc["Topic_AllJsonData"] = config.mqtt_topic_pub;

    doc["HttpGetUrl"] = config.json_http_url;
    doc["JsonValuePath"] = config.json_value_path;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleState() {
    String resp = "WattsOut:" + String(config.current_watts_out) +
                  ",Grid:" + String(config.current_grid_power, 1) +
                  ",NotAus:" + String(config.not_aus ? 1 : 0);
    server.send(200, "text/plain", resp);
}

void handleL1L2L3Auto() {
    if (server.hasArg("Value")) {
        config.current_grid_power = server.arg("Value").toFloat();
        config.last_sensor_time = millis();
        config.sensor_timeout = false;
        config.mode = MODE_AUTO_HTTP;
        executeRegulation();
        server.send(200, "application/json", "{\"status\":\"ok\",\"value\":" + String(config.current_grid_power) + "}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing Value parameter\"}");
    }
}

void handleL1L2L3Static() {
    if (server.hasArg("Value")) {
        config.static_watt = server.arg("Value").toInt();
        config.mode = MODE_STATIC;
        executeRegulation();
        server.send(200, "application/json", "{\"status\":\"ok\",\"static_watt\":" + String(config.static_watt) + "}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing Value parameter\"}");
    }
}

void handleNotAus() {
    config.not_aus = true;
    executeRegulation();
    server.send(200, "application/json", "{\"status\":\"ok\",\"not_aus\":true}");
}

void handleNotAusAus() {
    config.not_aus = false;
    executeRegulation();
    server.send(200, "application/json", "{\"status\":\"ok\",\"not_aus\":false}");
}

void handleMaxPower() {
    if (server.hasArg("Value")) {
        config.max_power = constrain(server.arg("Value").toInt(), 50, 10000);
        ConfigManager::saveConfig(config);
        server.send(200, "application/json", "{\"status\":\"ok\",\"max_power\":" + String(config.max_power) + "}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing Value parameter\"}");
    }
}

void handleSoyoCount() {
    if (server.hasArg("Value")) {
        config.soyo_count = constrain(server.arg("Value").toInt(), 1, 16);
        ConfigManager::saveConfig(config);
        server.send(200, "application/json", "{\"status\":\"ok\",\"soyo_count\":" + String(config.soyo_count) + "}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing Value parameter\"}");
    }
}

void handleSave() {
    if (server.hasArg("controller_name")) config.controller_name = server.arg("controller_name");
    if (server.hasArg("mode")) config.mode = server.arg("mode").toInt();
    if (server.hasArg("wait_seconds")) config.wait_seconds = constrain(server.arg("wait_seconds").toInt(), 1, 10);
    if (server.hasArg("deadband")) config.deadband = constrain(server.arg("deadband").toInt(), 0, 100);
    if (server.hasArg("target_offset")) config.target_offset = server.arg("target_offset").toInt();
    if (server.hasArg("tolerance_export")) config.tolerance_export = server.arg("tolerance_export").toInt();
    if (server.hasArg("tolerance_import")) config.tolerance_import = server.arg("tolerance_import").toInt();
    if (server.hasArg("rs485_dere_pin")) {
        config.rs485_dere_pin = server.arg("rs485_dere_pin").toInt();
        if (config.rs485_dere_pin >= 0) {
            pinMode(config.rs485_dere_pin, OUTPUT);
            digitalWrite(config.rs485_dere_pin, LOW);
        }
    }
    if (server.hasArg("sensor_timeout_sec")) config.sensor_timeout_sec = constrain(server.arg("sensor_timeout_sec").toInt(), 0, 300);
    if (server.hasArg("min_battery_voltage")) config.min_battery_voltage = server.arg("min_battery_voltage").toFloat();

    if (server.hasArg("shelly_ip")) config.shelly_ip = server.arg("shelly_ip");
    config.shelly_is_pro = (server.arg("shelly_is_pro") == "1");
    config.shelly_l1 = (server.arg("shelly_l1") == "1");
    config.shelly_l2 = (server.arg("shelly_l2") == "1");
    config.shelly_l3 = (server.arg("shelly_l3") == "1");

    if (server.hasArg("mqtt_server")) config.mqtt_server = server.arg("mqtt_server");
    if (server.hasArg("mqtt_port")) config.mqtt_port = server.arg("mqtt_port").toInt();
    if (server.hasArg("mqtt_user")) config.mqtt_user = server.arg("mqtt_user");
    if (server.hasArg("mqtt_pass") && server.arg("mqtt_pass").length() > 0) {
        config.mqtt_pass = server.arg("mqtt_pass");
    }
    if (server.hasArg("mqtt_topic_sub")) config.mqtt_topic_sub = server.arg("mqtt_topic_sub");
    if (server.hasArg("mqtt_topic_pub")) config.mqtt_topic_pub = server.arg("mqtt_topic_pub");

    if (server.hasArg("json_http_url")) config.json_http_url = server.arg("json_http_url");
    if (server.hasArg("json_value_path")) config.json_value_path = server.arg("json_value_path");

    ConfigManager::saveConfig(config);
    setupMQTT();
    server.send(200, "application/json", "{\"status\":\"saved\"}");
}

void handleRestart() {
    server.send(200, "application/json", "{\"status\":\"restarting\"}");
    delay(500);
    ESP.restart();
}

void handleBattery() {
    if (server.hasArg("Voltage")) {
        config.ext_battery_volts = server.arg("Voltage").toFloat();
    }
    if (server.hasArg("SoC")) {
        config.battery_soc = server.arg("SoC").toInt();
    }
    server.send(200, "application/json", "{\"status\":\"ok\",\"volts\":" + String(config.ext_battery_volts) + ",\"soc\":" + String(config.battery_soc) + "}");
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.on("/state", HTTP_GET, handleState);
    server.on("/Battery", HTTP_GET, handleBattery);
    server.on("/L1L2L3Auto", HTTP_GET, handleL1L2L3Auto);
    server.on("/L1L2L3Static", HTTP_GET, handleL1L2L3Static);
    server.on("/NotAus", HTTP_GET, handleNotAus);
    server.on("/NotAusAus", HTTP_GET, handleNotAusAus);
    server.on("/MaxPower", HTTP_GET, handleMaxPower);
    server.on("/SoyoCount", HTTP_GET, handleSoyoCount);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/restart", HTTP_GET, handleRestart);

    // OTA Web Firmware Updater on /update
    httpUpdater.setup(&server, "/update");

    server.begin();
    Serial.println(F("[Web] HTTP Server started"));
}

// ==========================================
// MQTT Handling & Home Assistant Auto-Discovery
// ==========================================
void publishHomeAssistantDiscovery() {
    String chipId = String(getDeviceChipId(), HEX);
    String devId = "soyo_" + chipId;

    auto addDeviceBlock = [&](JsonObject dev) {
        JsonArray ids = dev.createNestedArray("ids");
        ids.add(devId);
        dev["name"] = config.controller_name;
        dev["mf"] = "susesKaninchen";
        dev["mdl"] = "Soyosource Controller";
        dev["sw"] = "1.0.4-opensource";
    };

    // 1. WattsOut sensor
    {
        StaticJsonDocument<512> doc;
        doc["name"] = "Einspeiseleistung";
        doc["stat_t"] = config.mqtt_topic_pub;
        doc["val_tpl"] = "{{ value_json.WattsOut }}";
        doc["unit_of_meas"] = "W";
        doc["dev_cla"] = "power";
        doc["stat_cla"] = "measurement";
        doc["uniq_id"] = devId + "_watts_out";
        addDeviceBlock(doc.createNestedObject("dev"));

        char buf[512];
        serializeJson(doc, buf);
        mqttClient.publish(("homeassistant/sensor/" + devId + "_watts/config").c_str(), buf, true);
    }

    // 2. Grid Power sensor (L1L2L3)
    {
        StaticJsonDocument<512> doc;
        doc["name"] = "Netzbilanz";
        doc["stat_t"] = config.mqtt_topic_pub;
        doc["val_tpl"] = "{{ value_json.L1L2L3 }}";
        doc["unit_of_meas"] = "W";
        doc["dev_cla"] = "power";
        doc["stat_cla"] = "measurement";
        doc["uniq_id"] = devId + "_grid";
        addDeviceBlock(doc.createNestedObject("dev"));

        char buf[512];
        serializeJson(doc, buf);
        mqttClient.publish(("homeassistant/sensor/" + devId + "_grid/config").c_str(), buf, true);
    }

    // 3. NotAus Switch
    {
        StaticJsonDocument<512> doc;
        doc["name"] = "NotAus";
        doc["cmd_t"] = "Soyosource/cmd/notaus";
        doc["stat_t"] = config.mqtt_topic_pub;
        doc["val_tpl"] = "{{ 'ON' if value_json.NotAus else 'OFF' }}";
        doc["uniq_id"] = devId + "_notaus";
        doc["icon"] = "mdi:alert-octagon";
        addDeviceBlock(doc.createNestedObject("dev"));

        char buf[512];
        serializeJson(doc, buf);
        mqttClient.publish(("homeassistant/switch/" + devId + "_notaus/config").c_str(), buf, true);
    }

    // 4. Battery Voltage sensor
    {
        StaticJsonDocument<512> doc;
        doc["name"] = "Batteriespannung";
        doc["stat_t"] = config.mqtt_topic_pub;
        doc["val_tpl"] = "{{ value_json.DCVolts }}";
        doc["unit_of_meas"] = "V";
        doc["dev_cla"] = "voltage";
        doc["stat_cla"] = "measurement";
        doc["uniq_id"] = devId + "_dc_volts";
        addDeviceBlock(doc.createNestedObject("dev"));

        char buf[512];
        serializeJson(doc, buf);
        mqttClient.publish(("homeassistant/sensor/" + devId + "_dc_volts/config").c_str(), buf, true);
    }

    // 5. Battery Power sensor
    {
        StaticJsonDocument<512> doc;
        doc["name"] = "DC Leistung";
        doc["stat_t"] = config.mqtt_topic_pub;
        doc["val_tpl"] = "{{ value_json.DCWatts }}";
        doc["unit_of_meas"] = "W";
        doc["dev_cla"] = "power";
        doc["stat_cla"] = "measurement";
        doc["uniq_id"] = devId + "_dc_watts";
        addDeviceBlock(doc.createNestedObject("dev"));

        char buf[512];
        serializeJson(doc, buf);
        mqttClient.publish(("homeassistant/sensor/" + devId + "_dc_watts/config").c_str(), buf, true);
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char msg[64];
    if (length >= sizeof(msg)) length = sizeof(msg) - 1;
    memcpy(msg, payload, length);
    msg[length] = '\0';

    String strTopic(topic);
    if (strTopic == config.mqtt_topic_sub) {
        config.current_grid_power = atof(msg);
        config.last_sensor_time = millis();
        config.sensor_timeout = false;
    } else if (strTopic == "Soyosource/cmd/notaus") {
        if (strcmp(msg, "ON") == 0 || strcmp(msg, "1") == 0) {
            config.not_aus = true;
        } else {
            config.not_aus = false;
        }
        executeRegulation();
    } else if (strTopic == "Soyosource/cmd/static_watt") {
        config.static_watt = atoi(msg);
        config.mode = MODE_STATIC;
        executeRegulation();
    } else if (strTopic == "Soyosource/cmd/battery_voltage") {
        config.ext_battery_volts = atof(msg);
    } else if (strTopic == "Soyosource/cmd/battery_soc") {
        config.battery_soc = atoi(msg);
    }
}

void setupMQTT() {
    if (config.mqtt_server.length() == 0) return;

    mqttClient.setServer(config.mqtt_server.c_str(), config.mqtt_port);
    mqttClient.setCallback(mqttCallback);
}

void handleMQTT() {
    if (config.mqtt_server.length() == 0) return;

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastMqttReconnectAttempt > 5000) {
            lastMqttReconnectAttempt = now;
            String clientId = "Soyosource-" + String(getDeviceChipId(), HEX);
            bool ok = false;
            if (config.mqtt_user.length() > 0) {
                ok = mqttClient.connect(clientId.c_str(), config.mqtt_user.c_str(), config.mqtt_pass.c_str());
            } else {
                ok = mqttClient.connect(clientId.c_str());
            }

            if (ok) {
                Serial.println(F("[MQTT] Connected to broker"));
                if (config.mqtt_topic_sub.length() > 0) {
                    mqttClient.subscribe(config.mqtt_topic_sub.c_str());
                }
                mqttClient.subscribe("Soyosource/cmd/#");
                publishHomeAssistantDiscovery();
            }
        }
    } else {
        mqttClient.loop();
    }
}

// ==========================================
// HTTP Polling: Shelly 3EM / Pro 3EM & Custom JSON
// ==========================================
void pollShelly() {
    if (config.shelly_ip.length() == 0) return;

    HTTPClient http;
    WiFiClient client;
    String url;

    if (config.shelly_is_pro) {
        url = "http://" + config.shelly_ip + "/rpc/EM.GetStatus?id=0";
    } else {
        url = "http://" + config.shelly_ip + "/status";
    }

    http.begin(client, url);
    http.setTimeout(1200);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<2048> doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
            float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
            if (config.shelly_is_pro) {
                p1 = doc["a_act_power"].as<float>();
                p2 = doc["b_act_power"].as<float>();
                p3 = doc["c_act_power"].as<float>();
            } else {
                p1 = doc["emeters"][0]["power"].as<float>();
                p2 = doc["emeters"][1]["power"].as<float>();
                p3 = doc["emeters"][2]["power"].as<float>();
            }

            config.current_l1 = p1;
            config.current_l2 = p2;
            config.current_l3 = p3;

            float sum = 0.0f;
            if (config.shelly_l1) sum += p1;
            if (config.shelly_l2) sum += p2;
            if (config.shelly_l3) sum += p3;
            config.current_grid_power = sum;
            config.last_sensor_time = millis();
            config.sensor_timeout = false;
        }
    }
    http.end();
}

void pollCustomJson() {
    if (config.json_http_url.length() == 0) return;

    HTTPClient http;
    WiFiClient client;
    http.begin(client, config.json_http_url);
    http.setTimeout(1200);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<2048> doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
            // Traverse dot notation in json_value_path
            String path = config.json_value_path;
            JsonVariant current = doc.as<JsonVariant>();
            int dotIdx = 0;
            while ((dotIdx = path.indexOf('.')) >= 0) {
                String key = path.substring(0, dotIdx);
                current = current[key];
                path = path.substring(dotIdx + 1);
            }
            if (path.length() > 0) {
                current = current[path];
            }

            if (!current.isNull()) {
                config.current_grid_power = current.as<float>();
                config.last_sensor_time = millis();
                config.sensor_timeout = false;
            }
        }
    }
    http.end();
}

// ==========================================
// Serial RX: Receive Soyosource status frames
// ==========================================
uint8_t rxBuffer[32];
size_t rxIndex = 0;
unsigned long lastRxByteTime = 0;

void handleSerialRx() {
    while (Serial.available()) {
        uint8_t b = Serial.read();
        unsigned long now = millis();

        if (now - lastRxByteTime > 50) {
            rxIndex = 0;
        }
        lastRxByteTime = now;

        if (rxIndex < sizeof(rxBuffer)) {
            rxBuffer[rxIndex++] = b;
        } else {
            rxIndex = 0;
        }

        if (rxIndex >= 10) {
            float v = 0.0f, a = 0.0f;
            uint8_t s = 0;
            for (size_t offset = 0; offset <= rxIndex - 10; offset++) {
                if (SoyosourceProtocol::parseStatusFrame(&rxBuffer[offset], 10, v, a, s)) {
                    config.dc_volts = v;
                    config.dc_amps = a;
                    config.dc_watts = v * a;
                    config.has_dc_feedback = true;
                    rxIndex = 0;
                    break;
                }
            }
        }
    }
}

// ==========================================
// Core Soyosource Regulation & RS485 Send
// ==========================================
void executeRegulation() {
    int targetWatts = 0;

    // Safety priority 1: NotAus button or MQTT command
    if (config.not_aus) {
        targetWatts = 0;
    } 
    // Manual static power mode
    else if (config.mode == MODE_STATIC) {
        targetWatts = constrain(config.static_watt, 0, config.max_power);
    } 
    // Closed-loop automatic regulation modes (MQTT, Shelly, JSON, HTTP)
    else {
        // Safety priority 2: Watchdog check (Failsafe timeout if smart meter silent)
        if (config.sensor_timeout_sec > 0 && config.last_sensor_time > 0) {
            if (millis() - config.last_sensor_time > (config.sensor_timeout_sec * 1000UL)) {
                config.sensor_timeout = true;
            }
        }

        if (config.sensor_timeout) {
            targetWatts = 0;
        } else {
            float gridP = config.current_grid_power;

            // Closed loop tolerance window:
            // If grid power is within tolerance (e.g. -20W <= gridP <= +5W), maintain current feed-in!
            if (gridP >= config.tolerance_export && gridP <= config.tolerance_import) {
                targetWatts = config.current_watts_out;
            } else {
                // Out of balance:
                // gridP > tolerance_import -> importing from grid, increase feed-in
                // gridP < tolerance_export -> exporting to grid, decrease feed-in
                int delta = (int)round(gridP - config.target_offset);
                int newWatts = config.current_watts_out + delta;
                targetWatts = constrain(newWatts, 0, config.max_power);
            }

            // Power division among parallel inverters
            if (config.soyo_count > 1) {
                targetWatts /= config.soyo_count;
            }
        }
    }

    // Safety priority 3: Battery undervoltage protection
    float battV = config.has_dc_feedback ? config.dc_volts : config.ext_battery_volts;
    if (config.min_battery_voltage > 0.0f && battV > 0.0f && battV < config.min_battery_voltage) {
        config.battery_low = true;
        targetWatts = 0;
    } else {
        config.battery_low = false;
    }

    // Safety clamp to max_power
    targetWatts = constrain(targetWatts, 0, config.max_power);
    config.current_watts_out = targetWatts;

    // Transmit 8-byte RS485 frame at 4800 Baud with hardware DE/RE pin control
    SoyosourceProtocol::sendFrame(Serial, (uint16_t)targetWatts, config.rs485_dere_pin);

    config.last_packet_time = millis();
}

void publishTelemetry() {
    if (!mqttClient.connected() || config.mqtt_topic_pub.length() == 0) return;

    StaticJsonDocument<512> doc;
    doc["WattsOut"] = config.current_watts_out;
    doc["L1L2L3"] = config.current_grid_power;
    doc["L1"] = config.current_l1;
    doc["L2"] = config.current_l2;
    doc["L3"] = config.current_l3;
    doc["NotAus"] = config.not_aus;
    doc["Mode"] = config.mode;
    doc["SensorTimeout"] = config.sensor_timeout;
    doc["DCVolts"] = config.has_dc_feedback ? config.dc_volts : config.ext_battery_volts;
    doc["DCAmps"] = config.dc_amps;
    doc["DCWatts"] = config.dc_watts;
    doc["BatteryLow"] = config.battery_low;
    doc["BatterySoC"] = config.battery_soc;
    doc["Uptime"] = getUptimeStr();

    char buf[512];
    serializeJson(doc, buf);
    mqttClient.publish(config.mqtt_topic_pub.c_str(), buf);
}

// ==========================================
// Arduino Setup & Loop
// ==========================================
void setup() {
    config.start_time = millis();
    config.last_sensor_time = millis();
    ConfigManager::loadConfig(config);

    if (config.rs485_dere_pin >= 0) {
        pinMode(config.rs485_dere_pin, OUTPUT);
        digitalWrite(config.rs485_dere_pin, LOW);
    }

    // Soyosource RS485 operates strictly at 4800 Baud, 8N1
    Serial.begin(4800);
    delay(200);

    setupWiFi();
    setupWebServer();
    setupMQTT();

    // Initial zero-frame safety command
    executeRegulation();
}

void loop() {
    server.handleClient();
    handleMQTT();
    handleSerialRx();

    unsigned long now = millis();

    // Periodic sensor polling (every 1.5 seconds)
    if (now - lastPollTime >= 1500) {
        lastPollTime = now;
        if (config.mode == MODE_AUTO_SHELLY) {
            pollShelly();
        } else if (config.mode == MODE_AUTO_JSON) {
            pollCustomJson();
        }
    }

    // Periodic regulation loop
    unsigned long interval = config.wait_seconds * 1000UL;
    if (interval < 1000) interval = 1000;

    if (now - lastRegulationTime >= interval) {
        lastRegulationTime = now;
        executeRegulation();
        publishTelemetry();
    }
}
