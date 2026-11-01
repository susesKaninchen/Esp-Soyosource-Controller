#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(ESP8266)
  #include <LittleFS.h>
  #define FS_NAMESPACE LittleFS
#elif defined(ESP32)
  #include <LittleFS.h>
  #define FS_NAMESPACE LittleFS
#endif

enum ControllerMode {
    MODE_AUTO_MQTT = 0,
    MODE_AUTO_SHELLY = 1,
    MODE_AUTO_JSON = 2,
    MODE_AUTO_HTTP = 3,
    MODE_STATIC = 4
};

struct SystemConfig {
    // WiFi Settings
    String wifi_ssid = "";
    String wifi_pass = "";
    String ap_ssid = "SOYO_CONTROLLER";
    String ap_pass = "12345678";

    // General Controller Settings
    String controller_name = "SOYOSOURCE Controller";
    int mode = MODE_AUTO_MQTT;
    int static_watt = 0;
    int soyo_count = 1;         // Number of parallel inverters (divider)
    int max_power = 900;        // Max wattage limit
    int wait_seconds = 2;       // Regulation interval (seconds)
    int deadband = 15;          // Tolerance threshold in Watts
    int target_offset = 0;      // Offset to avoid grid export (e.g. +10W)
    int tolerance_export = -20; // Grid export threshold before lowering output (W)
    int tolerance_import = 5;   // Grid import threshold before increasing output (W)
    int rs485_dere_pin = 0;     // Hardware DE/RE direction pin (0 = GPIO0/D3, -1 = disabled/auto-flow)
    int sensor_timeout_sec = 30;// Failsafe timeout in seconds (0 = disabled)
    bool not_aus = false;       // Emergency stop flag

    // MQTT Settings
    String mqtt_server = "";
    int mqtt_port = 1883;
    String mqtt_user = "";
    String mqtt_pass = "";
    String mqtt_topic_sub = "shellies/shellyem3/emeter/0/power"; // Default topic
    String mqtt_topic_pub = "Soyosource/AllJsonData";

    // Shelly Settings
    String shelly_ip = "192.168.178.54";
    bool shelly_is_pro = false; // true for Pro 3EM (Gen2 RPC), false for Gen1 3EM
    bool shelly_l1 = true;
    bool shelly_l2 = true;
    bool shelly_l3 = true;

    // Custom JSON HTTP Settings (e.g. Tasmota IR lesekopf)
    String json_http_url = "";
    String json_value_path = "StatusSNS.SML.DJ_TPWRCURR";

    // Telemetry state (runtime, not saved)
    float current_l1 = 0.0f;
    float current_l2 = 0.0f;
    float current_l3 = 0.0f;
    float current_grid_power = 0.0f;
    int current_watts_out = 0;
    unsigned long start_time = 0;
    unsigned long last_packet_time = 0;
    unsigned long last_sensor_time = 0;
    bool sensor_timeout = false;
    String last_error = "";
};

class ConfigManager {
public:
    static const char* CONFIG_FILE;

    static bool loadConfig(SystemConfig& cfg) {
        if (!FS_NAMESPACE.begin()) {
            Serial.println("[FS] Failed to mount LittleFS!");
            return false;
        }

        if (!FS_NAMESPACE.exists(CONFIG_FILE)) {
            Serial.println("[FS] Config file does not exist, creating default...");
            saveConfig(cfg);
            return true;
        }

        File file = FS_NAMESPACE.open(CONFIG_FILE, "r");
        if (!file) {
            Serial.println("[FS] Failed to open config file for reading");
            return false;
        }

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.print("[FS] Failed to parse config JSON: ");
            Serial.println(error.c_str());
            return false;
        }

        if (doc.containsKey("wifi_ssid")) cfg.wifi_ssid = doc["wifi_ssid"].as<String>();
        if (doc.containsKey("wifi_pass")) cfg.wifi_pass = doc["wifi_pass"].as<String>();
        if (doc.containsKey("controller_name")) cfg.controller_name = doc["controller_name"].as<String>();
        if (doc.containsKey("mode")) cfg.mode = doc["mode"].as<int>();
        if (doc.containsKey("static_watt")) cfg.static_watt = doc["static_watt"].as<int>();
        if (doc.containsKey("soyo_count")) cfg.soyo_count = doc["soyo_count"].as<int>();
        if (doc.containsKey("max_power")) cfg.max_power = doc["max_power"].as<int>();
        if (doc.containsKey("wait_seconds")) cfg.wait_seconds = doc["wait_seconds"].as<int>();
        if (doc.containsKey("deadband")) cfg.deadband = doc["deadband"].as<int>();
        if (doc.containsKey("target_offset")) cfg.target_offset = doc["target_offset"].as<int>();
        if (doc.containsKey("tolerance_export")) cfg.tolerance_export = doc["tolerance_export"].as<int>();
        if (doc.containsKey("tolerance_import")) cfg.tolerance_import = doc["tolerance_import"].as<int>();
        if (doc.containsKey("rs485_dere_pin")) cfg.rs485_dere_pin = doc["rs485_dere_pin"].as<int>();
        if (doc.containsKey("sensor_timeout_sec")) cfg.sensor_timeout_sec = doc["sensor_timeout_sec"].as<int>();
        if (doc.containsKey("not_aus")) cfg.not_aus = doc["not_aus"].as<bool>();

