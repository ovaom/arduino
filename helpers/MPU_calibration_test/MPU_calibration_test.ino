#include <EEPROM.h>

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    Serial.println("Starting up calibration TEST");
    delay(1000);
    readEEPROM();
    while(1){}
}

// ====================================== //
// FUNCTIONS
// ====================================== //

void readEEPROM()
{
    uint addr = 0;

    struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
    } data;

    Serial.println("Reading data from Flash memory");

    EEPROM.begin(512);
    EEPROM.get(addr, data);
    Serial.printf("GyX_offset:%d GyY_offset:%d GyZ_offset:%d \n", data.x, data.y, data.z);
    Serial.println("Exiting..");
}