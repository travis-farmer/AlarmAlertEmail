//#define USE_ETHERNET 1


#include <SPI.h>
#ifdef USE_ETHERNET
  #include <Ethernet.h>
#else
  #include <WiFi101.h>
#endif // USE_ETHERNET
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <PZEM004T.h>

PZEM004T* pzema;
PZEM004T* pzemb;
IPAddress ipa(192,168,1,1);
IPAddress ipb(192,168,1,2);

// Update these with values suitable for your hardware/network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEB };
IPAddress server(192, 168, 1, 211);
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
int gblThermSet = 0;
float fltRoomTemp = 0.00;

void callback(char* topic, byte* payload, unsigned int length) {
  String tmpTopic = topic;
  char tmpStr[length+1];
  for (int x=0; x<length; x++) {
    tmpStr[x] = (char)payload[x]; // payload is a stream, so we need to chop it by length.
  }
  tmpStr[length] = 0x00; // terminate the char string with a null

  if (tmpTopic == "telemetry/furnonoff") {int intFurnOnOff = atoi(tmpStr); gblFurnOnOff = intFurnOnOff; }
  else if (tmpTopic == "telemetry/aconoff") {int intACOnOff = atoi(tmpStr); gblACOnOff = intACOnOff; }
  else if (tmpTopic == "telemetry/thermset") {int intThermSet = atoi(tmpStr); gblThermSet = intThermSet; }

}
#ifdef USE_ETHERNET
  EthernetClient aClient;
  EthernetClient bClient;
#else
  WiFiClient aclient;
  WiFiClient bClient;
#endif // USE_ETHERNET

PubSubClient client(bClient);

void printTelemetry() {
  char sz[32];

  dtostrf(roomTempF, 4, 2, sz);
  client.publish("telemetry/roomtempf",sz);
  dtostrf(roomTempF, 4, 2, sz);
  client.publish("telemetry/roomtempf",sz);
  float va = pzema->voltage(ipa);
  if (va < 0.0) va = 0.0;
  dtostrf(va, 4, 2, sz);
  client.publish("telemetry/l1volts",sz)
  float vb = pzemb->voltage(ipb);
  if (vb < 0.0) vb = 0.0;
  dtostrf(vb, 4, 2, sz);
  client.publish("telemetry/l2volts",sz)
  float ia = pzema->current(ipa);
  if (ia < 0.0) ia = 0.0;
  dtostrf(ia, 4, 2, sz);
  client.publish("telemetry/l1amps",sz)
  float ib = pzemb->current(ipb);
  if (ib < 0.0) ib = 0.0;
  dtostrf(ib, 4, 2, sz);
  client.publish("telemetry/l2amps",sz)
}

long lastReconnectAttempt = 0;
unsigned long lastThermostat = 0UL;

boolean reconnect() {
  if (client.connect("arduinoClient2")) {
    client.subscribe("telemetry/furnonoff");
    client.subscribe("telemetry/aconoff");
    client.subscribe("telemetry/thermset");
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

  while(!Serial1) { }
  pzema = new PZEM004T(&Serial1);
  pzema->setAddress(ipa);
  while(!Serial2) { }
  pzemb = new PZEM004T(&Serial2);
  pzemb->setAddress(ipb);

  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);

  client.setServer(serverb, 1883);
  client.setCallback(callback);

#ifdef USE_ETHERNET
  //Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");

    }
    if (Ethernet.linkStatus() == LinkOFF) {
      //Serial.println("Ethernet cable is not connected.");

    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  }
#else
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
#endif // USE_ETHERNET


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

  long timTimer = millis();
  if (timTimer - lastThermostat >= 1000) {
    lastThermostat = timTimer;
    if ((gblFurnOnOff == 1 || gblACOnOff == 1) && (gblACOnOff != gblFurnOnOff)) {
      if (gblACOnOff == 1) {
        // turn on A/C fan
      }
      if (gblACOnOff == 1 && fltRoomTemp >= (gblThermSet + 5.00)) {
        // turn on A/C
      } else if (gblFurnOnOff == 1 && fltRoomTemp <= (gblThermSet - 5.00)) {
        // turn on furnace
      }
    } else {
      //turn off furnace and A/C
    }
  }

}
