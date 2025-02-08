#include "Arduino.h"
#include <SettingsGyver.h>
#include <GyverDBFile.h>
#include "data.hpp"


extern RTC_DS3231 rtc;


SettingsGyver sett("NIKA - Вертелка", &db);



void web_update(sets::Updater& u) {
}


void build(sets::Builder &b)
{
    data.unixtime = rtc.now().unixtime();
    b.DateTime("Время на устр.", &data.unixtime);
    b.LabelFloat("Температура", data.temp, 2, sets::Colors::Blue);
    if (b.Switch(kk::disableall, "Выключить"))
    {
        sett.reload();
    }
    (db[kk::disableall].toBool()) ? analogWrite(12, 0) : analogWrite(12, 1);
    if (!db[kk::disableall].toBool())
    {
        {
            sets::Group g1(b, "Управление");
            b.Switch(kk::rotation, "Вращение против часовой");
            if (b.Slider(kk::speed, "Скорость вращения", 0.1, 2, 0.1, " об/мин"))
            {
                // freq = 1000 * db[kk::speed].toFloat();
                // analogWriteFreq(freq);
            }
            if (b.Switch(kk::enabled, "Расписание вкл/выкл"))
                b.reload(); // перезагрузить вебморду по клику на свитч
        }
        {
            if (db[kk::enabled])
            {
                sets::Group g2(b, "Расписание");
                b.Time(kk::timeOn, "Время включения");
                b.Time(kk::timeOff, "Время выключения");
                b.Slider(kk::angle, "Угол остановки", 0, 360, 0.5, "°");
                b.Switch(kk::softstop, "Плавная остановка");
            }
        }
    }
}