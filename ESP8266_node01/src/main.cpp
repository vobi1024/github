#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <MQTTClient.h>
//#include <everytime.h>

//#define DEBUG

const char ssid[] = "Nexus";
const char pass[] = "Nexus1024";

WiFiClient net;
MQTTClient client;

int count = 0;
char data[64];
String str;

//char* valPosition;
//int i = 0;
//String token[3];

char* valPosition;
String token[3];
const byte numChars = 64;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;
boolean newToken = false;

//------------------------------------functions begin---------------------------------------//

void connect() {
        int rst = 0;
        #ifdef DEBUG
        Serial.print("checking wifi...");
        #endif
        while (WiFi.status() != WL_CONNECTED) {
                Serial.print(".");
                delay(500);
                rst++;
                if (rst >= 40)
                        ESP.restart();
        }
        rst = 0;
        #ifdef DEBUG
        Serial.print("\nconnecting...");
        #endif
        while (!client.connect("node01", "nexus", "letmein")) {
                Serial.print(".");
                delay(100);
                rst++;
                if (rst >= 40)
                        ESP.restart();
        }
        rst = 0;

        #ifdef DEBUG
        Serial.println("\nconnected!");
        #endif

        client.subscribe("/to/node01/+", 1);
        client.publish("/from/node01/status", "ONLINE", 0, 1);
}

void messageReceived(String &topic, String &payload) {
        Serial.print("<CMD;" + topic + ";" + payload+ ">");
        if (payload == "RST")
                ESP.restart();
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
                #ifdef DEBUG
                Serial.print ("\nReceived: ");
                Serial.println(receivedChars);
                #endif
                newData = false;
                int i = 0;
                valPosition = strtok(receivedChars, ";");
                while (valPosition != NULL) {
                        //Serial.println(valPosition);
                        token[i] = (valPosition);
                        i++;
                        //Here we pass in a NULL value, which tells strtok to continue working with the previous string
                        valPosition = strtok(NULL, ";");
                }
                #ifdef DEBUG
                Serial.println(token[0]);
                Serial.println(token[1]);
                Serial.println(token[2]);
                #endif
                newToken = true;
        }
}

//------------------------------------functions end---------------------------------------//

void setup() {
        Serial.begin(19200);
        //while (!Serial) {
        //  ; // wait for serial port to connect. Needed for native USB port only
        //}
        WiFi.begin(ssid, pass);
        client.begin("192.168.1.97", net);
        client.setOptions(10, 1, 2500);
        client.onMessage(messageReceived);
        client.setWill("/from/node01/status", "OFFLINE", 0, 1);
        connect();
        client.publish("/from/node01/status", "START", 0, 1);
}

void loop() {

        client.loop();
        delay(10); // <- fixes some issues with WiFi stability
        if (!client.connected()) {
                connect();
        }

        recvWithStartEndMarkers();
        processNewData();

        if (newToken) {
                if (token[0] == "PUB")
                        client.publish(token[1], token[2], 0, 1);
                if (token[0] == "PUB")
                        client.publish(token[1], token[2], 0, 1);
                if (token[0] == "SUB")
                        client.subscribe(token[1], 1);
                if (token[0] == "CSB")
                        client.unsubscribe(token[1]);
                //    if (token[0] == "LWT")
                //      client.setWill(token[1], token[2], 0, 1);
                if (token[0] == "CWT")
                        client.clearWill();
                if (token[0] == "RST")
                        ESP.restart();

                for ( int i = 0; i < 3; ++i )
                        token[i] = "";
                newToken = false;
        }
} //loop
