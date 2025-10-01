#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "config_manager.h"

WebServer server(80);

String htmlForm()
{
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Setup Absensi</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          background: #f3f6fa;
          display: flex;
          justify-content: center;
          align-items: center;
          min-height: 100vh;
          margin: 0;
          padding: 15px;
        }
        .container {
          background: #ffffff;
          padding: 25px 30px;
          border-radius: 12px;
          box-shadow: 0 4px 12px rgba(0,0,0,0.1);
          text-align: center;
          width: 100%;
          max-width: 380px;
        }
        h2 {
          margin-bottom: 20px;
          color: #333;
          font-size: 20px;
        }
        label {
          display: block;
          text-align: left;
          margin: 10px 0 5px;
          font-weight: bold;
          color: #444;
          font-size: 14px;
        }
        input[type="text"], input[type="password"] {
          width: 100%;
          padding: 12px;
          border: 1px solid #ccc;
          border-radius: 8px;
          margin-bottom: 15px;
          font-size: 14px;
          outline: none;
          box-sizing: border-box;
          transition: border-color 0.2s;
        }
        input[type="text"]:focus, input[type="password"]:focus {
          border-color: #007bff;
        }
        input[type="submit"] {
          background: #007bff;
          color: #fff;
          border: none;
          padding: 14px;
          width: 100%;
          border-radius: 8px;
          font-size: 16px;
          cursor: pointer;
          transition: background 0.3s ease;
        }
        input[type="submit"]:hover {
          background: #0056b3;
        }
        #status {
          margin-top: 15px;
          font-weight: bold;
          font-size: 14px;
        }
        /* responsive tweaks */
        @media (max-width: 400px) {
          .container {
            padding: 20px;
            border-radius: 8px;
          }
          h2 {
            font-size: 18px;
          }
          input[type="text"], input[type="password"], input[type="submit"] {
            font-size: 14px;
            padding: 10px;
          }
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>Setup Absensi</h2>
        <form id="configForm">
          <label for="ssid">SSID</label>
          <input type="text" id="ssid" name="ssid" placeholder="Masukkan SSID WiFi">

          <label for="password">Password</label>
          <input type="password" id="password" name="password" placeholder="Masukkan Password WiFi">

          <input type="submit" value="Simpan">
        </form>
        <div id="status"></div>
      </div>

      <script>
        document.getElementById("configForm").addEventListener("submit", async function(e){
          e.preventDefault(); // cegah reload halaman

          let formData = new FormData(this);
          let params = new URLSearchParams(formData);

          let res = await fetch("/save", {
            method: "POST",
            body: params
          });

          let text = await res.text();
          document.getElementById("status").innerHTML = text;
        });
      </script>
    </body>
    </html>
  )rawliteral";
  return html;
}

void startConfigPortal()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SETUP ABSENSI");

  server.on("/", HTTP_GET, []()
            { server.send(200, "text/html", htmlForm()); });

  server.on("/save", HTTP_POST, []()
            {
    String ssid = server.arg("ssid");
    String pass = server.arg("password");

    if (saveConfig(ssid, pass)) {
      server.send(200, "text/html", "<span style='color:green'>✅ Config saved! Restarting...</span>");
      delay(1500);
      ESP.restart();
    } else {
      server.send(500, "text/html", "<span style='color:red'>❌ Failed to save config!</span>");
    } });

  server.begin();
}

void handleWebConfig()
{
  server.handleClient();
}
