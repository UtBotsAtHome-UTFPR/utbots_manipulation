#include <Servo.h>
#include <AccelStepper.h>

#define SERVO_DELAY 50 

/* Motor structs and functions */

struct ServoMotor {
    Servo servo;
    uint8_t pin;
    int start_angle;
    int max_usable_angle;

    ServoMotor(uint8_t pin, int start_angle, int max_usable_angle)
      : servo(Servo()),
        pin(pin),
        start_angle(start_angle),
        max_usable_angle(max_usable_angle) {}
};

// Maps an input angle (0–180) to servo's usable range
int scaledAngle(int goal, int maxUsable) {
    return map(goal, 0, 360, 0, maxUsable);
}

void servoReachGoal(ServoMotor &motor, int goal) {
    goal = scaledAngle(goal, motor.max_usable_angle);
    motor.servo.write(goal);
    motor.write(goal);
    if(goal>=motor.servo.read())
    {
        for(int pos = motor.servo.read(); pos <= goal; pos++) {
            motor.servo.write(pos);
            delay(SERVO_DELAY);
        }
    }
    else
    {
        for(int pos = motor.servo.read(); pos >= goal; pos--) {
            motor.servo.write(pos);
            delay(SERVO_DELAY);
        }
    }
}

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

    StepperMotor(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin,
                 int reducVal, int steps_per_rev, int maxSpd, int maxAcc, int startAng)
      : stepper(AccelStepper::DRIVER, stepPin, dirPin),
        step_pin(stepPin),
        dir_pin(dirPin),
        enable_pin(enablePin),
        endstop_pin(endstop_pin),
        reduc(reducVal),
        steps_per_rev(steps_per_rev),
        max_speed(maxSpd),
        max_accel(maxAcc),
        start_angle(startAng) {}
};

int angleToSteps(int angle, StepperMotor &motor) {
    float deg_per_step = 360 / motor.steps_per_rev;
    return (int) angle / deg_per_step; 
}

void resetStepperPosition(StepperMotor &motor) {
    if (digitalRead(motor.endstop_pin) == HIGH) {
        motor.stepper.stop();
        while (motor.stepper.isRunning()) {
        motor.stepper.run();
    } else {
        motor.stepper.setSpeed(1000); // positive speed = forward
        motor.stepper.runSpeed();     // non-blocking step execution
    }
    return;
  }
}

/* Definition of each joint */
ServoMotor shoulder (16, 
                    0, 
                    270);      // shoulder joint (160kgcm)

ServoMotor elbow    (17, 
                    0, 
                    270);  // elbow joint (80kgcm)

StepperMotor base   (A0, A1, 38, 55
                    1,
                    200,
                    6000, // steps per second
                    200,  // steps per second^2
                    180);  // initial position

                    
void setup() {
    // Attach servo joints to their respective pins
    shoulder.servo.attach(shoulder.pin);
    elbow.servo.attach(elbow.pin);
    
    // Initialize joints to their starting positions
    servo_reach_goal(shoulder, shoulder.start_angle);
    servo_reach_goal(elbow, elbow.start_angle);
    resetStepperPosition(base);
    base.stepper.moveTo(angleToSteps(base.start_angle, base.stepper))

    Serial.begin(115200); // Initialize serial communication at 115200 baud
    Serial.setTimeout(1); // Set a timeout for serial read operations
    Serial.println("Setup ok");
}

void loop() {
    /* Support variables for saving the decoded angle */
    uint8_t joint_idx = 0; // Index for the current joint being controlled
    uint8_t angle_value_idx = 0; // Value for the current joint
    char angle_value[4] = "000";

    if (Serial.available()) 
    {
        /* Decode the custom serial protocol, it follows the following basic structure:

            b<angle_value>,s<angle_value>

            - s: shoulder joint
            - e : elbow joint.

        */
        char chr = Serial.read();
        Serial.println(chr);
        if (chr = "b")
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
                Serial.println("Sending base joint to: " + String(angle))
                base.stepper.moveTo(angleToSteps(angle, base.stepper))
            }
            else if (joint_idx == 1) 
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
