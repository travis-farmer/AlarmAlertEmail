#include <SPI.h>
#include <WiFi101.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"


// Update these with values suitable for your hardware/network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEB };
IPAddress server(192, 168, 1, 26);
IPAddress serverb(192, 168, 1, 42);

// WiFi card example
char ssid[] = WSSID;    // your SSID
char pass[] = WPSWD;       // your SSID Password

String postData;

String lastData = "";

bool flagSec = false;
bool flagFire = false;

// Telemetry Variables
int gblFurnOnOff = 0;
int gblACOnOff = 0;


void callback(char* topic, byte* payload, unsigned int length) {
  String tmpTopic = topic;
  char tmpStr[length+1];
  for (int x=0; x<length; x++) {
    tmpStr[x] = (char)payload[x]; // payload is a stream, so we need to chop it by length.
  }
  tmpStr[length] = 0x00; // terminate the char string with a null

  if (tmpTopic == "telemetry/furnonoff") {int intFurnOnOff = atoi(tmpStr); gblFurnOnOff = intFurnOnOff; }
  else if (tmpTopic == "telemetry/aconoff") {int intACOnOff = atoi(tmpStr); gblACOnOff = intACOnOff; }

}

WiFiClient aclient;
WiFiClient bClient;
PubSubClient client(bClient);

void printTelemetry() {
  char sz[32];

  dtostrf(roomTempF, 4, 2, sz);
  client.publish("telemetry/roomtempf",sz);
  dtostrf(roomTempF, 4, 2, sz);
  client.publish("telemetry/roomtempf",sz);

}

long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect("arduinoClient2")) {
    client.subscribe("telemetry/furnonoff");
    client.subscribe("telemetry/aconoff");
    printTelemetry();

  }
  return client.connected();
}

void secIRQ() {
  if (digitalRead(2) == LOW) flagSec = true;
}

void fireIRQ() {
  if (digitalRead(3) == LOW) flagFire = true;
}

void setup() {

  Serial.begin(9600);

  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(22,INPUT);

  client.setServer(serverb, 1883);
  client.setCallback(callback);

  //WiFi.setPins(53,48,49);
  int status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // print out info about the connection:
  else {
    Serial.println("Connected to network");
    IPAddress ip = WiFi.localIP();
    Serial.print("My IP address is: ");
    Serial.println(ip);
  }

  delay(1500);
  lastReconnectAttempt = 0;

  // attach external interrupt pins to IRQ functions
  attachInterrupt(0, secIRQ, FALLING);
  attachInterrupt(1, fireIRQ, FALLING);

  // turn on interrupts
  interrupts();
}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }

  String alertStr = "";
  if (flagSec == true) {
    flagSec = false;
    alertStr = "S";
  } else {
    alertStr = "s";
  }
  if (flagFire == true) {
    flagFire = false;
    alertStr += "F";
  } else {
    alertStr += "f";
  }
  if (digitalRead(22) == HIGH) {
    alertStr += "P";
  } else {
    alertStr += "p";
  }
  String postVariable = "toemail=";
  postVariable += TOEMAIL;
  postVariable += "&fromemail=";
  postVariable += FROMEMAIL;
  postVariable += "&alarm=";
  postData = postVariable + alertStr;
  if (lastData != postData && alertStr != "**"){
    lastData = postData;
    if (aclient.connect(server, 80)) {
      aclient.println("POST /alarm/alarm-email.php HTTP/1.1");
      aclient.println("Host: www.tjfhome.net");
      aclient.println("Content-Type: application/x-www-form-urlencoded");
      aclient.print("Content-Length: ");
      aclient.println(postData.length());
      aclient.println();
      aclient.print(postData);
    }

    if (aclient.connected()) {
      aclient.stop();
    }
  }

}
