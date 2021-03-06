# Ovaom Library Changelog

##[Unreleased]

## [2.1.6] 2019-01-24
### Changed
- Fixed: don't send `/presetChange` if object is IDLE
- Bolstick: read BMP in STANDARD mode which is faster

## [2.1.5] 2019-01-24
### Changed
- Don't send `/presetChange` if object is IDLE

## [2.1.4] 2019-01-22
### Changed
- Set wifi output power to minimum to save battery

## [2.1.3] 2019-01-16

### Changed
- Clean checkObjectState(), fix instant detection bug
- Make IDLE and ACTIVE sensor data much more reactive
- Shake objects to wake them up
- Objects go to sleep faster
- Objects don't go to sleep when sensors are being touched

##[2.1.2] 2019-01-05

### Added 
- getAvg() and getRMS() function
- Add LED debug display

### Changed
- Objects go to sleep faster
- Send object state with Ping


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