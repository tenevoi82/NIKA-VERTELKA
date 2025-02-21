#include "motor.hpp"

float Motor::stepTograd(uint32_t step)
{
    /*
1 оборот это 60000 шагов, учитывая что в 1 обороте 360градусов то 1 градус это 60000 / 360
то 1 градус это примерно 166,6666 шага
*/
    return step / 166.6666F;
}
uint32_t Motor::gradToStep(float grad)
{
    /*
    1 оборот это 60000 шагов, учитывая что в 1 обороте 360градусов то 1 градус это 60000 / 360
    то 1 градус это примерно 166,6666 шага
    */
    return grad * 166.6666F;
}

Motor::Motor()
{
    pinMode(PIN_PAS, OUTPUT);
    digitalWrite(PIN_PAS, LOW);

    pinMode(PIN_DIR, OUTPUT);
    digitalWrite(PIN_DIR, LOW);

    pinMode(PIN_ENA, OUTPUT);
    digitalWrite(PIN_ENA, LOW);

    pinMode(PIN_ZERO, INPUT_PULLUP);

    Timer2.enableISR(CHANNEL_B);
}
void Motor::interruptfunction()
{
    if (!digitalRead(PIN_PAS))
    {
        //digitalWrite(PIN_PAS, HIGH);
        current_step++;
        if (current_step == 60000)
        {
            current_step = 0;
            Serial.println("КРУГ");
        }
    }
    //else digitalWrite(PIN_PAS, LOW);
        
       
}
void Motor::run_engine()
{
    // TODO запуск вращения
    Serial.println("Запуск вращения");
    digitalWrite(PIN_ENA, LOW);    //включаем двигатель
    Timer2.resume();                //запускаем таймер (частоту)
}
void Motor::set_direction(uint32_t dir)
{
    if (dir == 1)
    {
        Serial.println("Устанавливаем вращение по часовой");
        digitalWrite(PIN_DIR, LOW);
    }
    else if (dir == 2)
    {
        Serial.println("Устанавливаем вращение против часовой");
        digitalWrite(PIN_DIR, HIGH);
    }
}
void Motor::setsoftstop(uint32_t val)
{
    // TODO плавный стоп
    if (val == 0)
    {
        Serial.println("Выключаем установку плавный стоп");
    }
    else if (val == 1)
    {
        Serial.println("Включаем установку плавный стоп");
    }
}
void Motor::disable_all()
{
    Serial.println("Отключаем двигатель и тормоза");
    Timer2.pause();                 //выключаем частоту
    digitalWrite(PIN_PAS,LOW);
    digitalWrite(PIN_ENA,HIGH);      //отключаем enable чтобы расслабить двигатель
}
void Motor::stop_engine()
{
    Serial.println("Отключаем двигатель");
    digitalWrite(PIN_PAS,LOW);
    Timer2.pause();                 //выключаем частоту
}
void Motor::stop_on(uint32_t gr)
{
    // TODO стоп двигателя при достижении нужного положения вала в градусах
    Serial.print("Остановлюсь как только достигну ");
    Serial.print(gr);
    Serial.println(" градусов");
}
void Motor::set_speed(float ob_min)
{
    Serial.print("Установка скорости ");
    Serial.print(ob_min, 2);
    Serial.println(" оборотов в минуту.");
    int f = 100 * ob_min;
    Timer2.setFrequency(f * 2);
}
Motor::~Motor()
{
}

