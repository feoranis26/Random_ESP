#include <AccelStepper.h>
#include <MultiStepper.h>
#define motorPin1  2     // IN1 on ULN2003 ==> Blue   on 28BYJ-48
#define motorPin2  3     // IN2 on ULN2004 ==> Pink   on 28BYJ-48
#define motorPin3  4    // IN3 on ULN2003 ==> Yellow on 28BYJ-48
#define motorPin4  5    // IN4 on ULN2003 ==> Orange on 28BYJ-48

int endPoint = 1024;        // Move this many steps - 1024 = approx 1/4 turn

// NOTE: The sequence 1-3-2-4 is required for proper sequencing of 28BYJ-48
AccelStepper stepper1(8, motorPin1, motorPin3, motorPin2, motorPin4);

void setup() {
    stepper1.setAcceleration(1000);
    stepper1.setMaxSpeed(1000);
    stepper1.setSpeed(500);
}

// the loop function runs over and over again until power down or reset
void loop() {
    stepper1.runSpeed();
}
