#include "Arduino.h"
#include <SettingsGyver.h>
#include <GyverDBFile.h>
#include "data.hpp"
#include <StampKeeper.h>

extern RTC_DS3231 rtc;

SettingsGyver sett("NIKA - Вертелка", &db);

void web_update(sets::Updater &u)
{
}

void build(sets::Builder &b)
{
    b.LabelFloat("Температура", data.temp, 2, sets::Colors::Blue);
    {
        b.beginGroup();
        uint32_t unixtime = rtc.now().unixtime();
        if (b.DateTime("Время", &unixtime))
        {
            DateTime dt(unixtime);
            rtc.adjust(dt);
        }
        // uint32_t unix_time_brouser = sett.rtc.getUnix();
        // b.DateTime("На смартфоне", &unix_time_brouser);
        if (b.Button("Синхронизировать время c браузером"))
        {
            Serial.print("Время браузера: ");
            Serial.println(sett.rtc.now().toString());
            DateTime br_time(sett.rtc.getUnix());
            rtc.adjust(br_time);
            b.reload();
        }
        b.endGroup();
    }
    {
        sets::Group g5(b, "WIFI");
        if(b.Input(kk::wifissid, "SSID")){
            Serial.print("Изменён WiFi SSID на: ");
            Serial.println(db[kk::wifissid].c_str());
        }
        if(b.Pass(kk::wifipass, "Пароль")){
            Serial.print("Изменён WiFi пароль на: ");
            Serial.println(db[kk::wifipass].c_str());
        }
        if (b.Button("Перезагрузить устройство", sets::Colors::Orange))
        {
            if (db.update())
            {
                Serial.println("Изменения в базе зафиксированны, перезагрузка!");
                delay(200);
                ESP.restart();
            }
        }
    }    
    if (b.Switch(kk::disableall, "Выключить всё"))
    {
        if(db[kk::disableall].toBool()){
            Serial.println("Пользователь выключил \"Всё\" =)");
        }
        else{
            Serial.println("Пользователь включил \"Всё\" =)");
        }
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