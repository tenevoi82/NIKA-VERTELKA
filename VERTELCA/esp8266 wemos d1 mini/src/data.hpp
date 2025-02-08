#include "Arduino.h"
#include <RTClib.h>
#include <GyverDBFile.h>

 GyverDBFile db(&LittleFS, "/settings.db");

DB_KEYS(
    kk,
    disableall,
    timeOn,
    timeOff,
    speed,
    angle,
    enabled,
    rotation,
    softstop);


struct Data
{
    DateTime rtcTime;
    uint32_t unixtime;
    float temp;
    bool zeropoint = false;
};

Data data;
