#ifndef OVAOM_H
# define OVAOM_H

#include "Arduino.h"
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <OSCMessage.h>
#include <Wire.h>

// Software Versions
#define LIB_VERSION "Ovaom Lib 2.1.6"
  
// Hardware PCB Versions 
// Differences in hardware wiring of preset button and LED
// VERSION_1 : design by simon hill
// VERSION_2 : design by simon juif
#define HW_VERSION_1 1
#define HW_VERSION_2 2

// Tweak these parameters to ajust sensitivity
#define SAMPLING_FREQ 			15 		// in milliseconds
#define AVERAGING_BUFFER_SIZE	5 	// values are averaged in an int array of this size 
// Detect ACTIVE
#define ACTIVE_THRESHOLD 		15000
#define ACTIVE_TRIG_TIME 		400
// Detect IDLE
#define IDLE_THRESHOLD			90
#define IDLE_TRIG_TIME 			300


// Object State
#define IDLE 0
#define ACTIVE 1

// Led State
#define OFF 0
#define CONNECTING 1
#define CONNECTED 2
#define LOW_BATTERY 3
#define DEBUG_ACTIVE 4
#define DEBUG_IDLE 5

// Battery
#define BATTERY_INTERVAL 10000 // 300000=5mins

class Ovaom
{
public:
	int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
	Ovaom(); // deprecated
	Ovaom(int objectUID);
	Ovaom(int objectUID, int hwVersion, int maxParams);
	
	// Network
	void connectWifi();
	void sendPing();
	void sendOscMessage(char * addr);
	void sendOscMessage(String addr);
	void sendOscMessage(char * addr, int data);
	void sendOscMessage(String addr, int data);
	void sendOscMessage(OSCMessage *msg);
	
	// MPU
	void setupMPU(const int _MPU_addr);
	void getMpuValues();
	void checkObjectState();
	void sendObjectState();
	int  getObjectState();


	// Preset button
	void setupPresetButton();
	void setupPresetButton(int buttonPin); // deprecated
	bool presetButton();
	bool presetButtonChanged = false;

	// LED
	int  displayMode = OFF;
	void setupLed(const int ledPin);
	void setLedState(const int state);
	void updateLed();
	void updateLed(uint16_t hardwareVersion);

	// Battery 
	int  batteryLevel();

	// Helpers
	float  mapfloat(float x, float in_min, float in_max, float out_min, float out_max);
	void   dataLimiter(float *data, float min, float max);
	void   dataLimiter(int16_t *data, int16_t min, int16_t max);
	double getAvg(int16_t *a, int16_t len);
	double getRMS(int16_t *a, int16_t len);

	bool  sensorDataHasChanged = false;
	bool  sensorIsActive = false;

private:
	const int			_objectUID;
	const int           _hwVersion;
	const int			_maxParams;

	// Flash memory
	uint 				_flash_addr = 0;

	// Network
	WiFiUDP 			_Udp;
	IPAddress 			_outIp;
	const uint16_t	 	_outPort = 9001;
	const uint16_t	 	_localPort = 9002;
	unsigned long 		_prevPing = 0;

	// MPU
	int16_t	    		_MPU_addr;
	struct
    {
    	int16_t x;
    	int16_t y;
    	int16_t z;
    } 					_gyro_offset;
	int16_t 			_prevAcX = 0, _prevAcY = 0, _prevAcZ = 0;
	static const int16_t _mpu_buffer_max = AVERAGING_BUFFER_SIZE;
	int16_t				_mpu_buffer[_mpu_buffer_max];
	int16_t				_mpu_buff_idx = 0;
	unsigned long		_lastActive;
	
	// Object state
	bool 				_objectState = 0;
	bool 				_prevObjectState = 0;
	bool				_instantObjectState = 0;
	bool 				_prevInstantState = 0;

	// checkObjectState()
	unsigned long 		_stableStateTime = 0;

	// Preset button
	int16_t     		_buttonPin = 15;
	int16_t 			_buttonState = 0;
	int16_t 			_prevButtonState = 0;
	int16_t     		_digitalReadPresetButton();

	// Led update
	int16_t           	_ledPin = 16;
	unsigned long 		_prevLedMillis = 0;
	int16_t           	_ledState = LOW;
	int16_t			  	_ledON = 1;
	int16_t			 	_ledOFF = 0;

	// Battery
	int16_t				_battery_level;

};

# endif