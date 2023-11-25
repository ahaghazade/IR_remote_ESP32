/* IRremoteESP8266: IRsendDemo - demonstrates sending IR codes with IRsend.
 *
 * Version 1.1 January, 2019
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 *
 * An IR LED circuit *MUST* be connected to the ESP8266 on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLed = 5;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example of data captured by IRrecvDumpV2.ino
uint16_t rawData[197] = {6190, 7272,  652, 1522,  648, 1520,  652, 1518,  650, 1524,  646, 1524,  646, 1520,  652, 1520,  650, 1520,  650, 420,  652, 420,  652, 420,  650, 420,  652, 418,  652, 422,  650, 422,  648, 420,  652, 1520,  650, 1520,  648, 1522,  648, 1520,  650, 1522,  624, 1544,  626, 1544,  626, 1544,  626, 444,  626, 446,  648, 422,  646, 426,  626, 442,  626, 444,  628, 444,  628, 444,  628, 1544,  626, 1542,  628, 444,  626, 1544,  624, 1546,  624, 1542,  626, 1546,  624, 446,  626, 446,  624, 448,  622, 1546,  624, 448,  624, 448,  622, 452,  620, 448,  620, 1552,  594, 1576,  592, 478,  590, 480,  590, 1578,  588, 484,  586, 1584,  584, 1584,  584, 1588,  582, 488,  582, 1588,  582, 1586,  582, 490,  582, 1588,  582, 490,  582, 488,  586, 488,  586, 1584,  586, 1582,  612, 460,  612, 458,  614, 1560,  610, 1558,  612, 1558,  608, 462,  610, 462,  610, 460,  610, 1560,  610, 1560,  608, 462,  608, 464,  608, 464,  606, 1562,  608, 466,  604, 1566,  582, 488,  582, 1590,  580, 496,  576, 1590,  580, 492,  580, 1592,  578, 1590,  578, 494,  576, 1596,  574, 496,  574, 1614,  556, 516,  554, 1614,  556, 514,  554, 7368,  554}; 
// Example Samsung A/C state captured from IRrecvDumpV2.ino
uint8_t samsungState[kSamsungAcStateLength] = {
    0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
    0x01, 0xE2, 0xFE, 0x71, 0x40, 0x11, 0xF0};

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
  Serial.println("Sony");
  irsend.sendSony(0xa90, 12, 2);  // 12 bits & 2 repeats
  delay(2000);
  Serial.println("a rawData capture from IRrecvDumpV2");
  irsend.sendRaw(rawData, 197, 38);  // Send a raw data capture at 38kHz.
  delay(2000);
  Serial.println("a Samsung A/C state from IRrecvDumpV2");
  irsend.sendSamsungAC(samsungState);
  delay(2000);
}