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
    bool softstop(bool);
    bool DisableAll();
    bool Run();
};

Commands::Commands(SerialProtocol &sp)
    : sp(sp)
{
}

bool Commands::setDirection( uint32_t val){
    data[0] = 1;
    data[1] = val;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}

bool Commands::setSpeed( uint32_t val){
    data[0] = 2;
    data[1] = val;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}

bool Commands::stopOn( uint32_t val){
    data[0] = 3;
    data[1] = val;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}

bool Commands::DisableAll(){
    data[0] = 4;
    data[1] = 0;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}

bool Commands::Run(){
    data[0] = 6;
    data[1] = 0;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}


bool Commands::softstop(bool val){
    data[0] = 5;
    data[1] = val?1:0;
    sp.sendPacketNonBlocking((uint8_t*)(&data),sizeof(data),true);
    return true;
}