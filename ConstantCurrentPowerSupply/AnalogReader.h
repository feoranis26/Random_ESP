#ifndef AnalogReader_h
#define AnalogReader_h

#define AnalogReaderBufferSize 255

#include "Arduino.h"
#include "Adafruit_ADS1X15.h"

struct AnalogReaderBufferEntry {
public:
    long entry_millis;
    double value;

    AnalogReaderBufferEntry(long ms, double val) {
        entry_millis = ms;
        value = val;
    }
};

class AnalogReader {
public:
    AnalogReader(int pin_number, double RollingAverageSeconds) {
        pin = pin_number;
        average_length = RollingAverageSeconds * 1000;
    }

    AnalogReader(Adafruit_ADS1115* adc, int ch, double RollingAverageSeconds) {
        adc_i2c = adc;
        pin = ch;
        average_length = RollingAverageSeconds * 1000;
    }

    double GetValue() {
        return value;
    }

    void Init() {
        xTaskCreate(task, "adc_task", 8192, this, 0, NULL);
    }

private:
    AnalogReaderBufferEntry* buffer[AnalogReaderBufferSize];
    int buffer_size = 0;

    Adafruit_ADS1115* adc_i2c = nullptr;

    int pin = 0;
    double average_length = 0;

    double value = 0;

    void update() {
        if (buffer_size < AnalogReaderBufferSize)
            read_and_add_to_buffer();

        trim_buffer_and_get_average();
    }

    void read_and_add_to_buffer() {
        double value;// = ((double)analogReadMilliVolts(pin)) / 1000.0;
        long ms = millis();

        if (adc_i2c != nullptr)
            value = adc_i2c->readADC_SingleEnded(pin) * 0.0000625;
        else
            value = ((double)analogReadMilliVolts(pin)) / 1000.0;

        //Serial.println("read value\t: " + String(value));
        //Serial.println("pin\t\t: " + String(pin));
        /*Serial.println("1         : " + String(adc_i2c->computeVolts(adc_i2c->readADC_SingleEnded(0))));
        Serial.println("2         : " + String(adc_i2c->computeVolts(adc_i2c->readADC_SingleEnded(1))));
        Serial.println("3         : " + String(adc_i2c->computeVolts(adc_i2c->readADC_SingleEnded(2))));
        Serial.println("4         : " + String(adc_i2c->computeVolts(adc_i2c->readADC_SingleEnded(3))));*/

        if (abs(value - this->value) > 0.1)
            reset_buffer();

        buffer[buffer_size++] = new AnalogReaderBufferEntry(ms, value);
    }

    void trim_buffer_and_get_average() {
        double new_value = 0;
        double count = 0;

        for (int i = 0; i < buffer_size; i++) {
            AnalogReaderBufferEntry* entry = buffer[i];
            //Serial.print(String(entry->value) + " ,");

            if ((millis() - entry->entry_millis) > average_length) {
                delete entry;
                for (int j = i; j < buffer_size - 1; j++) {
                    buffer[j] = buffer[j + 1];
                }

                buffer_size--;
                continue;
            }

            new_value += entry->value;
            count += 1.0;
        }


        new_value /= count;

        value = new_value;
    }

    void reset_buffer() {
        for (int i = 0; i < buffer_size; i++)
            delete buffer[i];

        buffer_size = 0;
    }

    static bool wait;

    static void task(void* param) {
        AnalogReader* reader = (AnalogReader*)param;

        while (true) {
            while (wait)
                delay(6);

            wait = true;
            delay(2);
            reader->update();
            wait = false;

            delay(4);
        }
    }
};

#endif