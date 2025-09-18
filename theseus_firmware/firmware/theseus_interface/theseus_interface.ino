#include <Servo.h>

/* Definition of each joint */

struct ServoMotor {
    Servo servo;
    uint8_t pin;
    int start_angle;
    int max_usable_angle;
};

// Create and initialize in one line
ServoMotor shoulder = {Servo(), 16, 0, 270};      // shoulder joint (160kgcm)
ServoMotor elbow = {Servo(), 17, 0, 270};  // elbow joint (80kgcm)

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
    return map(goal, 0, 360, 0, maxUsable);
}

void servo_reach_goal(ServoMotor &motor, int goal) {
    goal = scaledAngle(goal, motor.max_usable_angle);
    motor.servo.write(goal);
//      motor.write(goal);
    // if(goal>=motor.servo.read())
    // {
    //     for(int pos = motor.servo.read(); pos <= goal; pos++) {
    //         motor.servo.write(pos);
    //         delay(50);
    //     }
    // }
    // else
    // {
    //     for(int pos = motor.servo.read(); pos >= goal; pos--) {
    //         motor.servo.write(pos);
    //         delay(5);
    //     }
    // }
}


void setup() {
    // Attach servo joints to their respective pins
    shoulder.servo.attach(shoulder.pin);
    elbow.servo.attach(elbow.pin);

    // Initialize joints to their starting positions
    // servo_reach_goal(shoulder, shoulder.start_angle);
    // servo_reach_goal(elbow, elbow.start_angle);

    Serial.begin(115200); // Initialize serial communication at 115200 baud
    Serial.setTimeout(1); // Set a timeout for serial read operations
    Serial.println("Setup ok");
}

void loop() {
    if (Serial.available()) 
    {
        /* Decode the custom serial protocol, it follows the following basic structure:

            b<angle_value>,s<angle_value>

            - s: shoulder joint
            - e : elbow joint.

        */
        char chr = Serial.read();
        Serial.println(chr);
        if(chr == 's')
        {
            joint_idx = 1;
            angle_value_idx = 0; // Reset angle value index
        } 
        else if (chr == 'e')
        {
            joint_idx = 2;
            angle_value_idx = 0; // Reset angle value index
        }
        /* Finalized the parsing of a joint, send the angle for the joint */
        else if (chr == ',')
        {
            Serial.println(String(joint_idx));
            int angle = atoi(angle_value); // Convert the angle value string to an integer
            if (joint_idx == 1) 
            {
                Serial.println("Sending shoulder joint to : " + String(angle));
                servo_reach_goal(shoulder, angle);
            } 
            else if (joint_idx == 2) 
            {
                Serial.println("Sending elbow joint to : " + String(angle));
                servo_reach_goal(elbow, angle);
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
