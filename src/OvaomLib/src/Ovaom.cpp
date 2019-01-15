#include "Ovaom.h"

Ovaom::Ovaom(int objectUID) : _objectUID(objectUID), _hwVersion(1), _maxParams(6) {}
Ovaom::Ovaom(int objectUID, int hwVersion, int maxParams) 
  : _objectUID(objectUID), _hwVersion(hwVersion), _maxParams(maxParams)
{
    if (_hwVersion == HW_VERSION_1) {
      _ledON = 1;
      _ledOFF = 0;
      _buttonPin = 15;
    } else if (_hwVersion == HW_VERSION_2) {
      _ledON = 0;
      _ledOFF = 1;
      _buttonPin = 13;
  }
}

void Ovaom::connectWifi() 
{
	char ssid[] = "ovaom";          
	char pass[] = "Passovaom"; 
	_outIp = IPAddress(192,168,4,1);

  Serial.println(LIB_VERSION);
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED) 
  {
	  delay(300);
	  Serial.print(".");
    updateLed();
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	Serial.println("Starting UDP");
	_Udp.begin(_localPort);
	Serial.print("Local port: ");
	Serial.println(_localPort);
}

void Ovaom::sendPing() {
  unsigned long currentMillis = millis();
  if (currentMillis - _prevPing > 1000) {
    OSCMessage m("/ping");
    m.add(_objectUID).add(getObjectState());
    sendOscMessage(&m);
    _prevPing = millis();
  }
}

void Ovaom::sendOscMessage(char * addr) {
  OSCMessage msg(addr);
  _Udp.beginPacket(_outIp, _outPort);
  msg.send(_Udp);
  _Udp.endPacket();
  msg.empty();
}

void Ovaom::sendOscMessage(String addr) {
  OSCMessage msg(addr.c_str());
  _Udp.beginPacket(_outIp, _outPort);
  msg.send(_Udp);
  _Udp.endPacket();
  msg.empty();
}

void Ovaom::sendOscMessage(char * addr, int data) {
  OSCMessage msg(addr);
  msg.add(data);
  _Udp.beginPacket(_outIp, _outPort);
  msg.send(_Udp);
  _Udp.endPacket();
  msg.empty();
}

void Ovaom::sendOscMessage(String addr, int data) {
  OSCMessage msg(addr.c_str());
  msg.add(data);
  _Udp.beginPacket(_outIp, _outPort);
  msg.send(_Udp);
  _Udp.endPacket();
  msg.empty();
}

void Ovaom::sendOscMessage(OSCMessage *msg) {
  _Udp.beginPacket(_outIp, _outPort);
  msg->send(_Udp);
  _Udp.endPacket();
  msg->empty();
}

void Ovaom::setupMPU(const int addr) {
  // Start MPU
	_MPU_addr = addr;
	Wire.begin();
	Wire.beginTransmission(_MPU_addr);
	Wire.write(0x6B);  // PWR_MGMT_1 register
	Wire.write(0);     // set to zero (wakes up the MPU-6050)
	Wire.endTransmission(true);
  // Fetch offset data from flash memory
  EEPROM.begin(512);
  EEPROM.get(_flash_addr, _gyro_offset);
  Serial.printf("GyX_offset:%d GyY_offset:%d GyZ_offset:%d \n", _gyro_offset.x, _gyro_offset.y, _gyro_offset.z);

}

void Ovaom::getMpuValues() {
	Wire.beginTransmission(_MPU_addr);
	Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
	Wire.endTransmission(false);
	Wire.requestFrom(_MPU_addr,14,true);  // request a total of 14 registers
	AcX = Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
	AcY = Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
	AcZ = Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
	Tmp = Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
	GyX = Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
	GyY = Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
	GyZ = Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  
  // Apply offsets 
  GyX += _gyro_offset.x;
  GyY += _gyro_offset.y;
  GyZ += _gyro_offset.z;
  // Serial.printf("x=%d\t\ty=%d\t\tz=%d\n", AcX, AcY, AcZ);
}

