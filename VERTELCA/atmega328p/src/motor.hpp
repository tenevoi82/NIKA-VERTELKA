#ifndef MOTOR_HPP
#define MOTOR_HPP

#include "Arduino.h"
// Пример генерации меандра на таймере 2 , канале B (D3 на Arduino UNO)
#include <GyverTimers.h>


#define PIN_PAS 17 //19 //PC5
#define PIN_DIR 14 //17 //PC3
#define PIN_ENA 12 //14 //PC0
#define PIN_ZERO 11 //12 //PB4

class Motor
{
private:
    float stepTograd(uint32_t);
    uint32_t gradToStep(float);
    volatile uint32_t current_step = 0;
    float current_speed = 1; // скорость гадусов в минуту
public:
    Motor();
    void interruptfunction();
    void run_engine();
    void set_direction(uint32_t);
    void setsoftstop(uint32_t);
    void disable_all();
    void stop_engine();
    void stop_on(uint32_t);
    void set_speed(float);
    ~Motor();
};



#endif /* MOTOR_HPP */