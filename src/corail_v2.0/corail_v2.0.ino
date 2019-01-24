/*
 * OBJECT_3 - version 2.0 (Corail)
 * This version includes LED and battery charge monitor 
*/

#include "src/Ovaom.h"
#include <Adafruit_ADS1015.h>

// OBJECT_3 = Corail
const int OBJECT_UID = 3;
const int maxParams = 3;
float     data[maxParams];

Ovaom ovaom(OBJECT_UID, HW_VERSION_2, maxParams);

// ADC
Adafruit_ADS1015 ads;


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
  Wire.begin(4,5); // begin ads
  ovaom.displayMode = CONNECTED;
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
    getPressure();
    ovaom.getMpuValues();
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

#define ADC_SENSITIVITY 8
int16_t adc[3], prev_adc[3];
void  getPressure() 
{
  ovaom.sensorIsActive = false;
  for(int16_t i = 0; i < 3; i++) 
  {
    adc[i] = ads.readADC_SingleEnded(i);
    // Serial.printf("ADC[%d]: %d\n", i, adc[i]);
    if (adc[i] > 2000) 
    {
      adc[i] = 0;
    }
    if (abs(adc[i] - prev_adc[i]) > ADC_SENSITIVITY) 
    {
      data[i] = ovaom.mapfloat((float)adc[i], 0.0, 1000.0, 0.0, 1.0);
      ovaom.dataLimiter(&data[i], 0.0, 1.0);
      ovaom.sensorDataHasChanged = true;
      prev_adc[i] = adc[i];
    }
    // if (data[i] > 0.7)
    //   ovaom.sensorIsActive = true;
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
    if (ovaom.getObjectState() == ACTIVE)
      ovaom.sendOscMessage("/object/" + String(OBJECT_UID) + "/presetChange");
  }
}
