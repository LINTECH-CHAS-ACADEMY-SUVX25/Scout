#pragma once

// Delade konstanter för Scout RC-protokollet.
// Används av både cam (ESP32-CAM) och screen (ESP32-S3) — ändra här, inte på två ställen.

// WiFi-nätverket som screenen skapar och kameran ansluter till
#define AP_SSID      "Scout_AP"
#define AP_PASS      "scout1234"

// Screenens fasta IP-adress som AP — kameran skickar alltid video hit
#define S3_IP        "192.168.4.1"

// UDP-portar: kameran skickar video på VID_PORT, screenen skickar styrkommandon på CMD_PORT
#define VID_PORT     3334
#define CMD_PORT     3335

// Magic-byte i första UDP-fragmentet för varje bild — används för att verifiera
// att vi faktiskt läser början av en ny bild och inte ett gammalt fragment
#define FRAME_MAGIC  0xAB

// Styrkommandon — skickas som ett enda byte från screen till cam via UDP
#define CMD_STOP     0x00
#define CMD_FORWARD  0x01
#define CMD_BACKWARD 0x02
#define CMD_LEFT     0x04
#define CMD_RIGHT    0x08
