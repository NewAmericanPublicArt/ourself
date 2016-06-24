/*  Arduino sketch to run Ourself audio subsystem
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

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define ARRIVAL_TRIGGER       3
#define LOAD_CELL_TRIGGER_PIN 4
#define DEPARTURE_TRIGGER     5

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav3;     //xy=172,354
AudioPlaySdWav           playSdWav1;     //xy=174,184
AudioPlaySdWav           playSdWav2;     //xy=176,279
AudioMixer4              mixer1;         //xy=444,217
AudioMixer4              mixer2;         //xy=451,314
AudioOutputI2S           i2s1;           //xy=686,260
AudioConnection          patchCord1(playSdWav3, 0, mixer1, 2);
AudioConnection          patchCord2(playSdWav3, 1, mixer2, 2);
AudioConnection          patchCord3(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord4(playSdWav1, 1, mixer2, 0);
AudioConnection          patchCord5(playSdWav2, 0, mixer1, 1);
AudioConnection          patchCord6(playSdWav2, 1, mixer2, 1);
AudioConnection          patchCord7(mixer1, 0, i2s1, 0);
AudioConnection          patchCord8(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=245,438
// GUItool: end automatically generated code

const char *filenames[] = {"01.wav", "02.wav", "03.wav", "04.wav", "05.wav", "06.wav", "07.wav", "08.wav", "09.wav", "10.wav",
                      "11.wav", "12.wav", "13.wav", "14.wav", "15.wav", "16.wav", "17.wav", "18.wav", "19.wav", "20.wav",
                      "21.wav", "22.wav", "23.wav", "24.wav", "25.wav", "26.wav", "27.wav", "28.wav", "29.wav", "30.wav",
                      "31.wav"};

void setup() {
    Serial.begin(115200);
    AudioMemory(8);
    sgtl5000_1.enable();
    sgtl5000_1.lineOutLevel(13); // 16 means 2.67 V peak-to-peak output from audioshield
                                 // For max volume, change to 13, which is 3.12 V peak-to-peak.
                                 // More info at https://www.pjrc.com/teensy/gui/?info=AudioControlSGTL5000
    SPI.setMOSI(7);
    SPI.setSCK(14);
    while(!(SD.begin(10))) {
        Serial.println("Unable to access the SD card");
        delay(500);
    }
    pinMode(13, OUTPUT); // LED on pin 13
    pinMode(LOAD_CELL_TRIGGER_PIN, INPUT);   // sensor on pin 1
    mixer1.gain(0, 0.5);
    mixer1.gain(1, 0.5);
    mixer2.gain(0, 0.5);
    mixer2.gain(1, 0.5);
    delay(1000);
}

void loop() {
    static signed int i = -1; // Start at -1 so first increment is 0, which is where the file array starts.

    if (playSdWav1.isPlaying() == false) {
        Serial.println("Start playing background atmosphere");
        playSdWav1.play("atmos.wav");
        delay(10); // wait for library to parse WAV info
    }

    float loud = 1.0;
    float quiet = 0.2;

    if(digitalRead(LOAD_CELL_TRIGGER_PIN)) { // if a person is detected . . .
        mixer1.gain(0, quiet);
        mixer1.gain(2, loud);
        mixer2.gain(0, quiet);
        mixer2.gain(2, loud);
        if(playSdWav3.isPlaying() == false) {
            i++; // one song ended, so move to the next one
            if(i > 30) {
                i = 0; // start over if we reach the end of the list
            }
            Serial.println("Play bell");
            playSdWav2.play("bell.wav");
            delay(2000);
            Serial.println("Start playing interviews");
            Serial.print("Tried to play: ");
            Serial.print(filenames[i]);
            Serial.print("\n");
            playSdWav3.play(filenames[i]);
            delay(10); // wait for library to parse WAV info
        }
    } else {
        if(playSdWav3.isPlaying() == true) {
            Serial.println("Stop interviews");
            playSdWav3.stop();
        }
        mixer1.gain(0, loud);
        mixer1.gain(2, quiet);
        mixer2.gain(0, loud);
        mixer2.gain(2, quiet);
    }
}
