# Ovaom Library Changelog

##[Unreleased]

## Added 

- getAvg() and getRMS() function


##[2.1.1] 2018-12-28

###Changed
- Load calibration offsets from object FLASH memory
- Apply calibration offsets to gyroscope
- Change thresholds macros accordingly

##[2.1.0] 2018-12-27
###Added
- Created this changelog
- Send ping to server
- Added `dataLimiter()` to truncate data readings
- Objects will Serial.print() their Library version on startup

###Changed
- SendState() :  objects send dummy parameters on the "/params" address when object is ACTIVE
- Send preset button info last, after "/params"
- Motion detection of objets using gyroscope and double gate threshold
- Stop sending sensor data when object is IDLE
- Motionless object remains ACTIVE if sensor data change is detected

##[2.0.0] 2018-11-01
### Added
- Status LED
- Battery monitoring
- Support for different PCB versions

##[1.0.0] 2018-07-01
- First library version, no LED and no battery monitoring 