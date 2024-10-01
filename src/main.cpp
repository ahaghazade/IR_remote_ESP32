#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRac.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h> 
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <AsyncElegantOTA.h>

// ==================== start of TUNEABLE PARAMETERS ====================
const uint16_t kRecvPin = 17;
const uint16_t kIrLedPin = 23;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint16_t kFrequency = 38000;  // 38kHz

IRsend irsend(kIrLedPin);
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
decode_results results;

IRFujitsuAC ac(kIrLedPin); 
//========= Save Json ===========
const char* CommandsDocpath   = "/LastCommands.json";
JsonDocument CommandsDoc;
const char* LastAcStatuspath   = "/AcStatus.json";
JsonDocument LastAcStatus; //{"type" : ""}
//========= AC Control ===========
bool PowerState = true;
int  TempState = 24;
bool SwingVState = false;
bool SwingHState = false;
int  FanState = 1;

void AcConfig(String type, bool power, int model, String mode, bool celsiusTrue = true, int degrees = 25 ,int fanspeed = 1,
     bool swingV = false, bool swingH = false, bool light = false, bool beep = false, bool econo = false,
     bool filter = false, bool turbo = false, bool quiet = false, int sleep = -1, bool clean = false,
     int clock = -1 )
{
  ac.setModel(fujitsu_ac_remote_model_t::ARDB1);  
  if (mode == "Cool")
    ac.setMode(kFujitsuAcModeCool);
  else if (mode == "Dry")
    ac.setMode(kFujitsuAcModeDry);
  else if (mode == "Heat")
    ac.setMode(kFujitsuAcModeHeat);
  else if (mode == "Fan")
    ac.setMode(kFujitsuAcModeFan);
  else if (mode == "Auto")
    ac.setMode(kFujitsuAcModeAuto);

  ac.setCelsius(celsiusTrue);
  ac.setTemp(degrees);
  if (fanspeed == 1)
    ac.setFanSpeed(kFujitsuAcFanLow);  
  else if (fanspeed == 2)
    ac.setFanSpeed(kFujitsuAcFanMed);  
  else if (fanspeed == 3)
    ac.setFanSpeed(kFujitsuAcFanHigh);  
  else if (fanspeed == 4)
    ac.setFanSpeed(kFujitsuAcFanAuto);  
    
  // if(swingV)
  //   ac.next.swingv = stdAc::swingv_t::kHighest;  // Don't swing the fan up or down.
  // else
  //   ac.next.swingv = stdAc::swingv_t::kOff;
  ac.setSwing(kFujitsuAcSwingVert);
//   ac.setSwing(kFujitsuAcSwingHoriz);
//   ac.toggleSwingHoriz(true);
//   ac.toggleSwingHoriz(true);
  // if(swingH)
  //   ac.next.swingh = stdAc::swingh_t::kMiddle;  // Don't swing the fan up or down.
  // else
  //   ac.next.swingh = stdAc::swingh_t::kOff;

  // ac.next.light = light;  // Turn off any LED/Lights/Display that we can.
  // ac.next.beep = beep;  // Turn off any beep from the A/C if we can.
  // ac.next.econo = econo;  // Turn off any economy modes if we can.
  // ac.next.filter = filter;  // Turn off any Ion/Mold/Health filters if we can.
  // ac.next.turbo = turbo;  // Don't use any turbo/powerful/etc modes.
  // ac.next.quiet = quiet;  // Don't use any quiet/silent/etc modes.
  // ac.next.sleep = sleep;  // Don't set any sleep time or modes.
  // ac.next.clean = clean;  // Turn off any Cleaning options if we can.
  // ac.next.clock = clock;  // Don't set any current time if we can avoid it.
  ac.setPower(power);
}
//======= wifi and network ======
bool WifiConnected = false;
IPAddress localIP;
AsyncWebServer server(80);
String ssidSPIFF = "Sharif-WiFi";
String passSPIFF = "";
String MQQTipSPIFF;
const char* ssidPath   = "/ssid.txt";
const char* passPath   = "/pass.txt";
const char* mqttipPath = "/ip.txt";
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
//======= Timer variables =======
unsigned long previousMillisWifi = 0;
const long intervalWifi = 10000;

