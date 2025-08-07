#include <Servo.h>

/* Definition of each joint */

struct ServoMotor {
    Servo servo;
    uint8_t pin;
    int start_angle;
    int max_usable_angle;
};

// Create and initialize in one line
ServoMotor base = {Servo(), 22, 0, 130};      // Base joint (160kgcm)
ServoMotor shoulder = {Servo(), 24, 0, 130};  // Shoulder joint (80kgcm)

/* Support variables for saving the decoded angle */
uint8_t joint_idx = 0; // Index for the current joint being controlled
uint8_t angle_value_idx = 0; // Value for the current joint
char angle_value[4] = "000";

void testServoPulseRange(Servo motor){
  // Sweep from 500 µs to 2500 µs in 10 µs steps
  for (int us = 500; us <= 2500; us += 10) {
    motor.writeMicroseconds(us);
    Serial.print("Pulse: ");
    Serial.println(us);
    delay(300); // wait so you can watch movement
  }
}

// Maps an input angle (0–180) to servo's usable range
int scaledAngle(int goal, int maxUsable) {
    return map(goal, 0, 180, 0, maxUsable);
}

void servo_reach_goal(ServoMotor &motor, int goal) {
    motor.servo.write(scaledAngle(goal, motor.max_usable_angle));
//      motor.write(goal);
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
    base.servo.attach(base.pin);
    shoulder.servo.attach(shoulder.pin);

    // Initialize joints to their starting positions
    servo_reach_goal(base, base.start_angle);
    servo_reach_goal(shoulder, shoulder.start_angle);

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
