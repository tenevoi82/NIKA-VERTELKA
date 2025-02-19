#pragma once
#include "Arduino.h"
#include "net.hpp"


class Commands
{
private:
    SerialProtocol &sp;
    uint32_t data[2];

public:
    Commands(SerialProtocol &);
    bool setDirection(uint32_t);
    bool setSpeed(uint32_t);
    bool stopOn(uint32_t);
    bool enableMotor(uint32_t);
};

Commands::Commands(SerialProtocol &sp)
    : sp(sp)
{
}

bool Commands::setDirection( uint32_t dir){
    data[0] = 1;
    data[1] = dir;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}

bool Commands::setSpeed( uint32_t spd){
    data[0] = 2;
    data[1] = spd;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}