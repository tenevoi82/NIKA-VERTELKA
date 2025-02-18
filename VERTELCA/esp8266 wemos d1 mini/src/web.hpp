#include "Arduino.h"
#include <SettingsGyver.h>
#include <GyverDBFile.h>
#include "data.hpp"

extern RTC_DS3231 rtc;

SettingsGyver sett("NIKA - Вертелка", &db);

void web_update(sets::Updater &u)
{
}

void build(sets::Builder &b)
{
    data.unixtime = rtc.now().unixtime();
    if (b.DateTime("Время на устр.", &data.unixtime))
    {
        DateTime dt(data.unixtime);
        rtc.adjust(dt);
    }
    b.LabelFloat("Температура", data.temp, 2, sets::Colors::Blue);
    if (b.Switch(kk::disableall, "Выключить"))
    {
        sett.reload();
    }
    if (!db[kk::disableall].toBool())
    {
        {
            sets::Group g1(b, "Управление");
            if (b.Switch(kk::rotation, "Вращение против часовой"))
            {
                if (db[kk::rotation].toBool() == true)
                {
                    Serial.println("Включено вращение против часовой стрелки");
                }
                else
                {
                    Serial.println("Выключено вращение против часовой стрелки");
                }
            }
            if (b.Slider(kk::speed, "Скорость вращения", 0.1, 2, 0.1, " об/мин"))
            {
                Serial.print("Установленно скорость вращения ");
                Serial.print(db[kk::speed].toFloat());
                Serial.println(" об/мин");
                // freq = 1000 * db[kk::speed].toFloat();
                // analogWriteFreq(freq);
            }
            if (b.Switch(kk::enabled, "Расписание вкл/выкл"))
            {
                b.reload(); // перезагрузить вебморду по клику на свитч
                if (db[kk::enabled])
                {
                    Serial.println("Рассписание включено");
                }
                else
                {
                    Serial.println("Расписание выключено");
                }
            }
        }
        {
            if (db[kk::enabled])
            {
                sets::Group g2(b, "Расписание");
                if (b.Time(kk::timeOn, "Время включения"))
                {
                    Serial.print("Установленно время включения: ");
                    Serial.println(db[kk::timeOn]);
                }
                if (b.Time(kk::timeOff, "Время выключения"))
                {
                    Serial.print("Установленно время выключения: ");
                    Serial.println(db[kk::timeOff]);
                }
                if (b.Slider(kk::angle, "Угол остановки", 0, 359, 1, "°"))
                {
                    Serial.print("Установленн угол остановки в ");
                    Serial.print(db[kk::angle]);
                    Serial.println(" градусов");
                }
                if (b.Switch(kk::softstop, "Плавная остановка"))
                {
                    if (db[kk::softstop].toBool())
                    {
                        Serial.println("Плавная остановка включена");
                    }
                    else
                    {
                        Serial.println("Плавная остановка выключена");
                    }
                }
            }
        }
    }
}