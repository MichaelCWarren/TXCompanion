/*
  FrSky telemetry decoder class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210108
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_DECODER_H_
#define _FRSKY_SPORT_DECODER_H_

#include "FrSkySportSensor.h"

#define FRSKY_DECODER_MAX_SENSORS 28

class FrSkySportDecoder
{
public:
  FrSkySportDecoder();
  void begin(
      FrSkySportSensor *sensor1, FrSkySportSensor *sensor2 = NULL, FrSkySportSensor *sensor3 = NULL,
      FrSkySportSensor *sensor4 = NULL, FrSkySportSensor *sensor5 = NULL, FrSkySportSensor *sensor6 = NULL,
      FrSkySportSensor *sensor7 = NULL, FrSkySportSensor *sensor8 = NULL, FrSkySportSensor *sensor9 = NULL,
      FrSkySportSensor *sensor10 = NULL, FrSkySportSensor *sensor11 = NULL, FrSkySportSensor *sensor12 = NULL,
      FrSkySportSensor *sensor13 = NULL, FrSkySportSensor *sensor14 = NULL, FrSkySportSensor *sensor15 = NULL,
      FrSkySportSensor *sensor16 = NULL, FrSkySportSensor *sensor17 = NULL, FrSkySportSensor *sensor18 = NULL,
      FrSkySportSensor *sensor19 = NULL, FrSkySportSensor *sensor20 = NULL, FrSkySportSensor *sensor21 = NULL,
      FrSkySportSensor *sensor22 = NULL, FrSkySportSensor *sensor23 = NULL, FrSkySportSensor *sensor24 = NULL,
      FrSkySportSensor *sensor25 = NULL, FrSkySportSensor *sensor26 = NULL, FrSkySportSensor *sensor27 = NULL,
      FrSkySportSensor *sensor28 = NULL);
  uint16_t decode(uint8_t byte);

private:
  enum State
  {
    START_FRAME = 0,
    SENSOR_ID = 1,
    DATA_FRAME = 2,
    APP_ID_BYTE_1 = 3,
    APP_ID_BYTE_2 = 4,
    DATA_BYTE_1 = 5,
    DATA_BYTE_2 = 6,
    DATA_BYTE_3 = 7,
    DATA_BYTE_4 = 8,
    CRC_BYTE = 9
  };
  FrSkySportSensor *sensors[FRSKY_DECODER_MAX_SENSORS];
  uint8_t sensorCount = 0;
  State state = State::START_FRAME;
  bool hasStuffing = false;
  uint8_t id = 0;
  uint16_t appId = 0;
  uint32_t data = 0;
  uint16_t crc = 0;
  uint16_t processByte(uint8_t byte);
};

#endif // _FRSKY_SPORT_DECODER_H_
