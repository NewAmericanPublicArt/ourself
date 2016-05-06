/*  Arduino sketch to run Ourself logging subsystem
    for installation in Camden, NJ, USA
    Copyright 2016 New American Public Art
    Written by Brandon Stafford, brandon@rascalmicro.com */

/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message

#define MOTION1   14
#define LOADCELLS 15
#define MOTION2   16

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
  }
  return pctime;
}

/* The circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 */

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// Teensy audio board: pin 10
// Wiz820+SD board: pin 4
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20
const int chipSelect = 4;

void setup()
{
    Serial.begin(115200);
    while (!Serial);  // Wait for Arduino Serial Monitor to open

    // Set up SD card
    Serial.print("Initializing SD card...");
    if (!SD.begin(chipSelect)) {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        return;
    }
    Serial.println("card initialized.");

    // Set up real-time clock
    setSyncProvider(getTeensy3Time);
    delay(100);
    if (timeStatus()!= timeSet) {
        Serial.println("Unable to sync with the RTC");
    } else {
        Serial.println("RTC has set the system time");
    }
    pinMode(MOTION1, INPUT);
    pinMode(LOADCELLS, INPUT);
    pinMode(MOTION2, INPUT);
}

void loop()
{
    // Try to sync the real-time clock
/*  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }*/
    String dataString = "";

    if(digitalRead(MOTION1) || digitalRead(LOADCELLS) || digitalRead(MOTION2)) {
        dataString += String(year());
        dataString += "-";
        if(month() < 10) {
            dataString += "0";
        }
        dataString += String(month());
        dataString += "-";
        if(day() < 10) {
            dataString += "0";
        }
        dataString += String(day());
        dataString += "T";
        if(hour() < 10) {
            dataString += "0";
        }
        dataString += String(hour());
        dataString += ":";
        if(minute() < 10) {
            dataString += "0";
        }
        dataString += String(minute());
        dataString += ":";
        if(second() < 10) {
            dataString += "0";
        }
        dataString += String(second());
        dataString += "Z,";

        if(digitalRead(MOTION1)) {
            dataString += "ARR,";
        } else {
            dataString += ",";
        }

        if(digitalRead(LOADCELLS)) {
            dataString += "OCC,";
        } else {
            dataString += ",";
        }
        if(digitalRead(MOTION2)) {
            dataString += "DEP";
        }

        File dataFile = SD.open("datalog.txt", FILE_WRITE);

        if (dataFile) {
            dataFile.println(dataString);
            dataFile.close();
            Serial.println(dataString);
        }
        else {
            Serial.println("error opening datalog.txt");
        }
        delay(1000); // delay so we can't write more than once per second
        // max entry size is 27 bytes
        // 27 bytes * 15 million seconds per 6 months is 405 MB. We have an 8 GB card.
    }
}
