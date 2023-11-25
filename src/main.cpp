#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

struct Mode
{
  #define Auto 0
  #define Cool 1
  #define Dry 2
  #define Fan 2
  #define Heat 4
  
};


unsigned long originalValue = 0x550000000000;

const uint16_t kIrLed = 5;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example of data captured by IRrecvDumpV2.ino
uint16_t rawData[197] = {6190, 7272,  652, 1522,  648, 1520,  652, 1518,  650, 1524,  646, 1524,  646, 1520,  652, 1520,  650, 1520,  650, 420,  652, 420,  652, 420,  650, 420,  652, 418,  652, 422,  650, 422,  648, 420,  652, 1520,  650, 1520,  648, 1522,  648, 1520,  650, 1522,  624, 1544,  626, 1544,  626, 1544,  626, 444,  626, 446,  648, 422,  646, 426,  626, 442,  626, 444,  628, 444,  628, 444,  628, 1544,  626, 1542,  628, 444,  626, 1544,  624, 1546,  624, 1542,  626, 1546,  624, 446,  626, 446,  624, 448,  622, 1546,  624, 448,  624, 448,  622, 452,  620, 448,  620, 1552,  594, 1576,  592, 478,  590, 480,  590, 1578,  588, 484,  586, 1584,  584, 1584,  584, 1588,  582, 488,  582, 1588,  582, 1586,  582, 490,  582, 1588,  582, 490,  582, 488,  586, 488,  586, 1584,  586, 1582,  612, 460,  612, 458,  614, 1560,  610, 1558,  612, 1558,  608, 462,  610, 462,  610, 460,  610, 1560,  610, 1560,  608, 462,  608, 464,  608, 464,  606, 1562,  608, 466,  604, 1566,  582, 488,  582, 1590,  580, 496,  576, 1590,  580, 492,  580, 1592,  578, 1590,  578, 494,  576, 1596,  574, 496,  574, 1614,  556, 516,  554, 1614,  556, 514,  554, 7368,  554}; 


void CalCommand(int Mode, int Temp, int Air, int FanSpeed, int Swing, int Command)
{
  
}

void setup() {
  irsend.begin();
#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266
}

void loop() {
  Serial.println("sendGoodweather");
  irsend.sendGoodweather(0x558C1A840000);
  delay(2000);
  Serial.println("a rawData capture from IRrecvDumpV2");
  irsend.sendRaw(rawData, 197, 38);  // Send a raw data capture at 38kHz.
  delay(2000);
}