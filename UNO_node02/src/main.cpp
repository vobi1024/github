#include <Arduino.h>

//#include <SPI.h>
// #include <avr/sleep.h>

#include "DHT.h"
DHT dht;
#include <Wire.h>
#include <avr/wdt.h>

float temperature;
float humidity;

unsigned long previousMillis = 0;
//unsigned long currentMillis;

String temp = "n/a";
String hum = "n/a";

//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"

#include <everytime.h>

//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

static String nodeID = "node02";

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

void setup() {
        wdt_disable();
        pinMode(LED_BUILTIN, OUTPUT);
//  Serial.begin(9600);
        dht.setup(4);
        //while (!Serial) ; // wait for Arduino Serial Monitor to open
        //Serial.println("AltSoftSerial Test Begin");
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
                //Serial.println(rxstr);

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

        }
        // AVRsleep();
} //loop
