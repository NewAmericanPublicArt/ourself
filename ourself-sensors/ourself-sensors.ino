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

/* Load cell code heavily based on HX711 code written by Bogde: https://github.com/bogde/HX711
 * Redistributed under GPL license in accordance with Bogde's license (GPL)
 * at https://github.com/bogde/HX711/blob/master/LICENSE */

#include <DmxSimple.h>

#define TRUE  1
#define FALSE 0

#define USONIC_THRESHOLD     100
#define THE_ONLY_BAR_WE_HAVE 1

#define DMX_OUT 6

#define PIR_R_OUT  9
#define USONIC_OUT 10
#define PIR_L_OUT  11

#define PIR_L_IN  14
#define USONIC_IN A4
#define PIR_R_IN  22

#define STATE_BASELINE 0
#define STATE_APPROACH 1
#define STATE_STORY    2
#define STATE_LEAVING  3

#define APPROACH_TIMEOUT 30000
#define LEAVING_TIMEOUT  30000

#define LOAD_THRESHOLD 300000

// SLOPPY GLOBAL VARS USED FOR LOAD CELL CODE

int PD_SCK  = 22;
int DOUT    = 14;
int GAIN = 2; // from set_gain()

void setLightBar(int address, int r, int g, int b) {
    DmxSimple.write(address, 255);   // master dimmer, 0-255 -> dim-bright
    DmxSimple.write(address + 1, 0); // flash rate: 0 = no flash, 1-255 -> long-short delay between flashes
    DmxSimple.write(address + 2, 0); // whether to run some fucked up program. 0-50=no, other numbers run weird shit
    DmxSimple.write(address + 3, 0); // how fast to fade in weird shit, 0-255 -> slow-fast
    DmxSimple.write(address + 4, r); // red channel, 0-255 -> dim-bright
    DmxSimple.write(address + 5, g); // green channel, 0-255 -> dim-bright
    DmxSimple.write(address + 6, b); // blue channel, 0-255 -> dim-bright
}

int motionDetected(void) {
    if(digitalRead(PIR_L_IN) | digitalRead(PIR_R_IN)) {
        return TRUE;
    } else {
        return TRUE;
        //return FALSE; // BREAKING THIS FOR TESTING
    }
}

bool personBetweenMirrors() {
    long loadCellReading;
    loadCellReading = readLoadCell();
    Serial.println(loadCellReading);
    if(loadCellReading > LOAD_THRESHOLD) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void updateStateMachine(void) {
    static int state = STATE_BASELINE;
    static unsigned long approach_timer;
    static unsigned long leaving_timer;

    Serial.print("STATE: ");
    Serial.print(state);
    Serial.print("\n");

    switch(state) {
        case STATE_BASELINE:
            setLightBar(THE_ONLY_BAR_WE_HAVE, 50, 50, 50); // set edge light to low
            // set ambient sound to very low
            digitalWrite(USONIC_OUT, LOW); // stop any story
            if(motionDetected()) {
                state = STATE_APPROACH;
                approach_timer = millis();
            }
            break;
        case STATE_APPROACH:
            // play alert
            // set ambient sound to high
            // single pulse of edge light, then down to medium
            if(personBetweenMirrors()) {
                state = STATE_STORY;
            }/* else if(approach_timer - millis() > APPROACH_TIMEOUT) {
                state = STATE_BASELINE;
            }*/
            break;
        case STATE_STORY:
            digitalWrite(USONIC_OUT, HIGH); // play start sound, then play next story as long as we're in this state
            // set ambient sound to low
            setLightBar(THE_ONLY_BAR_WE_HAVE, 255, 255, 255); // set edge light to high
            if(!personBetweenMirrors()) {
                if(motionDetected()) {
                    state = STATE_LEAVING;
                    leaving_timer = millis();
                } else {
                    // need a timeout in here?
                    state = STATE_BASELINE; // this is the case where the person vanished without tripping the PIR
                }
            }
            break;
        case STATE_LEAVING:
            // play leaving sound
            // set ambient sound to high
            // set edge light to medium
            // fade story to 0
            if(personBetweenMirrors()) { // ah, they went back in!
                state = STATE_STORY;
            }
            if(leaving_timer - millis() > LEAVING_TIMEOUT) {
                state = STATE_BASELINE;
            }
            break;
    }
}

bool is_ready() {
    return digitalRead(DOUT) == LOW;
}

long readLoadCell() {
    // wait for the chip to become ready
    while (!is_ready());

    unsigned long value = 0;
    byte data[3] = { 0 };
    byte filler = 0x00;

    // pulse the clock pin 24 times to read the data
    data[2] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    data[1] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    data[0] = shiftIn(DOUT, PD_SCK, MSBFIRST);

    // set the channel and the gain factor for the next reading using the clock pin
    for (unsigned int i = 0; i < GAIN; i++) {
        digitalWrite(PD_SCK, HIGH);
        digitalWrite(PD_SCK, LOW);
    }

    // Datasheet indicates the value is returned as a two's complement value
    // Flip all the bits
    data[2] = ~data[2];
    data[1] = ~data[1];
    data[0] = ~data[0];

    // Replicate the most significant bit to pad out a 32-bit signed integer
    if ( data[2] & 0x80 ) {
        filler = 0xFF;
    } else if ((0x7F == data[2]) && (0xFF == data[1]) && (0xFF == data[0])) {
        filler = 0xFF;
    } else {
        filler = 0x00;
    }

    // Construct a 32-bit signed integer
    value = ( static_cast<unsigned long>(filler) << 24
            | static_cast<unsigned long>(data[2]) << 16
            | static_cast<unsigned long>(data[1]) << 8
            | static_cast<unsigned long>(data[0]) );

    // ... and add 1
    return static_cast<long>(++value);
}

void initLoadCell() {
    pinMode(PD_SCK, OUTPUT);
    pinMode(DOUT, INPUT);
    // act as if gain is 32
    digitalWrite(PD_SCK, LOW); // from set_gain()
}

void setup() {
    pinMode(DMX_OUT, OUTPUT);    // send DMX
    pinMode(PIR_R_OUT, OUTPUT);  // announce PIR R signal
    pinMode(USONIC_OUT, OUTPUT); // announce USONIC signal
    pinMode(PIR_L_OUT, OUTPUT);  // announce PIR L signal
//    pinMode(PIR_L_IN, INPUT);    // read PIR L
//    pinMode(PIR_R_IN, INPUT);    // read PIR R
    Serial.begin(115200);
    DmxSimple.usePin(6);
    DmxSimple.maxChannel(100);
    initLoadCell();
}

void loop() {
    updateStateMachine();
}
