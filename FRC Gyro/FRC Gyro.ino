#define _ASYNC_WEBSERVER_LOGLEVEL_      4

#include <ESPmDNS.h>
#include <AsyncUDP.h>
#include <HTTP_Method.h>
#include <Uri.h>
#include <WebServer.h>
#include <AsyncTCP.h>
#include <ETH.h>
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <AsyncWebServer_WT32_ETH01.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <MPU9250_WE.h>
#include <Wire.h>
#include <WiFi.h>

TwoWire w(0);
MPU9250_WE imu(&w, 0x68);

double angle_offset = 0;
double tilt = 0;

xyzFloat acc;
xyzFloat spd;
xyzFloat pos;

double x_pos;
double y_pos;
double z_pos;

xyzFloat avg_gyro;
xyzFloat avg_accel;

AsyncUDP udp;
IPAddress remote(10, 8, 1, 1);

long last_pkt = -1000;

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    WT32_ETH01_onEvent();
    ETH.begin(1, 16);
    //ETH.config(IPAddress(10, 134, 102, 228), IPAddress(10, 134, 103, 254), IPAddress(255, 255, 255, 0), IPAddress(8, 8, 8, 8));
    //WT32_ETH01_waitForConnect();

    // Static IP, leave without this line to get IP via DHCP
    //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
    Serial.println("state:\tstartup ;");
    //Serial.println("Startup");
    delay(50);

    ledcSetup(0, 1000, 8);
    ledcAttachPin(33, 0);
    ledcWriteTone(0, 0);
    ledcWrite(0, 0);

    w.begin(17, 5);
    delay(50);


    bool ok = imu.init();
    if (!ok) {
        Serial.println("IMU Error!");
        ESP.deepSleep(UINT32_MAX);
    }

    imu.enableGyrDLPF();
    imu.setGyrDLPF(MPU9250_DLPF_6);  // lowest noise
    imu.setGyrRange(MPU9250_GYRO_RANGE_250); // highest resolution
    imu.setAccRange(MPU9250_ACC_RANGE_2G);
    imu.enableAccDLPF(true);
    imu.setAccDLPF(MPU9250_DLPF_6);

    imu.autoOffsets();
    //pinMode(2, OUTPUT);
    //pinMode(25, OUTPUT);

    //Serial.println("Begin task");
    disableCore0WDT();
    disableCore1WDT();

    xTaskCreatePinnedToCore(integrator_task, "integrator", 8192, NULL, 0, NULL, 1);
    //xTaskCreate(discovery_task, "mdns", 8192, NULL, 0, NULL);
    xTaskCreate(send_task, "mdns", 8192, NULL, 0, NULL);
    MDNS.begin("gyro");
    MDNS.addService("imu", "udp", 11752);

    udp.onPacket(on_packet);
    udp.listen(11752);
    udp.connect(remote, 11752);
    //WiFi.config(IPAddress(192, 168, 137, 5), IPAddress(192, 168, 137, 1), IPAddress(255, 255, 0, 0), IPAddress(192, 168, 137, 1), IPAddress(8, 8, 8, 8));
    //WiFi.begin("ARMATRON_NETWORK", "16777216");
    //Serial.println("OK.");
}

void discovery_task(void* param) {
    while (true) {
        IPAddress address;
        while (true) {
            delay(1000);
            address = MDNS.queryHost("driver");

            if (remote != IPAddress(0, 0, 0, 0))
                address = remote;

            if (address != IPAddress(0, 0, 0, 0))
                break;
        }

        ESP_LOGI("main", "Connected to driver! Address: %s", address.toString().c_str());
        udp.connect(address, 11752);
        remote = address;

        while (millis() - last_pkt < 1000) {
            delay(100);
        }

        remote = IPAddress(0, 0, 0, 0);
    }
}

void write(String msg) {
    //ESP_LOGI("main", "Message: %s", msg.c_str());
    udp.print((char*)msg.c_str());
}

struct avg_entry {
public:
    long m;
    xyzFloat v_angular;
    xyzFloat v_g;

    avg_entry(long ml, xyzFloat a, xyzFloat g)
    {
        m = ml;
        v_angular = a;
        v_g = g;
    }
};

bool calibrating = false;

