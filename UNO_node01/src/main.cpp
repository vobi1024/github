#include <Arduino.h>

//#include <SPI.h>
//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"
//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;

#include <avr/wdt.h>
#include <everytime.h>

unsigned long previousMillis = 0;

static String nodeID = "node01";

extern String readAltSerial();
extern void mqttpub(String channel, String msg);
extern String FloatToString(float value);

String dstemp = "n/a";

String readDS()
{
        digitalWrite(LED_BUILTIN, 1);
        sensors.requestTemperatures();
        float tempC = 0;
        tempC = sensors.getTempC(tempDeviceAddress);
        char buffer[10];
        String str = dtostrf(tempC, 5, 2, buffer);
        if ((tempC > 60) or (tempC < 0)) str = "n/a";
        digitalWrite(LED_BUILTIN, 0);
        return str;
}

void setup() {
        wdt_disable();
        pinMode(LED_BUILTIN, OUTPUT);
        Serial.begin(19200);
        sensors.begin();
        sensors.getAddress(tempDeviceAddress, 0);
        sensors.setResolution(tempDeviceAddress, 10);
        sensors.setWaitForConversion(false);
        //while (!Serial) ; // wait for Arduino Serial Monitor to open
        //Serial.println("AltSoftSerial Test Begin");
        //  altSerial.begin(19200);
        wdt_enable(WDTO_4S);
        mqttpub("status", "Arduino 01 online");
}

void loop() {

        wdt_reset();
        unsigned long currentMillis = millis();
        if ((unsigned long)(currentMillis - previousMillis) >= 42000) {
                //Serial.println("Sending reset...");
                Serial.println("PUB,/from/" + nodeID + "/status,RESET");
                delay(250);
                Serial.println("RST,,");
                previousMillis = currentMillis;
        }

        every(10000) dstemp = readDS();

        if (Serial.available()) {
                String rxstr = readAltSerial();

                if (rxstr.substring(15, 19) == "tmp1")
                {
                        mqttpub("tmp1", dstemp);
                }

                if (rxstr.substring(0, 3) == "CMD")
                {
                        previousMillis = currentMillis;
                }
        } //incoming msg end
        // AVRsleep();
} //loop
