#include <Servo.h>

/* Definition of each joint */

// Base joint
#define SERVO_BASE_PIN 22
#define BASE_START 90
Servo base;

// Shoulder joint
#define SERVO_SHOULDER_PIN 24
#define SHOULDER_START 90
Servo shoulder;

/* Support variables for saving the decoded angle */
uint8_t joint_idx = 0; // Index for the current joint being controlled
uint8_t angle_value_idx = 0; // Value for the current joint
char angle_value[3] = "000";

void servo_reach_goal(Servo motor, int goal)
{
      motor.write(goal);
//    if(goal>=motor.read())
//    {
//        for(int pos = motor.read(); pos <= goal; pos++) {
//            motor.write(pos);
//            delay(5);
//        }
//    }
//    else
//    {
//        for(int pos = motor.read(); pos >= goal; pos--) {
//            motor.write(pos);
//            delay(5);
//        }
//    }
}


void setup() {
    // Attach servo joints to their respective pins
    base.attach(SERVO_BASE_PIN);
    shoulder.attach(SERVO_SHOULDER_PIN);

    // Initialize joints to their starting positions
    base.write(BASE_START);
    shoulder.write(SHOULDER_START);

    Serial.begin(115200); // Initialize serial communication at 115200 baud
    Serial.setTimeout(1); // Set a timeout for serial read operations
    Serial.println("Setup ok");
}

void loop() {
    if (Serial.available()) 
    {
        /* Decode the custom serial protocol, it follows the following basic structure:

            b<angle_value>,s<angle_value>

            - b: base joint
            - s : shoulder joint.

        */
        char chr = Serial.read();
        Serial.println(chr);
        if(chr == 'b')
        {
            joint_idx = 0;
            angle_value_idx = 0; // Reset angle value index
        } 
        else if (chr == 's')
        {
            joint_idx = 1;
            angle_value_idx = 0; // Reset angle value index
        }
        /* Finalized the parsing of a joint, send the angle for the joint */
        else if (chr == ',')
        {
            Serial.println(String(joint_idx));
            int angle = atoi(angle_value); // Convert the angle value string to an integer
            if (joint_idx == 0) 
            {
                Serial.println("Sending base joint to : " + String(angle));
                servo_reach_goal(base, angle);
            } 
            else if (joint_idx == 1) 
            {
                Serial.println("Sending shoulder joint to : " + String(angle));
                servo_reach_goal(shoulder, angle);
            }

            // Reset angle value for the next joint
            angle_value[0] = '0';
            angle_value[1] = '0';
            angle_value[2] = '0';
            angle_value[3] = '\0'; 
        }
        /* If its a number, it must be saved, its part of the angle value */
        else
        {
            angle_value[angle_value_idx] = chr; // Save character to the current angle value index
            angle_value_idx++; // Increment the index for next numeric angles characters to be read
        }
    }
}
