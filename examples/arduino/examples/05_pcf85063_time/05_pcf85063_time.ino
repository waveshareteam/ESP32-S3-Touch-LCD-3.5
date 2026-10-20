/**
 *
 * @license MIT License
 *
 * Copyright (c) 2023 lewis he
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      PCF85063_SimpleTime.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2023-09-07
 *
 */
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include "SensorPCF85063.hpp"

#ifndef SENSOR_SDA
#define SENSOR_SDA  8
#endif

#ifndef SENSOR_SCL
#define SENSOR_SCL  7
#endif


SensorPCF85063 rtc;
uint32_t lastMillis;

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    Wire.begin(SENSOR_SDA, SENSOR_SCL);
    if (!rtc.begin(Wire)) {
        while (1) {
        Serial.println("Failed to find PCF8563 - check your wiring!");
        delay(1000);
        }
    }
    RTC_DateTime datetime = rtc.getDateTime();

    if (datetime.getYear() < 2025) {
        rtc.setDateTime(2025, 1, 1, 12, 0, 0);
    }

}


void loop()
{
    if (millis() - lastMillis > 1000) {
        lastMillis = millis();
        RTC_DateTime datetime = rtc.getDateTime();
        Serial.printf(" Year :"); Serial.print(datetime.getYear());
        Serial.printf(" Month:"); Serial.print(datetime.getMonth());
        Serial.printf(" Day :"); Serial.print(datetime.getDay());
        Serial.printf(" Hour:"); Serial.print(datetime.getHour());
        Serial.printf(" Minute:"); Serial.print(datetime.getMinute());
        Serial.printf(" Sec :"); Serial.println(datetime.getSecond());

    }
}



