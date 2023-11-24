/*
 Name:		rs485_test.ino
 Created:	5/12/2023 8:14:26 PM
 Author:	Alp D
*/

// the setup function runs once when you press reset or power the board

#define packet_size 64
String buffer;
void setup() {
    Serial.begin(115200);
    Serial1.begin(2000000, 134217756U, 26, 27);
    Serial.println("ok");
}

// the loop function runs over and over again until power down or reset
void loop() {
    delay(100);
    if (Serial1.available()) {
        Serial.println("incoming");
        while (Serial1.available())
            buffer += (char)Serial1.read();

    }

    while (buffer.length() > packet_size)
    {
        //Serial.println("Buffer: \"" + buffer + "\"");

        int index = 0;
        for (; index < 2 * packet_size; index++)
            if (buffer[index] == ';')
                break;
        
        //Serial.println("Index: " + String(index));
        if (index <= packet_size && index != -1) {
            String split = buffer.substring(0, index);
            Serial.println("Received: " + String(split));
            buffer = buffer.substring(index + 1);
        }
        else if (index > packet_size && index < 2 * packet_size)
        {
            buffer = buffer.substring(index - (packet_size - 1), index + 1);
        }
        else {
            buffer = buffer.substring(packet_size);
        }
    }

}
