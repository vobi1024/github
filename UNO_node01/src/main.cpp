#include <Arduino.h>

//#include <SPI.h>
// #include <avr/sleep.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
#include <avr/wdt.h>

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;

//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"

#include <everytime.h>

//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

unsigned long previousMillis = 0;
//unsigned long currentMillis;

static String nodeID = "node01";

// void AVRsleep()
// {
//         digitalWrite(LED_BUILTIN, 0);
//         set_sleep_mode(SLEEP_MODE_IDLE); //sleeps for the rest of this millisecond or less if other trigger
//         sleep_enable();
//         sleep_mode();     // put the device to sleep
//         sleep_disable();
//         digitalWrite(LED_BUILTIN, 1);
// }

String readAltSerial()
{
        delay(10);
        char c;
        String str = "";
        while (Serial.available() > 0) {
                c = Serial.read();
                //Serial.print(c);
                str += c;
        }
        return str;
}

void mqttpub(String channel, String msg)
{
        Serial.println("PUB,/from/" + nodeID + "/" + channel + "," + msg);
        //altSerial.print("PUB,/from/" + nodeID + "/" + channel + "," + msg);
}

String FloatToString(float value)
{
        char buffer[10];
        String str = dtostrf(value, 5, 2, buffer);
        return str;
}


String readDS()
{
        digitalWrite(LED_BUILTIN, 1);
        sensors.requestTemperatures();
        float tempC = 0;
        //Serial.println(sensors.getTempCByIndex(0));
        tempC = sensors.getTempC(tempDeviceAddress);
        //Serial.println(tempC, 2);
        char buffer[10];
        String str = dtostrf(tempC, 5, 2, buffer);
        if ((tempC > 60) or (tempC < 0)) str = "n/a";
        digitalWrite(LED_BUILTIN, 0);
        return str;
        //tem = ("Node01,tmp1,0," + tem + "\n");
        //tem.toCharArray(temper, 32);
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

String dstemp = "n/a";

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
                //Serial.println(rxstr);
                //Serial.println(rxstr.substring(0,3));

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
