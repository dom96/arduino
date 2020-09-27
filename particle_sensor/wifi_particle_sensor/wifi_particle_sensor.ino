#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "IsThisTheKrustyKrab"
#define STAPSK  "Spong3mock"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

bool hasError = false;
#define STATUS_LED 2

char respBuf[16];
int respBufIndex = 0;

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void SendCmd(const char * cmdBuf, unsigned int cmdSize) {
  //Send command
  unsigned int index = 0;
  for (index = 0; index < cmdSize; index++) {
    Serial.write(cmdBuf[index]);
  }
  Serial.flush();
}

void setup(void) {
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(2, LOW);
  Serial.begin(9600); // Used to write to HPM.
  Serial.swap();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  bool ledON = false;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(2, ledON ? LOW : HIGH);
    ledON = !ledON;
  }

  if (!MDNS.begin("hpm8266")) {
    hasError = true;
  }

  server.on("/", handleRoot);


  server.on("/start", []() {
    const char cmd[] = {0x68, 0x01, 0x01, 0x96};
    SendCmd(cmd, 4);
    server.send(200, "text/plain", "sent data to HPM to start particle measurement");
  });

  server.on("/stop", []() {
    const char cmd[] = {0x68, 0x01, 0x02, 0x95};
    SendCmd(cmd, 4);
    server.send(200, "text/plain", "sent data to HPM to stop particle measurement");
  });

  server.on("/data", []() {
    String hexstring = "";

    for(int i = 0; i < 16; i++) {
      if(respBuf[i] < 0x10) {
        hexstring += '0';
      }
  
      hexstring += String(respBuf[i], HEX);
    }
    server.send(200, "text/plain", hexstring);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  digitalWrite(2, HIGH); // Turn off LED.
}

void loop(void) {
  server.handleClient();
  MDNS.update();

  if (hasError) {
    digitalWrite(2, LOW); //ON
    delay(1000);
    digitalWrite(2, HIGH); // OFF
    delay(1000);
    digitalWrite(2, LOW); //ON
    delay(250);
    digitalWrite(2, HIGH); // OFF
    delay(250);
    digitalWrite(2, LOW); //ON
    delay(250);
    digitalWrite(2, HIGH); // OFF
  }

  if (Serial.available() > 0) {
    char byt = Serial.read();
    respBuf[respBufIndex] = byt;
    digitalWrite(2, LOW); //ON
    delay(20);
    digitalWrite(2, HIGH);
    respBufIndex++;
    if (respBufIndex > 15) {
      respBufIndex = 0;
    }
  }
}
