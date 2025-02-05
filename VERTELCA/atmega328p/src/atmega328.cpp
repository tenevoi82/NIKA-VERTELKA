#include "Arduino.h"
//Пример генерации меандра на таймере 2 , канале B (D3 на Arduino UNO)
#include <GyverTimers.h>
#include <SoftwareSerial.h>
#include "net.hpp"
void set_speed(float ob_min);
void stop_engine();
#define PIN_PAS 19
#define PIN_DIR PD2
#define PIN_ENA PD4
#define PIN_ZERO PD5

uint32_t current_step = 0;
float current_speed = 1;  // скорость гадусов в минуту

// Создаём объект SoftwareSerial для ATmega328p на пинах 10 (RX) и 11 (TX)
// Обычно на ATmega328p аппаратный Serial используется для отладки, поэтому здесь программный порт позволяет использовать другой канал для связи
SoftwareSerial softSerial(0, 1);

// Создаём глобальный объект протокола, передавая ему ранее созданный объект softSerial
SerialProtocol protocol(softSerial);



void stop_engine() {
}

void setup() {
  pinMode(PIN_PAS, OUTPUT);
  digitalWrite(PIN_PAS, LOW);

  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, LOW);

  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, LOW);

  pinMode(PIN_ZERO, INPUT_PULLUP);

  Timer2.enableISR(CHANNEL_B);
  set_speed(current_speed);  //отключить когда настрою протокол обмена
  //Serial.begin(9600);
  protocol.begin(9600); // Инициализируем SoftwareSerial для обмена данными на скорости 9600 бод
}

ISR(TIMER2_B) {
  if (!digitalRead(PIN_PAS)) {
    digitalWrite(PIN_PAS, HIGH);
    current_step++;
    if(current_step==60000){
      current_step = 0;
      //Serial.println("КРУГ");
    }

  } else
    digitalWrite(PIN_PAS, LOW);
}

float stepTograd(uint32_t step) {
  /*
  1 оборот это 60000 шагов, учитывая что в 1 обороте 360градусов то 1 градус это 60000 / 360 
  то 1 градус это примерно 166,6666 шага
  */
  return step / 166.6666F;
}

uint32_t gradToStep(float grad) {
  /*
  1 оборот это 60000 шагов, учитывая что в 1 обороте 360градусов то 1 градус это 60000 / 360 
  то 1 градус это примерно 166,6666 шага
  */
  return grad * 166.6666F;
}

//Зададим скорость вращения( оборотов в минуту )
void set_speed(float ob_min) {
  if (ob_min == 0) {  //если выставленная скорость нулевая то отключить двигатель.
    Timer2.stop();
    digitalWrite(PIN_PAS, LOW);
    return;
  }
  int f = 1000 * ob_min;
  Timer2.setFrequency(f * 2);
}

// функция для расчёта crc
uint8_t crc8_bytes(uint8_t *buffer, uint8_t size) {
  byte crc = 0;
  for (byte i = 0; i < size; i++) {
    byte data = buffer[i];
    for (int j = 8; j > 0; j--) {
      crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
      data >>= 1;
    }
  }
  return crc;
};

#pragma pack(push, 1)
struct SerialData{
  uint8_t magic = '$';
  uint16_t Size;
  uint16_t data[6] = {0,0,0,0,0,0};
  uint8_t crc;
};
#pragma pack(pop)




void loop() {

  static unsigned long m = millis();
  if(millis()- m > 3000){
    m=millis();
    //Serial.println("Serial data:");
    SerialData d;
    d.Size = 3;
    d.data[0] = 1;
    d.data[1] = 2;
    d.data [2] = 3;
    d.crc = crc8_bytes((byte*)&d, sizeof(d) - 1);
    //Serial.write((byte*)&d,sizeof(d));
    //Serial.println("\r\nEND");
  }

}