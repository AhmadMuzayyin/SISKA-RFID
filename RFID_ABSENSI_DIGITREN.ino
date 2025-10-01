#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_task_wdt.h"
#include "buzzer.h"
#include "config_manager.h"
#include "web_config.h"

const char *WIFI_SSID = "Bensabuh";
const char *WIFI_PASSWORD = "";

String SERVER_URL = "https://www.mqalamin.sch.id/api/rfid/scan";

#define RC522_SS 5
#define RC522_RST 4
#define SPI_SCK 18
#define SPI_MOSI 23
#define SPI_MISO 19

#define I2C_SDA 21
#define I2C_SCL 22

MFRC522 mfrc522(RC522_SS, RC522_RST);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// gunakan 1 client global untuk menghindari fragmentasi
WiFiClientSecure tlsClient;

String lastUID = "";
unsigned long lastScanTime = 0;
const unsigned long SCAN_COOLDOWN_MS = 2000;
unsigned long lastRFIDActivity = 0;
const unsigned long RFID_IDLE_RESET_MS = 60000;

void lcdShow(const String &line1, const String &line2 = "", uint16_t holdMs = 0)
{
  lcd.clear();

  // ===== Baris 1 =====
  if (line1.length() <= 16)
  {
    int startCol = (16 - line1.length()) / 2; // auto center
    lcd.setCursor(startCol, 0);
    lcd.print(line1);
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print(line1.substring(0, 16));
  }

  // ===== Baris 2 =====
  if (line2.length())
  {
    if (line2.length() <= 16)
    {
      int startCol = (16 - line2.length()) / 2; // auto center
      lcd.setCursor(startCol, 1);
      lcd.print(line2);
    }
    else
    {
      // auto scroll teks panjang
      for (int i = 0; i <= line2.length() - 16; i++)
      {
        lcd.setCursor(0, 1);
        lcd.print(line2.substring(i, i + 16));
        delay(300); // kecepatan scroll
      }
    }
  }

  // ===== Hold tampilan =====
  if (holdMs)
    delay(holdMs);
}

void lcdReady()
{
  lcdShow("Tempelkan Kartu", "Santri Anda");
}

void startAPFallback()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SETUP ABSENSI");
  lcdShow("AP Mode", "SSID: SETUP ABSENSI", 2000);
  lcdShow("Setup via WiFi");
}

bool wifiConnect(uint32_t timeoutMs = 3000)
{
  if (WiFi.status() == WL_CONNECTED)
    return true;

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());

  lcdShow("Menghubungkan", config.ssid);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < timeoutMs)
  {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    lcdShow("WiFi OK", WiFi.localIP().toString(), 1000);
    return true;
  }
  else
  {
    // Gagal nyambung, fallback ke AP
    startAPFallback();
    return false;
  }
}

String uidToHex(const MFRC522::Uid &uid)
{
  String s;
  for (byte i = 0; i < uid.size; i++)
  {
    if (uid.uidByte[i] < 0x10)
      s += "0";
    s += String(uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

bool postUID(const String &uid, String &typeOut, String &messageOut)
{
  typeOut = "";
  messageOut = "";

  wifiConnect();
  if (WiFi.status() != WL_CONNECTED)
  {
    messageOut = "WiFi putus";
    return false;
  }

  HTTPClient http;
  http.setTimeout(6000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(tlsClient, SERVER_URL))
  {
    messageOut = "HTTP begin fail";
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  String body = String("{\"uid\":\"") + uid + "\"}";
  int code = http.POST(body);

  if (code <= 0)
  {
    messageOut = String("HTTP err: ") + http.errorToString(code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (code != 200)
  {
    messageOut = "Kode " + String(code);
    return false;
  }

  StaticJsonDocument<768> doc;
  auto err = deserializeJson(doc, payload);
  if (err)
  {
    messageOut = "JSON parse fail";
    return false;
  }

  typeOut = doc["type"].as<String>();       // aman kalau null → ""
  messageOut = doc["message"].as<String>(); // aman kalau null → ""

  return true;
}

void setup()
{
  Serial.begin(115200);
  buzzerInit();
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcdShow("Memulai sistem...");
  if (!loadConfig())
  {
    startConfigPortal();
    return;
  }
  if (!wifiConnect(3000))
  {
    startConfigPortal();
    return;
  }
  lcdReady();
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, RC522_SS);
  mfrc522.PCD_Init();
  delay(50);
  tlsClient.setInsecure();
  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);
  lastRFIDActivity = millis();
}

void loop()
{
  if (WiFi.getMode() == WIFI_AP)
  {
    handleWebConfig();
    return;
  }

  // Kalau mode STA, pastikan tetap connect
  if (!wifiConnect(3000))
  {
    startConfigPortal();
    return;
  }

  esp_task_wdt_reset();

  // === Proses pembacaan kartu ===
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
  {
    lastRFIDActivity = millis();

    String uid = uidToHex(mfrc522.uid);
    if (uid == lastUID && (millis() - lastScanTime < SCAN_COOLDOWN_MS))
    {
      // abaikan double scan cepat
    }
    else
    {
      lastUID = uid;
      lastScanTime = millis();

      lcdShow("Prosesing....");
      String type, message;
      bool ok = postUID(uid, type, message);
      Serial.println(type);
      Serial.println(message);

      if (!ok)
      {
        lcdShow("Gagal!", "Coba lagi");
        beepFail();
      }
      else
      {
        if (type == "absensi")
        {
          lcdShow("Berhasil!", "Absen Selesai");
          beepSuccess();
        }
        else if (type == "register")
        {
          lcdShow("Berhasil!", " Kartu Di Daftarkan");
          beepSuccess();
        }
        else if (type == "unregistered")
        {
          lcdShow("Gagal!", "Kartu Tidak Terdaftar");
          beepFail();
        }
        else if (type == "invalid_time" || type == "error" || type == "failed")
        {
          lcdShow("Gagal!", message);
          beepFail();
        }
        else if (type == "already_absent")
        {
          lcdShow("Gagal!", "Anda Sudah Absen Hari ini");
          beepFail();
        }
        else
        {
          lcdShow("Gagal!", "Respon Tidak Dikenal");
          beepFail();
        }
      }

      delay(1800);
      lcdReady();
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }

  // Reset reader jika idle terlalu lama
  if (millis() - lastRFIDActivity > RFID_IDLE_RESET_MS)
  {
    Serial.println("RFID idle lama, re-init reader");
    mfrc522.PCD_Init();
    lastRFIDActivity = millis();
  }
}