void Ovaom::checkObjectState() {
  // Lambda function to return max value
  auto getMax = [](int16_t *a, int16_t len) 
  { 
    int max = 0;
    for(int i = 0; i < len; i++)
    {
      if (a[i] > max)
        max = a[i];
    }
    return (max);
  };
  // Lambda function to return average value
  auto getAvg = [](int16_t *a, int16_t len) 
  {
    double sum;

    for(int i = 0; i < len; i++)
    {
      sum += a[i];
    }
    sum = sum / len;
    return (sum);
  };

  int16_t diff[3];
  diff[0] = abs(GyX);
  diff[1] = abs(GyY);
  diff[2] = abs(GyZ);
  int max_XYZ = getMax(diff, 3);

  _mpu_buffer[_mpu_buff_idx] = max_XYZ;
  _mpu_buff_idx = (_mpu_buff_idx + 1) % _mpu_buffer_max;
  double avg = getAvg(_mpu_buffer, _mpu_buffer_max);

  // Serial.printf("%d %d %d %d\n", 0, ACTIVE_THRESHOLD, IDLE_THRESHOLD, (int)avg);
  // Serial.printf("%d %d %d\n", 0, IDLE_THRESHOLD, (int)avg);
  
  switch (_objectState)
  {
    case ACTIVE:
      if (avg > IDLE_THRESHOLD || this->sensorDataHasChanged || this->sensorIsActive)
        _instantObjectState = ACTIVE;
      else if (avg < IDLE_THRESHOLD )
        _instantObjectState = IDLE;
      break;
    
    case IDLE:
      if (avg > ACTIVE_THRESHOLD) 
        _instantObjectState = ACTIVE;
      else if (avg < IDLE_THRESHOLD || avg < ACTIVE_THRESHOLD)
        _instantObjectState = IDLE;
      break;
  
    default:
      break;
  }

  if (_instantObjectState != _prevInstantState)
  {
    _stableStateTime = millis();
  }
  else if (_instantObjectState == _prevInstantState) 
  {
    if (_instantObjectState == IDLE && millis() - _stableStateTime > IDLE_TRIG_TIME) 
    {
      // => Object is IDLE //
      _objectState = IDLE;
      displayMode = DEBUG_IDLE;
      _stableStateTime = millis();
    }
    else if (_instantObjectState == ACTIVE && millis() - _stableStateTime > ACTIVE_TRIG_TIME) 
    {
      // => Object is ACTIVE //
      _objectState = ACTIVE;
      displayMode = DEBUG_ACTIVE;
      _stableStateTime = millis();
    }
  }
  Serial.printf("%u %u\n", _instantObjectState, _objectState);
  _prevInstantState = _instantObjectState;
}

void Ovaom::sendObjectState() {
  if (_objectState != _prevObjectState) {  
    switch (_objectState)
    {
      case (IDLE):
        sendOscMessage("/object/" + String(_objectUID) + "/state", _objectState);
        break;

      case (ACTIVE):
      { 
        sendOscMessage("/object/" + String(_objectUID) + "/state", _objectState);
        String paramAddress = "/object/" + String(_objectUID) + "/params";
        OSCMessage params(paramAddress.c_str());
        for (int i = 0; i < _maxParams; i++) {
          params.add(0);
        }
        sendOscMessage(&params);
        break;
      }
      default:
        break;
    }
    _prevObjectState = _objectState;
  }
}

int Ovaom::getObjectState() {
  return (_objectState);
}

void Ovaom::setupPresetButton() {
  if (_hwVersion == HW_VERSION_1)
    pinMode(_buttonPin, INPUT);
  else if (_hwVersion == HW_VERSION_2)
    pinMode(_buttonPin, INPUT_PULLUP);
}

// Deprecated, used on first prototypes
void Ovaom::setupPresetButton(const int buttonPin) {
	_buttonPin = buttonPin;
  pinMode(buttonPin, INPUT);
}

bool Ovaom::presetButton() {
	_buttonState = _digitalReadPresetButton();
  if (_buttonState != _prevButtonState) {
    if (_buttonState) {
      sensorDataHasChanged = true;
      presetButtonChanged = true;
      // sendOscMessage("/object/" + String(_objectUID) + "/presetChange");
      _prevButtonState = _buttonState;
      return (true);
    }
    _prevButtonState = _buttonState;
  }
  delay(1);
  return (false);
}