unsigned long previousMillisIRRec = 0;
const long intervalIRRec = 10000;
bool IRRecFlag = false;
String IRName = "";
//========= websocket ===========
AsyncWebSocket ws("/ws");
void notifyClients(String text) {
  ws.textAll(text);
}

void SendCommand(String CommandName);

void saveJson(const char* filename, JsonDocument& doc) {
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  serializeJson(doc, file);
  file.close();
}

void loadJson(const char* filename, JsonDocument& doc) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  deserializeJson(doc, file);
  file.close();
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    Serial.println(message);

    // Parse the message as JSON
    JsonDocument doc;
    deserializeJson(doc, message);
    if (doc.containsKey("Command"))
    {
      Serial.println("Json is valid");
      String command = doc["Command"].as<String>();
      if (command == "start") {
        IRRecFlag = true;
        previousMillisIRRec = millis();
        IRName = doc["Name"].as<String>();  // Capture the IR name
        Serial.print("IRRecFlag change sate : ");Serial.println(IRRecFlag);
        Serial.print("IRName set to : ");Serial.println(IRName);
      }
      else if(command == "on")
      {
        String buttonName = doc["Name"].as<String>();  // Capture the IR name
        Serial.print("buttonName: ");Serial.println(buttonName);
        SendCommand(buttonName);
      }
      else if (command == "reset") {
        CommandsDoc.clear();
        Serial.print("CommandsDoc cleared");
      }
    }

    else
    {
      if (doc.containsKey("power"))
      {
        Serial.println("Json is valid");
        String powerstatus = doc["power"].as<String>();
        Serial.print("update power sate : ");Serial.println(powerstatus);
        LastAcStatus["power"] = powerstatus;
        if (powerstatus == "on")
          PowerState = true;
        else
          PowerState = false;
      }

      if (doc.containsKey("temperature"))
      {
        Serial.println("Json is valid");
        int temperature = doc["temperature"].as<int>();
        Serial.print("update temperature sate : ");Serial.println(temperature);
        TempState = temperature;
        LastAcStatus["temperature"] = temperature;
      }

      if (doc.containsKey("swing_vertical"))
      {
        Serial.println("Json is valid");
        String swing_vertical = doc["swing_vertical"].as<String>();
        Serial.print("update swing_vertical sate : ");Serial.println(swing_vertical);
        LastAcStatus["swing_vertical"] = swing_vertical;
        if (swing_vertical == "on")
          SwingVState = true;
        else
          SwingVState = false;
      }

      if (doc.containsKey("swing_horizontal"))
      {
        Serial.println("Json is valid");
        String swing_horizontal = doc["swing_horizontal"].as<String>();
        Serial.print("update swing_horizontal sate : ");Serial.println(swing_horizontal);
        LastAcStatus["swing_horizontal"] = swing_horizontal;
        if (swing_horizontal == "on")
          SwingHState = true;
        else
          SwingHState = false;
      }

      if (doc.containsKey("fan_speed"))
      {
        Serial.println("Json is valid");
        String fan_speed = doc["fan_speed"].as<String>();
        Serial.print("update fan_speed sate : ");Serial.println(fan_speed);
        LastAcStatus["fan_speed"] = fan_speed;
        if (fan_speed == "low")
          FanState = 1;
        else if (fan_speed == "medium")
          FanState = 2;
        else if (fan_speed == "high")
          FanState = 3;
        else if (fan_speed == "auto")
          FanState = 4;
      }
      
      AcConfig("FUJITSU",PowerState ,2 ,"Cool",true,TempState, FanState, SwingVState, SwingHState);
      ac.send();
      Serial.println("FUJITSU A/C remote is in the following state:");
      Serial.printf("  %s\n", ac.toString().c_str());
      saveJson(LastAcStatuspath, LastAcStatus);
    }

  }                   
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

