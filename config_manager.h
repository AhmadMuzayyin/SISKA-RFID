#pragma once
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

struct Config {
  String ssid;
  String password;
  String serverUrl;
};

Config config;

bool loadConfig() {
  if (!SPIFFS.begin(true)) {
    return false;
  }
  if (!SPIFFS.exists("/config.json")) {
    return false;
  }

  File file = SPIFFS.open("/config.json", "r");
  if (!file) return false;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;

  config.ssid = doc["ssid"].as<String>();
  config.password = doc["password"].as<String>();
  config.serverUrl = "https://www.mqalamin.sch.id/api/rfid/scan";
  return true;
}

bool saveConfig(const String &ssid, const String &password) {
  StaticJsonDocument<512> doc;
  doc["ssid"] = ssid;
  doc["password"] = password;

  File file = SPIFFS.open("/config.json", "w");
  if (!file) return false;
  serializeJson(doc, file);
  file.close();
  return true;
}
