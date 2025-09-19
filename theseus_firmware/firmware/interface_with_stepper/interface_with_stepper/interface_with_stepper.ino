#include <Servo.h>
#include <AccelStepper.h>

#define SERVO_DELAY 20 // Reduced delay for smoother, faster movement
#define HOMING_SPEED -6000

/*
================================================================================
MOTOR STRUCT DEFINITIONS
- Moved to the top of the file so they are declared before being used.
================================================================================
*/

struct ServoMotor {
    Servo servo;
    uint8_t pin;
    int start_angle;
    int max_usable_angle;
};

struct StepperMotor {
    AccelStepper stepper;
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t enable_pin;
    uint8_t endstop_pin;
    int reduc;
    int steps_per_rev;
    int max_speed;
    int max_accel;
    int start_angle;
    int current_angle;
};

/*
================================================================================
MOTOR CONTROL FUNCTIONS
================================================================================
*/

// Maps an input angle (0–360) to servo's usable range
int scaledAngle(int goal, int maxUsable) {
    return map(goal, 0, 360, 0, maxUsable);
}

void servoReachGoal(ServoMotor &motor, int goal) {
    goal = scaledAngle(goal, motor.max_usable_angle);
    int currentPos = motor.servo.read();

    if (goal > currentPos) {
        for (int pos = currentPos; pos <= goal; pos++) {
            motor.servo.write(pos);
            delay(SERVO_DELAY);
        }
    } else {
        for (int pos = currentPos; pos >= goal; pos--) {
            motor.servo.write(pos);
            delay(SERVO_DELAY);
        }
    }
}

long angleToSteps(int angle, StepperMotor stepper) {
    Serial.println("Goal angle to motor:");
    Serial.println(angle);

    // Direct formula instead of map()
    long steps = (long)angle * stepper.steps_per_rev * stepper.reduc / 360;

    Serial.println("Sending steps to motor:");
    Serial.println(steps);

    return steps;
}


void resetStepperPosition(AccelStepper &stepper, uint8_t endstop_pin) {
    // Set a moderate speed for homing
    stepper.setSpeed(HOMING_SPEED); // Negative speed to move towards the endstop

    // Move until the endstop is pressed
    Serial.println("Starting homing procedure...");
    while (digitalRead(endstop_pin) == HIGH) {
        Serial.println("Homing...");
        stepper.runSpeed();
    }

    // Stop the motor and set the current position as 0
    stepper.stop();
    stepper.setCurrentPosition(0);
    Serial.println("Base homed.");
}

/*
================================================================================
MOTOR AND JOINT DEFINITIONS
================================================================================
*/
/* Definition of each joint */
ServoMotor shoulder = {Servo(),
                      16, 
                      0, 
                      270};      // shoulder joint (160kgcm)

ServoMotor elbow    = {Servo(),
                      17, 
                      0, 
                      270};  // elbow joint (80kgcm)

#define baseStepPin A0
#define baseDirPin A1
StepperMotor base   = {AccelStepper(AccelStepper::DRIVER, baseStepPin, baseDirPin),
                      baseStepPin, baseDirPin, 38, 18,
                      99,
                      3200,
                      6000, // steps per second
                      1000,  // steps per second^2
                      0,  // initial position
                      0};  
                      
void resetStepperReference() {
    base.current_angle = 135;
}
                    
void setup() {
    pinMode(base.enable_pin, OUTPUT);
    pinMode(base.dir_pin, OUTPUT);
    pinMode(base.step_pin, OUTPUT);
    pinMode(base.endstop_pin, INPUT);
    pinMode(shoulder.pin, OUTPUT);
    pinMode(elbow.pin, OUTPUT);

    // Attach the interrupt to the endstop pin
    attachInterrupt(digitalPinToInterrupt(base.endstop_pin), resetStepperReference, FALLING);
    
    // Attach servo joints to their respective pins
    shoulder.servo.attach(shoulder.pin);
    elbow.servo.attach(elbow.pin);
    digitalWrite(base.enable_pin, LOW);
    base.stepper.setMaxSpeed(base.max_speed);      // steps per second
    base.stepper.setAcceleration(base.max_accel);   // steps per second^2

    Serial.begin(115200); // Initialize serial communication at 115200 baud
    Serial.setTimeout(1); // Set a timeout for serial read operations
    Serial.println("Setup ok");
    
    // Initialize joints to their starting positions
    servoReachGoal(shoulder, shoulder.start_angle);
    servoReachGoal(elbow, elbow.start_angle);
    resetStepperPosition(base.stepper, base.endstop_pin);
    base.stepper.moveTo(angleToSteps(base.start_angle, base));
}

/* Support variables for saving the decoded angle */
uint8_t joint_idx = 0; // Index for the current joint being controlled
uint8_t angle_value_idx = 0; // Value for the current joint
char angle_value[4] = "000";

void loop() {

    if(base.stepper.distanceToGo() != 0) {
        base.stepper.run();
    }
    if (Serial.available()) 
    {
        /* Decode the custom serial protocol, it follows the following basic structure:

            b<angle_value>,s<angle_value>

            - s: shoulder joint
            - e : elbow joint.

        */
        char chr = Serial.read();
        Serial.println(chr);
        
        if (chr == 'b')
        {
            joint_idx = 0;
            angle_value_idx = 0; // Reset angle value index
        }
        else if(chr == 's')
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
            if (joint_idx == 0)
            {
                Serial.println("Sending base joint to: " + String(angle));
                base.stepper.moveTo(angleToSteps(angle, base));
            }
            else if (joint_idx == 1) 
            {
                Serial.println("Sending shoulder joint to : " + String(angle));
                servoReachGoal(shoulder, angle);
            } 
            else if (joint_idx == 2) 
            {
                Serial.println("Sending elbow joint to : " + String(angle));
                servoReachGoal(elbow, angle);
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
