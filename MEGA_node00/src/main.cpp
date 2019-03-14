#include <Arduino.h>
#include <SPI.h>
#undef PN532DEBUGPRINT
//#include <avr/wdt.h>/

//#include "DHT.h"
//DHT dht;

#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // I2C

//#include <printf.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <everytime.h>
#include <EEPROM.h>

#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

static String nodeID = "node00";

//unsigned long previousMillis = 0;

bool newData = 0;
String topic = "";
String data = "";

#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

const uint8_t maxKeyLength = 4;
uint8_t validKeys[][maxKeyLength] = {
        {
                0x36, 0xE6, 0x1D, 0x9E
        }
        ,
        {
                0x06, 0x45, 0x12, 0x9E
        }
        ,
        {
                0x56, 0x28, 0x17, 0x9E
        }
};

int keyCount = sizeof validKeys / maxKeyLength;
bool lockStatus;
String User = "Owner";

float temperature;
float humidity;
float pressure;
String temp = "n/a";
String hum = "n/a";
String pres = "n/a";
String armstate = "n/a";

// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages.
ELClient esp(&Serial, &Serial);

// Initialize CMD client (for GetTime)
ELClientCmd cmd(&esp);

// Initialize the MQTT client
ELClientMqtt mqtt(&esp);

// Callback made from esp-link to notify of wifi status changes
// Here we just print something out for grins
void wifiCb(void* response) {
        ELClientResponse *res = (ELClientResponse*)response;
        if (res->argc() == 1) {
                uint8_t status;
                res->popArg(&status, 1);

                if(status == STATION_GOT_IP) {
                        Serial.println("WIFI CONNECTED");
                } else {
                        Serial.print("WIFI NOT READY: ");
                        Serial.println(status);
                }
        }
}

bool connected;

// Callback when MQTT is connected
void mqttConnected(void* response) {
        Serial.println("MQTT connected!");
        mqtt.subscribe("/to/node00e/#");
        //mqtt.subscribe("/hello/world/#");
        //mqtt.subscribe("/esp-link/2", 1);
        //mqtt.publish("/esp-link/0", "test1");
        connected = true;
        mqtt.publish("/from/node00e/status", "START", 1);
}

// Callback when MQTT is disconnected
void mqttDisconnected(void* response) {
        Serial.println("MQTT disconnected");
        connected = false;
}

// Callback when an MQTT message arrives for one of our subscriptions
void mqttData(void* response) {
        ELClientResponse *res = (ELClientResponse *)response;

        Serial.print("Received: topic=");
        topic = res->popString();
        Serial.println(topic);

        Serial.print("data=");
        data = res->popString();
        Serial.println(data);
        newData = 1;
}

void mqttPublished(void* response) {
        Serial.println("MQTT published");
}

//**------------functions begin--------------**//

char* string2char(String command){
        if(command.length()!=0) {
                char *p = const_cast<char*>(command.c_str());
                return p;
        }
}

// void mqttpub(String channel, String msg)
// {
//         delay(25);
//         Serial1.println("PUB,/from/" + nodeID + "/" + channel + "," + msg);
//         //altSerial.print("PUB,/from/" + nodeID + "/" + channel + "," + msg);
// }

void setLockState(bool state) {
        if (!state)
        {
                //mqttpub("arm1", User + " disarmed!");
                mqtt.publish("/from/node00e/arm1", string2char(User + " disarmed!"), 1);
                // Serial1.print("Node00,arm1,1,0,");
                // Serial1.print(User);
                // Serial1.println(" disarmed!");
                armstate = "Disarmed";
                //mqttpub("arms", armstate);
                mqtt.publish("/from/node00e/arms", string2char(armstate), 1);
                tone(8, 1500, 100);
                delay(100);
                tone(8, 1000, 100);
                delay(100);
                tone(8, 500, 100);
                delay(100);
                //noTone(8);
        }
        else
        {
                //mqttpub("arm1", User + " armed!");
                mqtt.publish("/from/node00e/arm1", string2char(User + " armed!"), 1);
                // Serial1.print("Node00,arm1,1,1,");
                // Serial1.print(User);
                // Serial1.println(" armed!");
                armstate = "Armed";
                //mqttpub("arms", armstate);
                mqtt.publish("/from/node00e/arms", string2char(armstate), 1);
                tone(8, 500, 100);
                delay(100);
                tone(8, 1000, 100);
                delay(100);
                tone(8, 1500, 100);
                delay(100);
        }
        EEPROM.write(0, state);
        digitalWrite(22, state);
        digitalWrite(24, !state);
        lockStatus = state;
        //mqttpub("arms", armstate);
}

