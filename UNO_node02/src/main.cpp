#include <Arduino.h>
//#include <SPI.h>
//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"
//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

#include "DHT.h"
DHT dht;
#include <Wire.h>
#include <avr/wdt.h>
#include <everytime.h>

float temperature;
float humidity;
String temp = "n/a";
String hum = "n/a";

unsigned long previousMillis = 0;

static String nodeID = "node02";

extern String readAltSerial();
extern void mqttpub(String channel, String msg);
extern String FloatToString(float value);

void setup() {
        wdt_disable();
        pinMode(LED_BUILTIN, OUTPUT);
        dht.setup(4);
        Serial.begin(19200);
        wdt_enable(WDTO_4S);
        mqttpub("status", "Arduino 02 online");
}

void loop() {

        wdt_reset();
        unsigned long currentMillis = millis();
        if ((unsigned long)(currentMillis - previousMillis) >= 122000) {
                //Serial.println("Sending reset...");
                Serial.println("PUB,/from/" + nodeID + "/status,RESET");
                delay(250);
                Serial.println("RST,,");
                previousMillis = currentMillis;
        }

        every(15000) {
                digitalWrite(LED_BUILTIN, 1);
                temp = FloatToString (dht.getTemperature());
                hum = FloatToString (dht.getHumidity());
                digitalWrite(LED_BUILTIN, 0);
        }

        if (Serial.available()) {
                String rxstr = readAltSerial();

                if (rxstr.substring(15, 19) == "tmp1")
                {
                        mqttpub("tmp1", temp);
                }

                if (rxstr.substring(15, 19) == "hum1")
                {
                        mqttpub("hum1", hum);
                }

                if (rxstr.substring(0, 3) == "CMD")
                {
                        previousMillis = currentMillis;
                }

        } // Serial available end
        // AVRsleep();
} //loop
