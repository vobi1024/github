#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <MQTTClient.h>
//#include <everytime.h>

const char ssid[] = "Nexus";
const char pass[] = "Nexus1024";

WiFiClient net;
MQTTClient client;

int count = 0;
char data[64];
String str;

char* valPosition;
int i = 0;
String token[3];

void connect() {
        int rst = 0;
        Serial.print("checking wifi...");
        while (WiFi.status() != WL_CONNECTED) {
                Serial.print(".");
                delay(1000);
                rst++;
                if (rst >= 20)
                        ESP.restart();
        }
        rst = 0;
        Serial.print("\nconnecting...");
        while (!client.connect("node00", "nexus", "letmein")) {
                Serial.print(".");
                delay(1000);
                rst++;
                if (rst >= 20)
                        ESP.restart();
        }
        rst = 0;
        Serial.println("\nconnected!");

        client.subscribe("/to/node00/+", 1);

        // client.unsubscribe("/hello");
        client.publish("/from/node00/status", "ONLINE", 0, 1);
}

void messageReceived(String &topic, String &payload) {
        Serial.print("CMD," + topic + "," + payload);
        if (payload == "RST")
                ESP.restart();
}

void setup() {
        Serial.begin(19200);
        //while (!Serial) {
        //  ; // wait for serial port to connect. Needed for native USB port only
        //}
        WiFi.begin(ssid, pass);

        client.begin("192.168.1.120", net);
        client.setOptions(10, 1, 2500);
        client.onMessage(messageReceived);
        client.setWill("/from/node00/status", "OFFLINE", 0, 1);
        connect();
        client.publish("/from/node00/status", "START", 0, 1);
}

void loop() {
        client.loop();
        delay(10); // <- fixes some issues with WiFi stability
        if (!client.connected()) {
                connect();
        }

        if (Serial.available()) {
                //Serial.println("ESP8266 - New command, collecting...");
                delay(10);
                count = 0;
                for ( int i = 0; i < sizeof(data); ++i )
                        data[i] = (char)0;
                //delay(10);
                while (Serial.available()) {
                        char character = Serial.read();
                        data[count] = character;
                        count++;
                }
                //Serial.print("ESP8266 - Command received: ");
                //Serial.println(data);
                str = String(data);
                //Serial.print("Tokenizing: ");
                //Serial.println(str);
                //delay(10);

                valPosition = strtok(data, ",");
                while (valPosition != NULL) {
                        //Serial.println(valPosition);
                        token[i] = (valPosition);
                        i++;
                        //Here we pass in a NULL value, which tells strtok to continue working with the previous string
                        valPosition = strtok(NULL, ",");
                }
                //Serial.println(token[0]);
                //Serial.println(token[1]);
                //Serial.println(token[2]);
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

                token[0] = "";
                token[1] = "";
                token[2] = "";
                str = ("");
                i = 0;
        } //serial available

} //loop
