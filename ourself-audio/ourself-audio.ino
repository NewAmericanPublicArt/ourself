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


void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  pinMode(1, INPUT);   // sensor on pin 1
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
  mixer2.gain(0, 0.5);
  mixer2.gain(1, 0.5);
  delay(1000);
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing 1");
    playSdWav1.play("jbond.wav");
    delay(10); // wait for library to parse WAV info
  }
/*  if (playSdWav2.isPlaying() == false) {
    Serial.println("Start playing 2");
    playSdWav2.play("rocket.wav");
    delay(10); // wait for library to parse WAV info
  } */

  float loud = 0.8;
  float quiet = 1.0 - loud;

  if(digitalRead(1)) { // if a person is detected . . .
    mixer1.gain(0, quiet);
    mixer1.gain(2, loud);
    mixer2.gain(0, quiet);
    mixer2.gain(2, loud);
    if (playSdWav3.isPlaying() == false) {
      Serial.println("Start playing 3");
      playSdWav3.play("law.wav");
      delay(10); // wait for library to parse WAV info
    }
  } else {
    if (playSdWav3.isPlaying() == false) {
      mixer1.gain(0, loud);
      mixer1.gain(2, quiet);
      mixer2.gain(0, loud);
      mixer2.gain(2, quiet);
    }
  }
}