void calibrate() {
    Serial.println("Calibrating gyro!");
    write("state:\tcalib_wait ;");
    calibrating = true;


    avg_entry* values[512];
    int numValues = 0;

    double avg = 0;
    xyzFloat read;
    /*while (millis() - start_millis < 1000 || abs(avg - last_read) > 0.1)
    {
        delay(10);
        values[numValues++] = new avg_entry(millis(), imu.getGyrValues(), imu.getGValues());

        for (int i = 0; i < numValues; i++) {
            if (millis() - values[i]->m > 1000)
            {
                delete values[i];
                for (int j = i; j < numValues - 1; j++)
                    values[j] = values[j + 1];
                numValues--;
            }
        }

        for (int i = 0; i < numValues; i++)
            avg += values[i]->v;

        avg /= numValues;

        digitalWrite(25, millis() % 1000 < 250);
    }

    for (int j = 0; j < numValues; j++)
        delete values[j];*/

    while (true) {
        ledcWriteTone(0, 3000);
        delay(100);
        ledcWriteTone(0, 1000);
        delay(100);

        double start_millis = millis();
        xyzFloat start_read_gyr = imu.getGyrValues();
        xyzFloat start_read_acc = imu.getGValues();
        double max_mag_gyr = 0;
        double max_mag_acc = 0;

        while (millis() - start_millis < 1000) {
            xyzFloat read_gyr = imu.getGyrValues();
            xyzFloat read_acc = imu.getGValues();

            xyzFloat diff_gyr = read_gyr - start_read_gyr;
            xyzFloat diff_acc = read_acc - start_read_acc;
            double mag_gyr = diff_gyr.x + diff_gyr.y + diff_gyr.z;
            double mag_acc = diff_acc.x + diff_acc.y + diff_acc.z;

            if (abs(mag_gyr) > abs(max_mag_gyr))
                max_mag_gyr = mag_gyr;

            if (abs(mag_acc) > abs(max_mag_acc))
                max_mag_acc = mag_acc;
        }
        Serial.printf("Mag: %f - %f\n", max_mag_gyr, max_mag_acc);

        if (abs(max_mag_gyr) < 0.32 && abs(max_mag_acc) < 0.007)
            break;
    }

    ledcWriteTone(0, 3000);
    delay(100);
    ledcWriteTone(0, 1000);
    delay(100);
    ledcWrite(0, 0);
    delay(200);

    ledcWriteTone(0, 3000);
    delay(100);
    ledcWriteTone(0, 1000);
    delay(100);
    ledcWrite(0, 0);
    delay(200);

    ledcWriteTone(0, 3000);
    imu.autoOffsets();
    ledcWriteTone(0, 1000);
    delay(100);
    ledcWrite(0, 0);
    delay(1000);
    tilt = 0;

    calibrating = false;
}

double z_degrees = 0;
double prev_time = 0;
void integrator_task(void* param) {
    while (true) {
        if (calibrating)
        {
            delay(100);
            continue;
        }
        double t = ((double)micros()) / 1000000;
        double dT = prev_time - t;
        prev_time = t;

        double dZ = imu.getGyrValues().z;
        double dTilt = imu.getGyrValues().x;
        dZ = abs(dZ) > 0.1 ? dZ : 0;

        acc = imu.getGValues() * 9.81;

        /*if (abs(acc.x) < 0.05)
            acc.x = 0;
        if (abs(acc.y) < 0.05)
            acc.y = 0;
        if (abs(acc.z) < 0.05)*/
        acc.z = 0;

        spd += acc * dT;

        pos += spd * dT;

        z_degrees += dZ * dT;
        tilt += dTilt * dT;

        delayMicroseconds(1000);
    }
}

double get_angle() {
    return z_degrees;// imu.getAngles().z - angle_offset;
}

void set_angle(double angles) {
    z_degrees = angles;// angle_offset = imu.getAngles().z - angles;
}

void zero_update() {
    spd = xyzFloat(0, 0, 0);
    pos = xyzFloat(0, 0, 0);
}

void on_packet(AsyncUDPPacket pkt) {
    String read = String((char*)pkt.data(), pkt.length());
    Serial.println("Read: " + read);

    String spliced[32];
    int numSplices = 0;

    int idx = read.indexOf(' ');
    while (idx != -1) {
        idx = read.indexOf(' ');

        String token = read.substring(0, idx);
        //Serial.println(token);
        spliced[numSplices++] = token;
        read = read.substring(idx + 1);
    }

    if (spliced[0] == "reset")
        ESP.restart();

    else if (spliced[0] == "calib")
        calibrate();

    else if (spliced[0] == "zero_vel")
        zero_update();

    else if (spliced[0] == "set_angle" && numSplices > 1)
        set_angle(atof(spliced[1].c_str()));

    else if (spliced[0] == "connect") {
        udp.connect(pkt.remoteIP(), 11752);
        remote = pkt.remoteIP();
    }

    last_pkt = millis();
}

void send_task(void* param) {
    while (true) {
        delay(50);

        if (remote == IPAddress(0, 0, 0, 0))
            continue;

        if (calibrating) {
            udp.printf("state:calibrating");
            continue;
        }

        udp.printf("angle:%f;tilt:%f;accel:%f,%f,%f;speed:%f,%f,%f;position:%f,%f,%f;state:ok;", get_angle(), tilt, acc.x, acc.y, acc.z, spd.x, spd.y, spd.z, pos.x, pos.y, pos.z);
    }
}
void loop() {
    delay(250);

    if (Serial.available()) {
        String read = Serial.readStringUntil(';');
        String spliced[32];
        int numSplices = 0;

        int idx = read.indexOf(' ');
        while (idx != -1) {
            idx = read.indexOf(' ');

            String token = read.substring(0, idx);
            //Serial.println(token);
            spliced[numSplices++] = token;
            read = read.substring(idx + 1);
        }

        if (spliced[0] == "reset")
            ESP.restart();

        if (spliced[0] == "calib")
            calibrate();

        else if (spliced[0] == "zero_vel")
            zero_update();
    }

    Serial.printf("Remote:\t%s\n", remote.toString().c_str());
    Serial.printf("IP:\t%s\n", ETH.localIP().toString().c_str());

    Serial.printf("angle:\t%f\n", get_angle());
    Serial.printf("tilt:\t%f\n", tilt);

    Serial.printf("x:\t%f\n", spd.x);
    Serial.printf("y:\t%f\n", spd.y);
    Serial.printf("z:\t%f\n", spd.z);

    if (calibrating) {
        Serial.printf("state:\tcalibrating\n");
        return;
    }


    if (remote == IPAddress(0, 0, 0, 0))
    {
        Serial.printf("state:\tdisconnected\n");
        return;
    }

    Serial.printf("state:\tok\n");
}
