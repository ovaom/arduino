/*
 * OBJECT_0 - version 2.2 (Grelot)
 * This version includes LED and battery charge monitor
 * This is a new test sending only accelerometer values 
*/

#include "src/Ovaom.h"

#define CALIBRATION_ENABLED false

// OBJECT_0 = grelot a facettes
const int OBJECT_UID = 0;
const int maxParams = 3;
float data[maxParams];

Ovaom ovaom(OBJECT_UID, HW_VERSION_1, maxParams);

/*****************/
/*     SETUP     */
/*****************/
void setup() {
  Serial.begin(115200);
  ovaom.setupLed(16);
  ovaom.updateLed();
  ovaom.connectWifi();
  ovaom.setupMPU(0x68);
  ovaom.setupPresetButton(15);
  ovaom.displayMode = CONNECTED;
}


unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long prevBattMillis = 0;
/***************/
/*    LOOP     */
/***************/
void loop() {
  ovaom.sendPing();
  currentMillis = millis();
  if (currentMillis - previousMillis > SAMPLING_FREQ) {
    previousMillis = currentMillis;
    ovaom.getMpuValues();
    getAccelValues();
    ovaom.checkObjectState();
    ovaom.sendObjectState();
    ovaom.presetButton();
    sendData();
  }
  if (currentMillis - prevBattMillis > BATTERY_INTERVAL) {
    prevBattMillis = currentMillis;
    if (ovaom.batteryLevel() == LOW) {
      ovaom.displayMode = LOW_BATTERY;
    } else {
      ovaom.displayMode = CONNECTED;
    }
  }
  ovaom.updateLed();
}

#define ACCEL_THRSLD 3600
int16_t prevAcX = 0, prevAcY = 0, prevAcZ = 0;
int getAccelValues() {
  if (abs(ovaom.AcX - prevAcX) > ACCEL_THRSLD) {
    data[0] = ovaom.mapfloat((float)ovaom.AcX, -18000.0, 18000.0, 0.0, 1.0);
    ovaom.dataLimiter(&data[0], 0.0, 1.0);
    prevAcX = ovaom.AcX;
    ovaom.sensorDataHasChanged = true;
  }
  if (abs(ovaom.AcY - prevAcY) > ACCEL_THRSLD) {
    data[1] = ovaom.mapfloat((float)ovaom.AcY, -18000.0, 18000.0, 0.0, 1.0);
    ovaom.dataLimiter(&data[1], 0.0, 1.0);
    prevAcY = ovaom.AcY;
    ovaom.sensorDataHasChanged = true;
  }
  if (abs(ovaom.AcZ - prevAcZ) > ACCEL_THRSLD) {
    data[2] = ovaom.mapfloat((float)ovaom.AcZ, -18000.0, 18000.0, 0.0, 1.0);
    ovaom.dataLimiter(&data[2], 0.0, 1.0);;
    prevAcZ = ovaom.AcZ;
    ovaom.sensorDataHasChanged = true;
  }
}

OSCMessage params("/object/0/params");

void sendData() {
  // if (ovaom.getObjectState() == IDLE)
  //   ovaom.sensorDataHasChanged = false;
  if (ovaom.sensorDataHasChanged) {
    ovaom.sensorDataHasChanged = false;
    params.add(data[0]).add(data[1]).add(data[2]);
    ovaom.sendOscMessage(&params);
  }
  if (ovaom.presetButtonChanged && ovaom.getObjectState() == ACTIVE) {
    ovaom.presetButtonChanged = false;
    ovaom.sendOscMessage("/object/" + String(OBJECT_UID) + "/presetChange");
  }
}
