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

bool isAuto = true;

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

void SendCmdRaw(const char * cmdBuf, unsigned int cmdSize) {
  unsigned int index = 0;
  for (index = 0; index < cmdSize; index++) {
    Serial.write(cmdBuf[index]);
  }
  Serial.flush();
}

/* NOTE: this shouldn't be used for commands that expect
 * data as response, this is because it looks for a positive 
 * ACK packet. If one is not received it repeats the command.
 */
void SendCmd(const char * cmdBuf, unsigned int cmdSize, int repeat = 0) {
  // Clear Serial.
  while (Serial.available() > 0) {
    Serial.read();
  }
  
  // Send command
  SendCmdRaw(cmdBuf, cmdSize);

  // Expect a positive ACK.
  char first = Serial.read();
  char second = Serial.read();
  respBuf[respBufIndex] = first;
  respBufIndex++;
  if (respBufIndex > 15) {
    respBufIndex = 0;
  }
  if (first != '\xA5' && second != '\xA5') { // Some lee way here. Both should be A5 really.
    delay(100);
    if (repeat > 5) {
      return;
    }
    SendCmd(cmdBuf, cmdSize, repeat+1);
  }
}

void DisableAutoMode() {
  const char cmd[] = {0x68, 0x01, 0x20, 0x77};
  SendCmd(cmd, 4);
  isAuto = false;
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

  server.on("/read", []() {
    const char cmd[] = {0x68, 0x01, 0x04, 0x93};
    SendCmdRaw(cmd, 4);
    server.send(200, "text/plain", "sent data to HPM to read metrics");
  });

  server.on("/autooff", []() {
    DisableAutoMode();
    server.send(200, "text/plain", "sent data to HPM to stop auto sends");
  });

  server.on("/metrics", []() {
    if (isAuto) {
      // We need to disable auto mode first.
      DisableAutoMode();
      server.send(500, "text/plain", "HPM needs init.");
    }
    
    // TODO: Return prometheus-style metrics for stored PM values.
  });

  server.on("/data", []() {
    String hexstring = "";

    for (int i = 0; i < 16; i++) {
      hexstring += "0x";
  
      hexstring += String(respBuf[i], HEX);
      hexstring += " ";
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
//    respBuf[respBufIndex] = byt;
    if (byt == '\x42') {
      digitalWrite(2, LOW); //ON
      delay(20);
      digitalWrite(2, HIGH);
    }
//    respBufIndex++;
//    if (respBufIndex > 15) {
//      respBufIndex = 0;
//    }
  }
}
