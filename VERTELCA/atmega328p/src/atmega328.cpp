#include "Arduino.h"
// #include <avr/io.h>

#include <SoftwareSerial.h>
#include "net.hpp"
#include "motor.hpp"

SoftwareSerial softSerial(4, 3);
SerialProtocol protocol(softSerial);
Motor motor;

ISR(TIMER2_B)
{
  motor.interruptfunction();
}

bool gotcommand(uint8_t num[], uint8_t len)
{
  uint32_t cmd = *(uint32_t *)(&num[0]);
  uint32_t dt = *(uint32_t *)(&num[4]);
  switch (cmd)
  {
  case 1:
    motor.set_direction(dt);
    return true;
    break;
  case 2:
    motor.set_speed((float)dt / 10);
    return true;
    break;
  case 3:
    motor.stop_on(dt);
    return true;
    break;
  case 4:
    motor.disable_all();
    return true;
    break;
  case 5:
    motor.setsoftstop(dt);
    return true;
    break;
  case 6:
    motor.run_engine();
    return true;
    break;

  default:
    Serial.print(cmd);
    Serial.print(",");
    Serial.println(dt);
    return false;
    break;
  }
}

unsigned long s1 = micros();
unsigned long s2 = micros();
void step(){
  if((micros() - s1) >= 10000)
  {
    PORTC |= (1 << PC3);
    //digitalWrite(PIN_PAS, HIGH);
    s1 = micros();
  }else if(PINC & (1 << PC3) && (micros() - s1) > 5000 )
  {
    PORTC &= ~(1 << PC3);
    //digitalWrite(PIN_PAS, LOW);
  }
}

void setup()
{
  motor.set_speed(1);
  Serial.begin(9600);
  Serial.println("Start");

  protocol.begin(9600); // Инициализируем SoftwareSerial для обмена данными на скорости 9600 бод
  protocol.SetListener(gotcommand);
}

void loop()
{
  protocol.update();
  step();
}