        if (doc.containsKey("mqtt_server")) cfg.mqtt_server = doc["mqtt_server"].as<String>();
        if (doc.containsKey("mqtt_port")) cfg.mqtt_port = doc["mqtt_port"].as<int>();
        if (doc.containsKey("mqtt_user")) cfg.mqtt_user = doc["mqtt_user"].as<String>();
        if (doc.containsKey("mqtt_pass")) cfg.mqtt_pass = doc["mqtt_pass"].as<String>();
        if (doc.containsKey("mqtt_topic_sub")) cfg.mqtt_topic_sub = doc["mqtt_topic_sub"].as<String>();
        if (doc.containsKey("mqtt_topic_pub")) cfg.mqtt_topic_pub = doc["mqtt_topic_pub"].as<String>();

        if (doc.containsKey("shelly_ip")) cfg.shelly_ip = doc["shelly_ip"].as<String>();
        if (doc.containsKey("shelly_is_pro")) cfg.shelly_is_pro = doc["shelly_is_pro"].as<bool>();
        if (doc.containsKey("shelly_l1")) cfg.shelly_l1 = doc["shelly_l1"].as<bool>();
        if (doc.containsKey("shelly_l2")) cfg.shelly_l2 = doc["shelly_l2"].as<bool>();
        if (doc.containsKey("shelly_l3")) cfg.shelly_l3 = doc["shelly_l3"].as<bool>();

        if (doc.containsKey("json_http_url")) cfg.json_http_url = doc["json_http_url"].as<String>();
        if (doc.containsKey("json_value_path")) cfg.json_value_path = doc["json_value_path"].as<String>();

        Serial.println("[FS] Configuration loaded successfully.");
        return true;
    }

    static bool saveConfig(const SystemConfig& cfg) {
        if (!FS_NAMESPACE.begin()) {
            Serial.println("[FS] Failed to mount LittleFS!");
            return false;
        }

        File file = FS_NAMESPACE.open(CONFIG_FILE, "w");
        if (!file) {
            Serial.println("[FS] Failed to open config file for writing");
            return false;
        }

        StaticJsonDocument<2048> doc;
        doc["wifi_ssid"] = cfg.wifi_ssid;
        doc["wifi_pass"] = cfg.wifi_pass;
        doc["controller_name"] = cfg.controller_name;
        doc["mode"] = cfg.mode;
        doc["static_watt"] = cfg.static_watt;
        doc["soyo_count"] = cfg.soyo_count;
        doc["max_power"] = cfg.max_power;
        doc["wait_seconds"] = cfg.wait_seconds;
        doc["deadband"] = cfg.deadband;
        doc["target_offset"] = cfg.target_offset;
        doc["tolerance_export"] = cfg.tolerance_export;
        doc["tolerance_import"] = cfg.tolerance_import;
        doc["rs485_dere_pin"] = cfg.rs485_dere_pin;
        doc["sensor_timeout_sec"] = cfg.sensor_timeout_sec;
        doc["not_aus"] = cfg.not_aus;

        doc["mqtt_server"] = cfg.mqtt_server;
        doc["mqtt_port"] = cfg.mqtt_port;
        doc["mqtt_user"] = cfg.mqtt_user;
        doc["mqtt_pass"] = cfg.mqtt_pass;
        doc["mqtt_topic_sub"] = cfg.mqtt_topic_sub;
        doc["mqtt_topic_pub"] = cfg.mqtt_topic_pub;

        doc["shelly_ip"] = cfg.shelly_ip;
        doc["shelly_is_pro"] = cfg.shelly_is_pro;
        doc["shelly_l1"] = cfg.shelly_l1;
        doc["shelly_l2"] = cfg.shelly_l2;
        doc["shelly_l3"] = cfg.shelly_l3;

        doc["json_http_url"] = cfg.json_http_url;
        doc["json_value_path"] = cfg.json_value_path;

        if (serializeJson(doc, file) == 0) {
            Serial.println("[FS] Failed to write config to file");
            file.close();
            return false;
        }

        file.close();
        Serial.println("[FS] Configuration saved successfully.");
        return true;
    }
};

const char* ConfigManager::CONFIG_FILE = "/config.json";
