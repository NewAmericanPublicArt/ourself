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

#define LED1 2
#define LED2 3
#define LED3 4
#define LED4 5
#define LED5 6
#define LED6 7

#define PIR1 8
#define PIR2 9

#define AUD1 10
#define AUD2 11
#define AUD3 12

#define PWM1 23
#define PWM2 22

#define CLK1 21
#define DAT1 20
#define CLK2 19
#define DAT2 18
#define CLK3 17
#define DAT3 16
#define CLK4 15
#define DAT4 14

#define STATE_BASELINE 0
#define STATE_APPROACH 1
#define STATE_STORY    2
#define STATE_LEAVING  3

#define APPROACH_TIMEOUT 30000
#define LEAVING_TIMEOUT  30000

#define WEIGHT_THRESHOLD -300000

// constant needed to set gain of load cell amp
// from https://github.com/bogde/HX711/blob/master/HX711.cpp#L22
// Value of 2 means gain of 32.
#define LOAD_CELL_GAIN_EDGES 3

byte clk_table[4] = {CLK1, CLK2, CLK3, CLK4};
byte dat_table[4] = {DAT1, DAT2, DAT3, DAT4};

bool load_cell_one   = false;
bool load_cell_two   = false;
bool load_cell_three = false;
bool load_cell_four  = false;

bool motion_sensor_one = false;
bool motion_sensor_two = false;

void setLights(byte brightness) {
    analogWrite(PWM1, brightness);
    analogWrite(PWM2, brightness);
}

int motionDetected(void) {
    if(motion_sensor_one || motion_sensor_two) {
        return true;
    } else {
        return false;
    }
}

bool personBetweenMirrors() {
    if(load_cell_one || load_cell_two || load_cell_three || load_cell_four) {
        return true;
    }
    return false; // none of the load cell readings were above the threshold
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
            setLights(25); // set edge lights to low
            // by default, ambient sound will be very low
            digitalWrite(AUD1, LOW);
            digitalWrite(AUD2, LOW);
            digitalWrite(AUD3, LOW);
            if(motionDetected()) {
                state = STATE_APPROACH;
                // single pulse of edge lights on state transition, then down to medium
                setLights(150);
                delay(500);
                setLights(100);
                approach_timer = millis();
            }
            break;
        case STATE_APPROACH:
            digitalWrite(AUD1, HIGH);
            digitalWrite(AUD2, LOW);
            digitalWrite(AUD3, LOW);
            if(personBetweenMirrors()) {
                state = STATE_STORY;
            } else if(!motionDetected()) {
                state = STATE_BASELINE;
            }
            break;
        case STATE_STORY:
            digitalWrite(AUD1, LOW);
            digitalWrite(AUD2, HIGH);
            digitalWrite(AUD3, LOW);
            setLights(255); // set edge light to high
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
            digitalWrite(AUD1, LOW);
            digitalWrite(AUD2, LOW);
            digitalWrite(AUD3, HIGH);
            setLights(100); // set edge lights to medium
            if(personBetweenMirrors()) { // ah, they went back in!
                state = STATE_STORY;
            }
            if(!motionDetected()) { // may also want a timeout in here
                state = STATE_BASELINE;
            }
            break;
    }
}

bool is_ready(byte dat_pin) {
    // need to add timeout here
    return digitalRead(dat_pin) == LOW;
}

long readLoadCell(byte cell) {
    byte gain_edges = LOAD_CELL_GAIN_EDGES;
    byte clk = clk_table[cell];
    byte dat = dat_table[cell];

    // wait for the chip to become ready
    while (!is_ready(dat));

    unsigned long value = 0;
    byte data[3] = { 0 };
    byte filler = 0x00;

    // pulse the clock pin 24 times to read the data
    data[2] = shiftIn(dat, clk, MSBFIRST);
    data[1] = shiftIn(dat, clk, MSBFIRST);
    data[0] = shiftIn(dat, clk, MSBFIRST);

    // set the channel and the gain factor for the next reading using the clock pin
    for (unsigned int i = 0; i < gain_edges; i++) {
        digitalWrite(clk, HIGH);
        digitalWrite(clk, LOW);
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

bool checkLoadCellAgainstThreshold(byte cell) {
    long loadCellReading;

    loadCellReading = readLoadCell(cell);
    Serial.print("Load cell ");
    Serial.print(cell);
    Serial.print(": ");
    Serial.print(loadCellReading);
    Serial.print("\n");
    if(loadCellReading < WEIGHT_THRESHOLD) {
        return true;
    } else {
        return false;
    }
}

void checkSensors() {
    load_cell_one   = checkLoadCellAgainstThreshold(0);
    load_cell_two   = checkLoadCellAgainstThreshold(1);
    load_cell_three = checkLoadCellAgainstThreshold(2);
    load_cell_four  = checkLoadCellAgainstThreshold(3);

    motion_sensor_one = digitalRead(PIR1);
    motion_sensor_two = digitalRead(PIR2);
}

void updateLEDs() {
    if(load_cell_one == true) {
        digitalWrite(LED1, HIGH);
    } else {
        digitalWrite(LED1, LOW);
    }
    if(load_cell_two == true) {
        digitalWrite(LED2, HIGH);
    } else {
        digitalWrite(LED2, LOW);
    }
    if(load_cell_three == true) {
        digitalWrite(LED3, HIGH);
    } else {
        digitalWrite(LED3, LOW);
    }
    if(load_cell_four == true) {
        digitalWrite(LED4, HIGH);
    } else {
        digitalWrite(LED4, LOW);
    }
    if(motion_sensor_one == true) {
        digitalWrite(LED5, HIGH);
    } else {
        digitalWrite(LED5, LOW);
    }
    if(motion_sensor_two == true) {
        digitalWrite(LED6, HIGH);
    } else {
        digitalWrite(LED6, LOW);
    }
}

void initAudio() {
    pinMode(AUD1, OUTPUT);
    pinMode(AUD2, OUTPUT);
    pinMode(AUD3, OUTPUT);
}

void initLEDs() {
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
    pinMode(LED5, OUTPUT);
    pinMode(LED6, OUTPUT);
}

void initLights() {
    pinMode(PWM1, OUTPUT);
    pinMode(PWM2, OUTPUT);
}

void initSensors() {
    pinMode(PIR1, INPUT);
    pinMode(PIR2, INPUT);

    pinMode(CLK1, OUTPUT);
    pinMode(DAT1, INPUT);
    pinMode(CLK2, OUTPUT);
    pinMode(DAT2, INPUT);
    pinMode(CLK3, OUTPUT);
    pinMode(DAT3, INPUT);
    pinMode(CLK4, OUTPUT);
    pinMode(DAT4, INPUT);
    // act as if gain is 32
    digitalWrite(CLK1, LOW); // from set_gain()
    digitalWrite(CLK2, LOW);
    digitalWrite(CLK3, LOW);
    digitalWrite(CLK4, LOW);
}

void setup() {
    Serial.begin(115200);
    initAudio();
    initLEDs();
    initLights();
    initSensors();
}

void loop() {
    updateStateMachine();
    checkSensors();
    updateLEDs();
}
