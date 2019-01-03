#include <EEPROM.h>
#include <Wire.h>


#define CALIBRATION_TIME 20000
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

#define BUFF_SIZE 200
float avg_gx[BUFF_SIZE];
float avg_gy[BUFF_SIZE];
float avg_gz[BUFF_SIZE];

struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} gyro_offset;

int timestep = 0;
unsigned long current_millis;
unsigned long prev_millis;

// ====================================== //
// SETUP
// ====================================== //
void setup()
{
    Serial.begin(115200);
    setupMPU();
}

// ====================================== //
// LOOP
// ====================================== //
void loop()
{
    Serial.println("Warming up, calibration will start in 5secs ...");
    delay(5000);
    Serial.println("Starting, DON'T MOVE !!! ");
    
    unsigned long start = millis();
    while (millis() - start < CALIBRATION_TIME)
    {
        wdt_reset(); // reset watchdog timer
        current_millis = millis();
        if (prev_millis - current_millis > 15) {
            getMpuValues();

            if (GyX > 0) gyro_offset.x--; else if (GyX < 0) gyro_offset.x++;
            if (GyY > 0) gyro_offset.y--; else if (GyY < 0) gyro_offset.y++;
            if (GyZ > 0) gyro_offset.z--; else if (GyZ < 0) gyro_offset.z++;
        }
    }
    Serial.println("Finished calibrating");
    Serial.printf("GyX_offset: %d \t GyY_offset: %d \t GyZ_offset: %d\n", gyro_offset.x, gyro_offset.y, gyro_offset.z);
    delay(3000);
    writeEEPROM();

    while(1){}
}

// ====================================== //
// FUNCTIONS
// ====================================== //
void setupMPU()
{
    Wire.begin();
    Wire.beginTransmission(0x68);
    Wire.write(0x6B); // PWR_MGMT_1 register
    Wire.write(0);    // set to zero (wakes up the MPU-6050)
    Wire.endTransmission(true);
}

void getMpuValues()
{
    Wire.beginTransmission(0x68);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 14, true);     // request a total of 14 registers
    AcX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    AcY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AcZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    Tmp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    GyX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    GyY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    GyZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
    GyX += gyro_offset.x;
    GyY += gyro_offset.y;
    GyZ += gyro_offset.z;
    Serial.printf("x=%d\t\ty=%d\t\tz=%d\n", GyX, GyY, GyZ);
}


void writeEEPROM()
{
    uint addr = 0;

    Serial.println("Writing data to Flash memory");

    // commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
    // this step actually loads the content (512 bytes) of flash into
    // a 512-byte-array cache in RAM
    EEPROM.begin(512);

    // replace values in byte-array cache with modified data
    // no changes made to flash, all in local byte-array cache
    EEPROM.put(addr, gyro_offset);

    // actually write the content of byte-array cache to
    // hardware flash.  flash write occurs if and only if one or more byte
    // in byte-array cache has been changed, but if so, ALL 512 bytes are
    // written to flash
    EEPROM.commit();

    Serial.println("Finished! Exiting..");
}