String FloatToString(float value)
{
        char buffer[10];
        String str = dtostrf(value, 5, 2, buffer);
        return str;
}

// String readAltSerial()
// {
//         delay(10);
//         char c;
//         String str = "";
//         while (Serial1.available() > 0) {
//                 c = Serial1.read();
//                 //Serial.print(c);
//                 str += c;
//         }
//         return str;
// }

//**------------functions end--------------**//

void setup(void) {
        tone(8, 1000, 500);
        //wdt_disable();
        //pinMode(LED_BUILTIN, OUTPUT);
        pinMode(22, OUTPUT);
        pinMode(24, OUTPUT);
        pinMode(23, INPUT);

        Serial.begin(115200);
        Serial.println("EL-Client starting!");

        // Sync-up with esp-link, this is required at the start of any sketch and initializes the
        // callbacks to the wifi status change callback. The callback gets called with the initial
        // status right after Sync() below completes.
        esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
        bool ok;
        do {
                ok = esp.Sync(); // sync up with esp-link, blocks for up to 2 seconds
                if (!ok) Serial.println("EL-Client sync failed!");
        } while(!ok);
        Serial.println("EL-Client synced!");

        // Set-up callbacks for events and initialize with es-link.
        mqtt.connectedCb.attach(mqttConnected);
        mqtt.disconnectedCb.attach(mqttDisconnected);
        mqtt.publishedCb.attach(mqttPublished);
        mqtt.dataCb.attach(mqttData);
        mqtt.setup();

        //Serial.println("ARDUINO: setup mqtt lwt");
        //mqtt.lwt("/lwt", "offline", 0, 0); //or mqtt.lwt("/lwt", "offline");

        Serial.println("EL-MQTT ready");
        //wdt_disable();
        pinMode(LED_BUILTIN, OUTPUT);

        bool status;
        status = bme.begin();
        if (!status) {
                Serial.println("Could not find a valid BME280 sensor, check wiring!");
                while (1);
        }

        //dht.setup(4);
        //Serial.begin(9600);

        Serial1.begin(19200);

        nfc.begin();
        uint32_t versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {
                //mqttpub("status", User + " Didn't find PN53x board");
                mqtt.publish("/from/node00e/status", string2char(User + " Didn't find PN53x board"), 1);
                while (1); // halt
        }

        //mqttpub("status", "PN53x online");

        // configure board to read RFID tags
        nfc.SAMConfig();
        lockStatus = EEPROM.read(0);
        digitalWrite(22, lockStatus);
        digitalWrite(24, !lockStatus);
        setLockState(lockStatus);
        //wdt_enable(WDTO_4S);
        //mqttpub("status", "Arduino 00 online");
        //mqttpub("arms", armstate);
}