bool initWiFi() {

    WiFi.mode(WIFI_STA);

    if (ssidSPIFF == "MobinNet20")
    {
      IPAddress local_IP(192, 168, 100, 210);
      IPAddress gateway(192, 168, 100, 1);
      IPAddress subnet(255, 255, 255, 0);
      WiFi.config(local_IP, gateway, subnet);
    }
    else if (ssidSPIFF == "Sharif-WiFi")
    {
      IPAddress local_IP(172, 27, 10, 152);
      IPAddress gateway(172, 27, 10, 1);
      IPAddress subnet(255, 255, 254, 0);
      WiFi.config(local_IP, gateway, subnet);
    }
    if (passSPIFF == "")
      WiFi.begin(ssidSPIFF);
    
    else
      WiFi.begin(ssidSPIFF.c_str(), passSPIFF.c_str());
   
    // WiFi.begin("Sharif-WiFi");
    // WiFi.begin("Myphone" , "amir1996");

    Serial.print("Connecting to ");
    Serial.print(ssidSPIFF);

    unsigned long currentMillis = millis();
    previousMillisWifi = currentMillis;

    while(WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(250);
      currentMillis = millis();
      if (currentMillis - previousMillisWifi >= intervalWifi) {
        Serial.println("Failed to connect.");
        return false;
      }
    }
    Serial.print("\nConneted: ");
    Serial.println(WiFi.localIP());
    String ipString = WiFi.localIP().toString();
    Serial.println(WiFi.gatewayIP());
    Serial.println(ipString);
    if (ipString.startsWith("172")) //login to shrif net
    {
      Serial.println("Connecting to net2");
      HTTPClient http;
      String url = "https://net2.sharif.edu/login";
      String payload = "username=a.aghazade&password=sharif1400";

      http.begin(url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      int httpCode = http.POST(payload);
      String response = http.getString();

      Serial.print("HTTP Code: ");
      Serial.println(httpCode);
      Serial.print("Response: ");
      Serial.println(response);

      http.end();
      }
      else
      {
        Serial.println("IP is not for Sharif uni");
      }
    return true;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}
// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}
// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

void sendSavedIRData();

bool isDuplicate(String resultString, String Name);

String StateString(decode_results ResultsData)
{
  uint16_t nbytes = ceil(static_cast<float>(ResultsData.bits) / 8.0);
  String output = "";
  output += F("uint8_t state[");
  output += uint64ToString(nbytes);
  output += F("] = {");
  for (uint16_t i = 0; i < nbytes; i++) {
    output += F("0x");
    if (ResultsData.state[i] < 0x10) output += '0';
    output += uint64ToString(ResultsData.state[i], 16);
    if (i < nbytes - 1) output += kCommaSpaceStr;
  }
  output += F("};\n");
  return output;
}

void SaveIrData(decode_results ResultsData, String Name)
{
  Name.toLowerCase();
  
  if(CommandsDoc.containsKey(Name))
  {
    CommandsDoc[Name].clear();
  }
  
  uint16_t *raw_array = resultToRawArray(&ResultsData);
  uint16_t size = getCorrectedRawLength(&ResultsData);
  
  Serial.println("Sending First time:");
  irsend.sendRaw(raw_array, size, kFrequency);

  JsonObject newEntry = CommandsDoc.createNestedObject(Name);
  // Store the raw data as an array
  JsonArray dataArray = newEntry.createNestedArray("data");
  for (uint16_t i = 0; i < size; i++) {
    dataArray.add(raw_array[i]);  // Save each raw value
  }
  CommandsDoc[Name]["SData"] = StateString(ResultsData);

  saveJson(CommandsDocpath, CommandsDoc);

  // serializeJsonPretty(CommandsDoc, Serial);  

  JsonDocument SaveStatus;
  SaveStatus["status"] = "saved";
  SaveStatus["name"] = Name;
  String message;
  serializeJson(SaveStatus, message);
  notifyClients(message);
}

void PrintIrData(decode_results ResultsData)
{
  uint16_t size = getCorrectedRawLength(&ResultsData);
  decode_type_t protocol = results.decode_type;
  bool success = true;
  uint32_t now = millis();
  Serial.printf(
      "%06u.%03u: A %d-bit %s message was %ssuccessfully retransmitted.\n",
      now / 1000, now % 1000, size, typeToString(protocol).c_str(),
      success ? "" : "un");
  String description = IRAcUtils::resultAcToString(&ResultsData);
  if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
  yield();  // Feed the WDT as the text output can take a while to print.
  Serial.println(resultToSourceCode(&ResultsData));
}

void StartServers()
{
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html", false);
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/wifimanager.html", "text/html");
  });
  
  server.serveStatic("/config", SPIFFS, "/config");
  
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) {
          ssidSPIFF = p->value().c_str();
          Serial.print("SSID set to: ");
          Serial.println(ssidSPIFF);
          // Write file to save value
          writeFile(SPIFFS, ssidPath, ssidSPIFF.c_str());
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          passSPIFF = p->value().c_str();
          Serial.print("Password set to: ");
          Serial.println(passSPIFF);
          // Write file to save value
          writeFile(SPIFFS, passPath, passSPIFF.c_str());
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) {
          MQQTipSPIFF = p->value().c_str();
          Serial.print("MQQTipSPIFF Address set to: ");
          Serial.println(MQQTipSPIFF);
          // Write file to save value
          writeFile(SPIFFS, mqttipPath, MQQTipSPIFF.c_str());
        }
      }
    }
    request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + MQQTipSPIFF);
    delay(3000);
    ESP.restart();
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "application/javascript");
  });

  server.on("/manuals", HTTP_GET, [](AsyncWebServerRequest *request){
    String massage;
    serializeJson(CommandsDoc, massage);
    Serial.println("CommandsDoc keys:");
    for (JsonPair kv : CommandsDoc.as<JsonObject>())
      Serial.println(kv.key().c_str());  
    
    Serial.println("--==--==--==");

    request->send(200, "application/json", massage);
  });

  server.on("/acstatus", HTTP_GET, [](AsyncWebServerRequest *request){
    String acStatus;
    serializeJson(LastAcStatus, acStatus);
    Serial.println("LastAcStatus:");
    serializeJsonPretty(LastAcStatus, Serial);
    Serial.println("");
    request->send(200, "application/json", acStatus);
  });

  server.onRequestBody(
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            if ((request->url() == "/api/gateway/write") &&
                (request->method() == HTTP_POST))
            {
                const size_t        JSON_DOC_SIZE   = 512U;
                JsonDocument jsonDoc;
                
                if (DeserializationError::Ok == deserializeJson(jsonDoc, (const char*)data))
                {
                    JsonObject obj = jsonDoc.as<JsonObject>();
                    Serial.print("Write Req: ");
                    serializeJsonPretty(jsonDoc, Serial);
                    Serial.println();
                    
                    if (obj.containsKey("value") && obj.containsKey("addressId"))
                    {
                      Serial.println("Json is valid");
                      int WriteValue  = obj["value"].as<int>();
                      String WriteAdd = obj["addressId"].as<String>();
                      request->send(200, "application/json", "Response");
                    }
                    else
                      Serial.println("Invalid JSON");
                      request->send(200, "application/json", "{ \"status\": 404, \"massage\" : \"Invalid JSON\" , \"addressId\":\"-1/-1/-1\"}");
                }
            }
      });

  server.begin();
  while(!MDNS.begin("remote"))
  {
    Serial.println("Starting mDNS...");
    delay(1000);
  }
  Serial.println("MDNS started");

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  Serial.println("HTTP server started");

  // client.setServer(MQQTipSPIFF.c_str(), mqttPort);
  // client.setCallback(MQTTcallback);
  WifiConnected = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(50);

  initSPIFFS();
  loadJson(CommandsDocpath, CommandsDoc);
  loadJson(LastAcStatuspath, LastAcStatus); //AppAdd_ActPath
  Serial.println("Act Relay Status loaded");
  serializeJsonPretty(LastAcStatus, Serial);
  Serial.println("");
  // Load values saved in SPIFFS
  // ssidSPIFF = readFile(SPIFFS, ssidPath);
  // passSPIFF = readFile(SPIFFS, passPath);
  MQQTipSPIFF = readFile(SPIFFS, mqttipPath);

  Serial.println(ssidSPIFF);
  Serial.println(passSPIFF);
  Serial.println(MQQTipSPIFF);

  if(initWiFi()) {
    Serial.println("WiFi Connected");
  }
  else 
  {
      WifiConnected = false;
      // Connect to Wi-Fi network with SSID and password
      Serial.println("Setting AP (Access Point)");
      // NULL sets an open Access Point
      WiFi.softAP("KRemote-WIFI-MANAGER", NULL);

      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);   
  }

  StartServers();
  irrecv.enableIRIn();
  irsend.begin();
  Serial.println("SmartIRRepeater is now running...");
  ac.begin();
  ac.on();
}

