/*
 * OBJECT_2 - version 2.1 (Bolstick)
 * This version includes LED and battery charge monitor 
 * Uses an ADS1015 ADC and BMP180 pressure sensor
*/

#include <Adafruit_ADS1015.h>
#include <Adafruit_BMP085.h>
#include "src/Ovaom.h"

// OBJECT_2 = Bolstick
const int OBJECT_UID = 2;
const int maxParams = 3;
float data[3];

// Analog in 
Adafruit_ADS1015 ads;

// BMP 
Adafruit_BMP085 bmp;
double referencePressure = 0.0;
double prevCalibMillis;

Ovaom ovaom(OBJECT_UID, HW_VERSION_2, maxParams);

/*****************/
/*     SETUP     */
/*****************/
void setup(){
  Serial.begin(115200);
  ovaom.setupLed(16);
  ovaom.updateLed();
  ovaom.connectWifi();
  ovaom.setupMPU(0x68);
  ovaom.setupPresetButton();
  Wire.begin(4,5);
  if (!bmp.begin()) {
	Serial.println("Could not find a valid BMP085 sensor, check wiring!");
	while (1) {}
  }
  pressureAutoCalib();
}

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long prevBattMillis = 0;
/***************/
/*    LOOP     */
/***************/
void loop(){
  ovaom.sendPing();
  currentMillis = millis();
  if (currentMillis - previousMillis > SAMPLING_FREQ) {
    previousMillis = currentMillis;
    readJoystick();
    readBMP180();
    ovaom.getMpuValues();
    ovaom.checkObjectState();
    if (ovaom.getObjectState() == IDLE && millis() - prevCalibMillis > 30000)
      pressureAutoCalib();
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

#define JOYSTICK_THRESHOLD 12
int32_t joystick[2], prevJoystick[2];
void readJoystick() {
  for(int16_t i = 0; i < 2; i++) {
    joystick[i] = ads.readADC_SingleEnded(i);
    if ( abs(joystick[i] - prevJoystick[i]) > JOYSTICK_THRESHOLD ){
      data[i] = ovaom.mapfloat((float)joystick[i], 0.0, 1200.0, 0.0, 1.0);
      ovaom.dataLimiter(&data[i], 0.0, 1.0);
      ovaom.sensorDataHasChanged = true;
      prevJoystick[i] = joystick[i];
    } 
  }
}

#define PRESSURE_THRESHOLD 4.0
#define P_BUFF_LEN 10
double  avgPressure, prev_avgPressure;
int16_t pressureBuff[P_BUFF_LEN];
int buffIdx = 0;


void pressureAutoCalib()
{ 
  int32_t p;

  Serial.print("Autocalibration pressure...");
  for(int i = 0; i < 50; i++)
  {
    p = bmp.readPressure();
    if (p > referencePressure) {
      referencePressure = p;
    }
  }
  Serial.println(referencePressure);
  prevCalibMillis = millis();
}

void readBMP180() 
{
  pressureBuff[buffIdx] = (int16_t)(bmp.readPressure() - referencePressure);
  buffIdx = (buffIdx + 1) % P_BUFF_LEN;
  double avgPressure = ovaom.getRMS(pressureBuff, P_BUFF_LEN);

  if ( abs(avgPressure - prev_avgPressure) > PRESSURE_THRESHOLD) {
    Serial.println(avgPressure);
    data[2] = ovaom.mapfloat((float)avgPressure, 0.0, 300.0, 0.0, 1.0);
    ovaom.dataLimiter(&data[2], 0.0, 1.0);
    ovaom.sensorDataHasChanged = true;
    prev_avgPressure = avgPressure;
  }
}

String paramAddress = "/object/" + String(OBJECT_UID) + "/params";
OSCMessage params(paramAddress.c_str());
void sendData() {
  if (ovaom.getObjectState() == IDLE)
    ovaom.sensorDataHasChanged = false;
  if (ovaom.sensorDataHasChanged) {
      ovaom.sensorDataHasChanged = false;
      params.add(data[0]).add(data[1]).add(data[2]);
      ovaom.sendOscMessage(&params);
    }
    if (ovaom.presetButtonChanged) {
      ovaom.presetButtonChanged = false;
      ovaom.sendOscMessage("/object/" + String(OBJECT_UID) + "/presetChange");
    }
}
