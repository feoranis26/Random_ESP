#include <QuickPID.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_ADS1X15.h>
#include <Adafruit_I2CDevice.h>

#include "AnalogReader.h"

TwoWire i2c(1);

Adafruit_ADS1115 adc;

//AnalogReader currentReader(&adc, 0, 0.1);
AnalogReader voltageReader(&adc, 0, 0.1);

float vPID_in;
float vPID_out;
float vPID_setp;
QuickPID vPID(&vPID_in, &vPID_out, &vPID_setp, 1, 1, 0, QuickPID::Action::direct);

float cPID_in;
float cPID_out;
float cPID_setp;
//QuickPID cPID(&cPID_in, &cPID_out, &cPID_setp, 100, 0, 100, QuickPID::Action::direct);

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(10);

    ledcAttachPin(27, 0);
    ledcSetup(0, 65536, 10);
    ledcWrite(0, 0);

    i2c.begin(21, 22);

    bool adc_ok = adc.begin(72, &i2c);
    if (adc_ok)
        Serial.println("ADC OK");
    else
        Serial.println("ADC Error!!!");

    adc.setGain(GAIN_TWO);

    //currentReader.Init();
    voltageReader.Init();

    vPID.SetOutputLimits(0, 256);
    vPID.SetMode(QuickPID::Control::automatic);

    //cPID.SetOutputLimits(-8192, 256);
    //cPID.SetMode(QuickPID::Control::automatic);
}

long print_ms = 0;

double currentDutyCycle = 0;

double targetVoltage = 5;
double targetCurrent = 0;

bool constantCurrent = false;
long last_us = 0;

void loop() {
    //double current_voltage = currentReader.GetValue();

    double current = 0;// (current_voltage - 1.6625) / (0.185 / 2);
    double voltage = voltageReader.GetValue() * 11;

    if (!constantCurrent) {
        vPID_in = voltage;
        vPID_setp = targetVoltage;

        vPID.Compute();
    }
    else {
        /*cPID_in = current;
        cPID_setp = targetCurrent;

        cPID.Compute();*/
    }

    double new_us = micros();
    double dT = ((double)(new_us - last_us)) / 1000000.0;
    last_us = new_us;

    //if (((vPID_out - currentDutyCycle) / dT) > 4096)
    //    currentDutyCycle += copysign(4096 * dT, vPID_out - currentDutyCycle);
    //else

    if (constantCurrent)
        currentDutyCycle = cPID_out;// *dT;
    else
        currentDutyCycle = vPID_out;// *dT;


    currentDutyCycle = min(max(currentDutyCycle, 0.0), 8192.0);

    if (Serial.available()) {
        String read = Serial.readStringUntil(';');

        if (read == "voltage")
            constantCurrent = false;
        else if (read == "current")
            constantCurrent = true;
        else {
            double in = atof(read.c_str());
            targetCurrent = constantCurrent ? in : 0;
            targetVoltage = !constantCurrent ? in : 0;
        }
    }

    if (millis() - print_ms > 200) {
        Serial.println("Current\t: " + String(current, 5));
        Serial.println("Voltage\t: " + String(voltage, 5));
        Serial.println("Target volts\t: " + String(targetVoltage, 5));
        Serial.println("Target amps\t: " + String(targetCurrent, 5));
        Serial.println("Duty\t\t: " + String(currentDutyCycle));
        Serial.println("Mode\t\t: " + String(constantCurrent ? "CC" : "CV"));

        print_ms = millis();
    }


    ledcWrite(0, currentDutyCycle / 8.0);
    delay(1);
}
