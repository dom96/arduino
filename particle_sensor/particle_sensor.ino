/**
 * @file example.ino
 * @author Felix Galindo
 * @date June 2017
 * @brief Example using HPMA115S0 sensor library on a Feather 32u4
 * @license MIT
 */

#include <Esp.h>
#include <ESP8266WiFi.h>
#include <hpma115S0.h>
#include <SoftwareSerial.h>

//Create an instance of software serial
//#define D2 4 // See 12-E pinout: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
//#define D1 5
SoftwareSerial hpmaSerial(D3, D4, false); // D3 connected to pin 9 on HPM (TX) and D4 to pin 7 (RX). Orange/Black is TX (pin 9).
SoftwareSerial mySerial(D7, D6); // RX, TX
//SoftwareSerial mySerial(D0, D1); // RX, TX

// White/Blue connected to pin 7 (RX) on the HPM is used to send the command. It works fine.
// When white is in D4 and black in D3 the command works, but if I unplug one of them it stops working.

// On the UART to USB thingy. Orange is plugged into where ESP's TX would be plugged in. This is pin 9 on HPM which is TX.
//                            Blue is plugged into where ESP's RX would be plugged in.

//Create an instance of the hpma115S0 library
HPMA115S0 hpma115S0(mySerial);

int iterations = 0;

void setup() {
  Serial.begin(9600);

  mySerial.begin(9600);
  Serial1.begin(9600);
  delay(2000);
  Serial.println("ESP8266 has started up");
  Serial.println("Waiting 1secs to init HPM");
//  hpmaSerial.begin(9600);
  delay(1000);
  Serial.print("Starting HPM on D3/D4 or ");
  Serial.print(D3);
  Serial.print("/");
  Serial.print(D4);
  Serial.println("...");
//  hpma115S0.Init();
//  hpma115S0.StartParticleMeasurement();
}

void SendCmd(const char * cmdBuf, unsigned int cmdSize) {
  //Send command
  Serial.print("PS- Sending cmd: ");
  unsigned int index = 0;
  for (index = 0; index < cmdSize; index++) {
    Serial.print(cmdBuf[index], HEX);
    Serial.print(" ");
    mySerial.write(cmdBuf[index]);
  }
  Serial.println("");
  return;
}

void loop() {
  if (iterations >= 5) {

    return;
  }

  unsigned int pm2_5, pm10;
//  if (hpma115S0.ReadParticleMeasurement(&pm2_5, &pm10)) {
//    Serial.println("PM 2.5: " + String(pm2_5) + " ug/m3" );
//    Serial.println("PM 10: " + String(pm10) + " ug/m3" );
//  }
//

  static unsigned char dataBuf[HPM_READ_PARTICLE_MEASURMENT_LEN_C - 1];
  int len = hpma115S0.ReadCmdResp(dataBuf, sizeof(dataBuf), READ_PARTICLE_MEASURMENT);
  if ((len == (HPM_READ_PARTICLE_MEASURMENT_LEN - 1)) || (len == (HPM_READ_PARTICLE_MEASURMENT_LEN_C - 1))) {

    if (len == (HPM_READ_PARTICLE_MEASURMENT_LEN - 1)) {
      // HPMA115S0 Standard devices
      pm2_5 = dataBuf[0] * 256 + dataBuf[1];
      pm10 = dataBuf[2] * 256 + dataBuf[3];
    } else {
      // HPMA115C0 Compact devices
      pm2_5 = dataBuf[2] * 256 + dataBuf[3];
      pm10 = dataBuf[6] * 256 + dataBuf[7];
    }
     Serial.println("PS- PM 2.5: " + String(pm2_5) + " ug/m3" );
     Serial.println("PS- PM 10: " + String(pm10) + " ug/m3" );
  }


//  Serial.print("Recv: ");
//  int i = 0;
//  while (mySerial.available() || i == 0) {
//    char b = mySerial.read();
//    Serial.print(b, HEX);
//    Serial.print(" ");
//    i = 1;
//  }
//  Serial.println();

  iterations++;
  Serial.print("ITer: ");
  Serial.println(iterations);
  if (iterations == 5) {
    Serial.println("Stopping!");
    const char cmd[] = {0x68, 0x01, 0x02, 0x95};
    SendCmd(cmd, 4);
//    hpma115S0.StopParticleMeasurement();
  }
  delay(1000);
}
