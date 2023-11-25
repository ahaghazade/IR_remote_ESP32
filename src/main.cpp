#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"
#include <WiFiMulti.h>
#include <ESPmDNS.h>

#define DNSNAME "ircontrol"
AsyncWebServer serverIR(80);
WiFiClient client;
WiFiMulti wifiMulti;

#define BlueLedPin 2  

//manufactors
#define Goodweather 0
#define Sony 5
#define Samsung 2
#define LG 3



const uint16_t kIrLed = 5;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

void SendToIR(String Command, int Type)
{
  switch (Type)
  {
  case Goodweather:
   const uint64_t result = strtoull(Command.substring(2).c_str(), NULL, 16);
    Serial.println(result);
    uint64_t a = 0x558C1A840000;
    Serial.println(a);
    irsend.sendGoodweather(result);
    break;

  case Sony:
    // irsend.sendSony(Command.c_str());
    Serial.println("Type: Sony");
    break;

  default:
    Serial.println("Not Valid Type");
    break;
  }

  // Serial.println("sendGoodweather");
  // irsend.sendGoodweather(0x558C1A840000);
}

void setup() {
  Serial.begin(115200);
  pinMode(BlueLedPin,OUTPUT);

  irsend.begin();
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("IGH-Wifi", "Konect210");
  // wifiMulti.addAP("MobinNet20", "K6YJScyY");
  wifiMulti.addAP("Irancell-TF-i60-B6A7_1", "@tm@1425#@tm@");
  Serial.println("Connecting Wifi...");
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  delay(1000);

  serverIR.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32. My IP: " + WiFi.localIP().toString());
    digitalWrite(BlueLedPin , !digitalRead(BlueLedPin));
  });

  serverIR.onRequestBody(
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            if ((request->url() == "/command") &&
                (request->method() == HTTP_POST))
            {
                const size_t        JSON_DOC_SIZE   = 512U;
                DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
                
                if (DeserializationError::Ok == deserializeJson(jsonDoc, (const char*)data))
                {
                  String Command = "";
                  int Type = -1;
                  Serial.println("===============");
                    JsonObject obj = jsonDoc.as<JsonObject>();
                    if (obj.containsKey("command"))
                    {
                      Command = obj["command"].as<String>();
                      Serial.print("Command: ");
                      Serial.println(Command);
                    }
                    else
                      Serial.println("Command key not exist...");

                     if (obj.containsKey("type"))
                    {
                      Type = obj["type"].as<int>();
                      Serial.print("Type: ");
                      Serial.println(Type);
                    }
                    else
                      Serial.println("Type key not exist...");

                    if(Type != -1 && Command != "")
                      SendToIR(Command, Type);
                }

                request->send(200, "application/json", "{ \"status\": 0 }");
            }
        }
    );

  serverIR.begin();
  Serial.println("HTTP server started");

  while(!MDNS.begin(DNSNAME))
  {
     Serial.println("Starting mDNS...");
     delay(1000);
  }
  Serial.println("MDNS started");
}

void loop() {
  delay(1000);
}