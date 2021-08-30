#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>

#include "DHT.h"

#include "parser.h"

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

unsigned char respBuf[128];
int respBufIndex = 0;
int packetReadSize = -1; // -1 when haven't seen 0x40

unsigned int p1, p2_5, p4, p10 = 0;
unsigned int parseFails = 0;
Ticker dataReadTimer;

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
bool needsDhtRead = false;
unsigned int dhtFails = 0;
float currentHumidity = NAN;
float currentTemperature = NAN;
float currentHeatIndex = NAN;

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
  if (first != '\xA5' && second != '\xA5') {
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

void SendReadRequest() {
  const char cmd[] = {0x68, 0x01, 0x04, 0x93};
  SendCmdRaw(cmd, 4);
}

void ParseReadResult() {
  int startIndex = respBufIndex-16;
  if (startIndex < 0) {
    return;
  }
  if (!parseReadAck(&respBuf[startIndex], 16, &p1, &p2_5, &p4, &p10)) {
    parseFails++;
    return;
  }
}

void setup(void) {
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(2, LOW);
  Serial.begin(9600); // Used to write to HPM.
  Serial.swap();

  dataReadTimer.attach(10.0, []() {
    SendReadRequest();
    needsDhtRead = true;
  });
  
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
    SendReadRequest();
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
      return;
    }
    // TODO: Compare to https://aqicn.org/city/london/
    // Also good https://en.wikipedia.org/wiki/Particulates#European_Union
    String result = "";
    result += "# HELP hpm_pollution_pm1 Particulate matter with a diameter of 1.0μm or less (μg per cubic metre).\n";
    result += "# TYPE hpm_pollution_pm1 gauge\n";
    result += "hpm_pollution_pm1 ";
    result += p1;
    result += "\n# HELP hpm_pollution_pm25 Particulate matter with a diameter of 2.5μm or less (μg per cubic metre).\n";
    result += "# TYPE hpm_pollution_pm25 gauge\n";
    result += "hpm_pollution_pm25 ";
    result += p2_5;
    result += "\n# HELP hpm_pollution_pm4 Particulate matter with a diameter of 4μm or less (μg per cubic metre).\n";
    result += "# TYPE hpm_pollution_pm4 gauge\n";
    result += "hpm_pollution_pm4 ";
    result += p4;
    result += "\n# HELP hpm_pollution_pm10 Particulate matter with a diameter of 10μm or less (μg per cubic metre).\n";
    result += "# TYPE hpm_pollution_pm10 gauge\n";
    result += "hpm_pollution_pm10 ";
    result += p10;
    result += "\n# HELP hpm_parse_failures Count of times packet from HPM failed to parse.\n";
    result += "# TYPE hpm_parse_failures counter\n";
    result += "hpm_parse_failures ";
    result += parseFails;
    if (!isnan(currentTemperature)) {
      result += "\n# HELP hpm_temperature Current room temperature in Celsius.\n";
      result += "# TYPE hpm_temperature gauge\n";
      result += "hpm_temperature ";
      result += currentTemperature;
    }
    if (!isnan(currentHumidity)) {
      result += "\n# HELP hpm_humidity Current room humidity in percent.\n";
      result += "# TYPE hpm_humidity gauge\n";
      result += "hpm_humidity ";
      result += currentHumidity;
    }
    if (!isnan(currentHeatIndex)) {
      result += "\n# HELP hpm_heat_index Current room heat index in Celsius.\n";
      result += "# TYPE hpm_heat_index gauge\n";
      result += "hpm_heat_index ";
      result += currentHeatIndex;
    }
    server.send(200, "text/plain", result);
  });

  server.on("/data", []() {
    String hexstring = "";

    for (int i = 0; i < 128; i++) {
      hexstring += "0x";
  
      hexstring += String(respBuf[i], HEX);
      hexstring += " ";
    }
    server.send(200, "text/plain", hexstring);
  });

  server.onNotFound(handleNotFound);

  server.begin();

  dht.begin();

  DisableAutoMode();
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
    
    if (byt == '\x40') {
      digitalWrite(2, LOW); //ON
      packetReadSize = 1;
      delay(100);
      digitalWrite(2, HIGH);
    }

    if (packetReadSize != -1) {
      respBuf[respBufIndex] = byt;
      respBufIndex++;
      if (respBufIndex > 127) {
        respBufIndex = 0;
      }
      packetReadSize++;
    }

    if (packetReadSize > 16) {
      packetReadSize = -1;
      ParseReadResult();
    }
  }

  if (needsDhtRead) {
    currentHumidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    currentTemperature = dht.readTemperature();
    if (isnan(currentHumidity) || isnan(currentTemperature)) {
      dhtFails++;
    } else {
      // Compute heat index in Celsius (isFahreheit = false)
      currentHeatIndex = dht.computeHeatIndex(currentTemperature, currentHumidity, false);
    }
  }
}
