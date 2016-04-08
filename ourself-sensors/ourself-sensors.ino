/*  Arduino sketch to run Ourself sensor subsystem
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

#include <DmxSimple.h>

#define USONIC_THRESHOLD     100
#define THE_ONLY_BAR_WE_HAVE 1

#define DMX_OUT 6

#define PIR_R_OUT  9
#define USONIC_OUT 10
#define PIR_L_OUT  11

#define PIR_L_IN  14
#define USONIC_IN A4
#define PIR_R_IN  22

void setup() {
    pinMode(DMX_OUT, OUTPUT);    // send DMX
    pinMode(PIR_R_OUT, OUTPUT);  // announce PIR R signal
    pinMode(USONIC_OUT, OUTPUT); // announce USONIC signal
    pinMode(PIR_L_OUT, OUTPUT);  // announce PIR L signal
    pinMode(PIR_L_IN, INPUT);    // read PIR L
    pinMode(PIR_R_IN, INPUT);    // read PIR R
    Serial.begin(115200);
    DmxSimple.usePin(6);
    DmxSimple.maxChannel(100);
}

void setLightBar(int address, int r, int g, int b) {
    DmxSimple.write(address, 255);   // master dimmer, 0-255 -> dim-bright
    DmxSimple.write(address + 1, 0); // flash rate: 0 = no flash, 1-255 -> long-short delay between flashes
    DmxSimple.write(address + 2, 0); // whether to run some fucked up program. 0-50=no, other numbers run weird shit
    DmxSimple.write(address + 3, 0); // how fast to fade in weird shit, 0-255 -> slow-fast
    DmxSimple.write(address + 4, r); // red channel, 0-255 -> dim-bright
    DmxSimple.write(address + 5, g); // green channel, 0-255 -> dim-bright
    DmxSimple.write(address + 6, b); // blue channel, 0-255 -> dim-bright
}

void loop() {
    int i;

    int sensorValue = analogRead(USONIC_IN);
    if(sensorValue < USONIC_THRESHOLD) {
        digitalWrite(USONIC_OUT, HIGH);
    } else {
        digitalWrite(USONIC_OUT, LOW);
    }
    Serial.println(sensorValue);
    delay(100);
    setLightBar(THE_ONLY_BAR_WE_HAVE, 255 - min(sensorValue, 255), 0, min(sensorValue, 255));
}
