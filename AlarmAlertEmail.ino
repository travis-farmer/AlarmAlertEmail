#include <SPI.h>
#include <WiFi101.h>
#include "arduino_secrets.h"


// Update these with values suitable for your hardware/network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEB };
IPAddress server(192, 168, 1, 26);

// WiFi card example
char ssid[] = WSSID;    // your SSID
char pass[] = WPSWD;       // your SSID Password

String postData;

String lastData = "";

WiFiClient client;

void setup() {

  Serial.begin(9600);

  pinMode(2,INPUT);
  pinMode(3,INPUT);

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

}

void loop() {

  String alertStr = "";
  if (digitalRead(2) == HIGH) {
    alertStr = "S";
  } else {
    alertStr = "*";
  }
  if (digitalRead(3) == HIGH) {
    alertStr += "F";
  } else {
    alertStr += "*";
  }
  String postVariable = "toemail=";
  postVariable += TOEMAIL;
  postVariable += "&fromemail=";
  postVariable += FROMEMAIL;
  postVariable += "&alarm=";
  postData = postVariable + alertStr;
  if (lastData != postData && alertStr != "**"){
    lastData = postData;
    if (client.connect(server, 80)) {
      client.println("POST /alarm/alarm-email.php HTTP/1.1");
      client.println("Host: www.tjfhome.net");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.print(postData);

      Serial.println("Email Sent");
      Serial.println(postData);
    }

    if (client.connected()) {
      client.stop();
    }
  }

}