void loop(void) {
        esp.Process();
        digitalWrite(LED_BUILTIN, 0);
        //wdt_reset();
        //unsigned long currentMillis = millis();
        // if ((unsigned long)(currentMillis - previousMillis) >= 42000) {
        //         //Serial.println("Sending reset...");
        //         Serial1.println("PUB,/from/" + nodeID + "/status,RESET");
        //         delay(250);
        //         Serial1.println("RST,,");
        //         previousMillis = currentMillis;
        // }
        uint8_t NFCsuccess;
        uint8_t NFCuid[] = {
                0, 0, 0, 0, 0, 0, 0
        };                            // Buffer to store the returned UID
        uint8_t uidLength;                  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        User = "Owner";

        every(600) {
                NFCsuccess = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, NFCuid, &uidLength, 200);
        }

        if (NFCsuccess) {
                // Display some basic information about the card
                //Serial.println("Found an ISO14443A card");
                //Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
                //nfc.PrintHex(NFCuid, uidLength);

                bool valid = false;

                // Compare this key to the valid once registered here in sketch
                for (int i = 0; i < keyCount && !valid; i++) {
                        for (int j = 0; j < uidLength && !valid; j++) {
                                if (NFCuid[j] != validKeys[i][j]) {
                                        break;
                                }
                                if (j == uidLength - 1) {
                                        switch (i) {
                                        case 0:
                                                User = "Kata";
                                                break;
                                        case 1:
                                                User = "Andras";
                                                break;
                                        }
                                        valid = true;
                                }
                        }
                }
                if (valid) {
                        // Switch lock status
                        setLockState(!lockStatus);
                }

                // Wait for card/tag to leave reader
                //while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, NFCuid, &uidLength, 200));
        }

        every(1000) {
                digitalWrite(LED_BUILTIN, 1);
                if (!lockStatus) digitalWrite(22, !digitalRead(23));
                if (lockStatus) digitalWrite(24, !digitalRead(23));
                // if (lockStatus & digitalRead(23)) {
                if (digitalRead(23)) {
                        // Serial1.println("Node00,mot1,1,1,Movement detected!"); //Movement detected!
                        //mqttpub("alm1", "Movement detected!");
                        Serial.println("Movement detected!");
                        mqtt.publish("/from/node00e/alm1", string2char("Movement detected!"), 1);
                        digitalWrite(LED_BUILTIN, 0);
                }
        }

        every(10000) {
                digitalWrite(LED_BUILTIN, 1);
                float temperature = bme.readTemperature();
                temp = FloatToString (temperature-1);
                hum = FloatToString (bme.readHumidity());
                pres = FloatToString (bme.readPressure() / 100.0F);
                digitalWrite(LED_BUILTIN, 0);
        }

        if (newData) {
                digitalWrite(LED_BUILTIN, 1);

                if (topic.substring(12, 16) == "tmp1")
                {
                        mqtt.publish("/from/node00e/tmp1", string2char(temp), 1);
                        Serial.print("MQTT published: ");
                        Serial.println(temp);
                }

                if (topic.substring(12, 16) == "hum1")
                {
                        mqtt.publish("/from/node00e/hum1", string2char(hum), 1);
                        Serial.print("MQTT published: ");
                        Serial.println(hum);
                }

                if (topic.substring(12, 16) == "prs1")
                {
                        //mqttpub("dht1", temp + ";" + hum);
                        mqtt.publish("/from/node00e/prs1", string2char(pres), 1);
                        Serial.print("MQTT published: ");
                        Serial.println(pres);
                }

                if (topic.substring(12, 16) == "arms")
                {
                        setLockState(!lockStatus);
                        //mqttpub("arms", armstate);
                        //mqtt.publish("/from/node00e/arms", string2char(armstate), 1);
                }

                if (topic.substring(12, 16) == "arma")
                {
                        //mqttpub("arms", armstate);
                        mqtt.publish("/from/node00e/arms", string2char(armstate), 1);
                }

                if (topic.substring(12, 16) == "arq1")
                {
                        float sensorValue;
                        for(int x = 0; x < 100; x++)
                        {
                                sensorValue = sensorValue + analogRead(A15);
                        }
                        sensorValue = sensorValue/100.0;
                        mqtt.publish("/from/node00e/arq1", string2char(FloatToString(sensorValue)), 1);
                        Serial.print("MQTT published: ");
                        Serial.println(sensorValue);
                }

                if (topic.substring(12, 16) == "lvl1")
                {
                        float sensorValue;
                        for(int x = 0; x < 100; x++)
                        {
                                sensorValue = sensorValue + analogRead(A14);
                        }
                        sensorValue = sensorValue/100.0;
                        sensorValue = 1024 - sensorValue;
                        mqtt.publish("/from/node00e/lvl1", string2char(FloatToString(sensorValue)), 1);
                        Serial.print("MQTT published: ");
                        Serial.println(sensorValue);
                }
                //
                // if (rxstr.substring(0, 3) == "CMD")
                // {
                //         //previousMillis = currentMillis;
                // }
                newData = 0;
                digitalWrite(LED_BUILTIN, 0);
        }
}