void loop() {
  // Serial.println(millis() - previousMillisIRRec);
  if(IRRecFlag)
  {
    Serial.println("==== Recieve Mode ====");
    irrecv.resume();
    IRRecFlag = false;
    while(millis() - previousMillisIRRec <= intervalIRRec)
    {
      if (irrecv.decode(&results)) 
      {
        String resultString = StateString(results);

        if (!isDuplicate(resultString, IRName)) 
        {
          SaveIrData(results, IRName);
          // Display a crude timestamp & notification.
          PrintIrData(results);

          delay(1000);
          // Serial.println("\nSending data for second time...");
          // SendCommand(IRName);
          break;
        } 
        else 
        {
          Serial.println("Duplicate IR message, not storing.");
        }
        irrecv.resume();
      }
    }

    yield();
  }
  // delay(500);
}

bool isDuplicate(String resultString, String Name) {
  Name.toLowerCase();

  for (JsonPair kv : CommandsDoc.as<JsonObject>()) {
    // if(kv.key().c_str() == Name.c_str())
    //   return true;

    JsonObject item = kv.value().as<JsonObject>(); 
    if (item["SData"].as<String>() == resultString) {
      return true;
    }
  }
  return false;
}
// Function to send each saved result using irsend.sendRaw
void sendSavedIRData() {
  for (JsonPair kv : CommandsDoc.as<JsonObject>()) {
    JsonObject item = kv.value().as<JsonObject>();  // Access the value as a JsonObject
    JsonArray dataArray = item["data"].as<JsonArray>();

    // Prepare the raw data array for sending
    uint16_t rawArray[dataArray.size()];
    for (uint16_t i = 0; i < dataArray.size(); i++) {
      rawArray[i] = dataArray[i];
    }

    // Send the saved IR data
    irsend.sendRaw(rawArray, dataArray.size(), kFrequency);
    Serial.println("IR data sent from saved entry.");
    delay(1000);
  }
}

void SendCommand(String CommandName)
{
  JsonObject item = CommandsDoc[CommandName].as<JsonObject>();  // Access the value as a JsonObject
  JsonArray dataArrays = item["data"].as<JsonArray>();

  // Prepare the raw data array for sending
  uint16_t rawArray[dataArrays.size()];
  for (uint16_t i = 0; i < dataArrays.size(); i++) {
    rawArray[i] = dataArrays[i];
  }

  // Send the saved IR data
  irsend.sendRaw(rawArray, dataArrays.size(), kFrequency);
  Serial.println("IR data sent from saved entry.");
}

