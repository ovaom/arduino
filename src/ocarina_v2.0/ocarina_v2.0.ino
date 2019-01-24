/*
 * OBJECT_1 - version 2.0 (Ocarina)
 * This version includes LED and battery charge monitor
*/

#include "src/Ovaom.h"
#include "Adafruit_MPR121.h"

// OBJECT_1 = touch sensor
const int OBJECT_UID = 1;
const int maxParams = 4;
uint16_t data[maxParams];

Ovaom ovaom(OBJECT_UID, HW_VERSION_1, maxParams);

// Touch sensing
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t lasttouched = 0;
uint16_t currtouched = 0;


/*****************/
/*     SETUP     */
/*****************/
void setup()
{
  Serial.begin(115200);
  ovaom.setupLed(16);
  ovaom.updateLed();
  ovaom.connectWifi();
  ovaom.setupMPU(0x68);
  ovaom.setupPresetButton(15);
  ovaom.displayMode = CONNECTED;

  // Setup touch sensing
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    delay(2000);
    while (1);
  }
  Serial.println("MPR121 found!");
  setupTouch();
}

void setupTouch()
{
  //_____ Soft reset _____
  cap.writeRegister(MPR121_SOFTRESET, 0x63);
  delay(1);
  for (uint8_t i=0; i<0x7F; i++) {
   Serial.print("$"); Serial.print(i, HEX); 
   Serial.print(": 0x"); Serial.println(cap.readRegister8(i));
  }

  cap.setThreshholds(12, 6);
  cap.writeRegister(MPR121_MHDR, 0x01); //Maximum Half Delta
  cap.writeRegister(MPR121_NHDR, 0x01); //Noise Half Delta
  cap.writeRegister(MPR121_NCLR, 0x0E); //Noise Count Limit 
  cap.writeRegister(MPR121_FDLR, 0x00); //Filter Delay Count Limit

  cap.writeRegister(MPR121_MHDF, 0x01);
  cap.writeRegister(MPR121_NHDF, 0x05);
  cap.writeRegister(MPR121_NCLF, 0x01);
  cap.writeRegister(MPR121_FDLF, 0xFF);

  cap.writeRegister(MPR121_NHDT, 0x00);
  cap.writeRegister(MPR121_NCLT, 0x00);
  cap.writeRegister(MPR121_FDLT, 0x00);

  cap.writeRegister(MPR121_DEBOUNCE, 0);
  cap.writeRegister(MPR121_CONFIG1, 0x10); // default, 16uA charge current
  cap.writeRegister(MPR121_CONFIG2, 0x20); // 0.5uS encoding, 1ms period

  cap.writeRegister(MPR121_ECR, 0x8F); // start
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
    touchSensing();
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

void touchSensing() 
{
  ovaom.sensorIsActive = false;
  // Get the currently touched pads
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<4; i++) 
  {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) 
    {
      // Serial.print(i); Serial.println(" touched");
      data[i] = 1;
      ovaom.sensorDataHasChanged = true;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) 
    {
      // Serial.print(i); Serial.println(" released");
      data[i] = 0;
      ovaom.sensorDataHasChanged = true;
    }
    if (data[i] > 0)
      ovaom.sensorIsActive = true;
  }
  lasttouched = currtouched;
}

String paramAddress = "/object/" + String(OBJECT_UID) + "/params";
OSCMessage params(paramAddress.c_str());

void sendData() {
  if (ovaom.getObjectState() == IDLE)
    ovaom.sensorDataHasChanged = false;
  if (ovaom.sensorDataHasChanged) {
    ovaom.sensorDataHasChanged = false;
    for (int i = 0; i < 4; i++) {
      params.add(data[i]);
    }
    ovaom.sendOscMessage(&params); 
  }
  if (ovaom.presetButtonChanged) {
    ovaom.presetButtonChanged = false;
    if (ovaom.getObjectState() == ACTIVE)
      ovaom.sendOscMessage("/object/" + String(OBJECT_UID) + "/presetChange");
  }
}
