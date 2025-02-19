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
    softstop,
    wifissid,
    wifipass);


struct Data
{
    float temp;
};

Data data;