int16_t Ovaom::_digitalReadPresetButton() {
  if (_hwVersion == HW_VERSION_1)
    return(digitalRead(_buttonPin));
  else if (_hwVersion == HW_VERSION_2) 
    return(!digitalRead(_buttonPin));
}

void Ovaom::setupLed(const int ledPin) {
  _ledPin = ledPin;
  pinMode(ledPin, OUTPUT);
  displayMode = CONNECTING;
}

void Ovaom::updateLed() {
  unsigned long currentMillis = millis();
  switch (displayMode)
  {
    case CONNECTING:
      if (currentMillis - _prevLedMillis > 250 ) 
      {
        _prevLedMillis = currentMillis;
        if (_ledState == _ledOFF)
          _ledState = _ledON;
        else
          _ledState = _ledOFF;
      }
      break;
    
    case CONNECTED:
      _ledState = _ledON;
      break;
    
    case OFF:
      _ledState = _ledOFF;
      break;
    
    case LOW_BATTERY:
      // if (currentMillis - _prevLedMillis > 50 ) 
      // {
      //   _prevLedMillis = currentMillis;
      //   if (_ledState == _ledOFF)
      //     _ledState = _ledON;
      //   else
      //     _ledState = _ledOFF;
      // }
      break;

    case DEBUG_ACTIVE:
    if (currentMillis - _prevLedMillis > 50 ) 
    {
      _prevLedMillis = currentMillis;
      if (_ledState == _ledOFF)
        _ledState = _ledON;
      else
        _ledState = _ledOFF;
    }
    break;

    case DEBUG_IDLE:
    if (currentMillis - _prevLedMillis > 200 ) 
    {
      _prevLedMillis = currentMillis;
      if (_ledState == _ledOFF)
        _ledState = _ledON;
      else
        _ledState = _ledOFF;
    }
    break;  
    
    default:
      break;
  }
  digitalWrite(_ledPin, _ledState);
}

int Ovaom::batteryLevel() {
 
  // read the battery level from the ESP8266 analog in pin.
  // analog read level is 10 bit 0-1023 (0V-1V).
  // our 1M & 220K voltage divider takes the max
  // lipo value of 4.2V and drops it to 0.758V max.
  // this means our min analog read value should be 580 (3.14V)
  // and the max analog read value should be 774 (4.2V).
  // convert battery level to percent :
  // level = map(level, 580, 774, 0, 100);
  // level = map(level, 657, 858, 0, 100); // 580+64=667 774+64=858
  // level = map(level, 570, 750, 0, 100); // empirical with lab power
  // level = map(level, 650, 750, 0, 100); //empiral with 3.7V = 0%
  // level = map(level, 660, 873, 0, 100); //empiricalv2
  
  int level = analogRead(A0);
  delay(1);
  level += analogRead(A0);
  delay(1);
  level += analogRead(A0);
  level /= 3;
  level = map(level, 690, 780, 0, 100); //empirical v3

  // Serial.print("Battery level: "); Serial.print(level); Serial.println("%");
  sendOscMessage("/battery", level);
  if (level <= 20)
    return (LOW);
  else
    return (HIGH);
}

float Ovaom::mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void  Ovaom::dataLimiter(float *data, float min, float max)
{
  if (*data > max)
    *data = max;
  else if (*data < min)
    *data = min;
}

void  Ovaom::dataLimiter(int16_t *data, int16_t min, int16_t max)
{
  if (*data > max)
    *data = max;
  else if (*data < min)
    *data = min;
}

double Ovaom::getAvg(int16_t *a, int16_t len)
{
  double sum;

  for(int i = 0; i < len; i++)
  {
    sum += a[i];
  }
  sum = sum / len;
  return (sum);
};

double Ovaom::getRMS(int16_t *a, int16_t len) 
{
  double sum;

  for(int i = 0; i < len; i++)
  {
    sum += pow((float)a[i], 2);
  }
  sum = sum / len;
  sum = sqrt(sum);
  return (sum);
};