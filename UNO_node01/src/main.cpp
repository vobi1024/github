#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <everytime.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
//#include <SPI.h>
//#include <printf.h>
//#include <stdlib.h>
//#include "LowPower.h"
//#include <AltSoftSerial.h>
//AltSoftSerial altSerial;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
String dstemp = "n/a";

char* valPosition;
String token[4];
const byte numChars = 64;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;
boolean newToken = false;

unsigned long previousMillis = 0;

static String nodeID = "node01";

//------------------------------------functions begin---------------------------------------///

void AVRsleep()
{
        digitalWrite(LED_BUILTIN, 0);
        set_sleep_mode(SLEEP_MODE_IDLE); //sleeps for the rest of this millisecond or less if other trigger
        sleep_enable();
        sleep_mode();     // put the device to sleep
        sleep_disable();
        digitalWrite(LED_BUILTIN, 1);
}

void recvWithStartEndMarkers() {
        static boolean recvInProgress = false;
        static byte ndx = 0;
        char startMarker = '<';
        char endMarker = '>';
        char rc;

        while (Serial.available() > 0 && newData == false) {
                rc = Serial.read();

                if (recvInProgress == true) {
                        if (rc != endMarker) {
                                receivedChars[ndx] = rc;
                                ndx++;
                                if (ndx >= numChars) {
                                        ndx = numChars - 1;
                                }
                        }
                        else {
                                receivedChars[ndx] = '\0'; // terminate the string
                                recvInProgress = false;
                                ndx = 0;
                                newData = true;
                        }
                }

                else if (rc == startMarker) {
                        recvInProgress = true;
                }
        }
}

void processNewData() {
        if (newData == true) {
                Serial.print("This just in ... ");
                Serial.println(receivedChars);
                newData = false;
                int i = 0;
                valPosition = strtok(receivedChars, ",;/");
                while (valPosition != NULL) {
                        //Serial.println(valPosition);
                        token[i] = (valPosition);
                        i++;
                        //Here we pass in a NULL value, which tells strtok to continue working with the previous string
                        valPosition = strtok(NULL, ",;/");
                }
                Serial.println(token[0]);
                Serial.println(token[1]);
                Serial.println(token[2]);
                Serial.println(token[3]);
                newToken = true;
        }
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

//------------------------------------functions end---------------------------------------///

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

        recvWithStartEndMarkers();
        processNewData();

        if (newToken) {
                newToken = false;
                if (token[3] == "tmp1") mqttpub("tmp1", dstemp);
                if (token[0] == "CMD") previousMillis = currentMillis;
        }

        AVRsleep();
} //loop
