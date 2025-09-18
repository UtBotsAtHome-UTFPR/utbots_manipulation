#include <AccelStepper.h>

struct StepperMotor {
    AccelStepper stepper;   // The actual stepper object
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t enable_pin;
    int reduc;
    int steps_per_rev;
    int max_speed;
    int max_accel;
    int start_angle;

    // Constructor for convenience
    StepperMotor(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin,
                 int reducVal, int steps_per_rev, int maxSpd, int maxAcc, int startAng)
      : stepper(AccelStepper::DRIVER, stepPin, dirPin),
        step_pin(stepPin),
        dir_pin(dirPin),
        enable_pin(enablePin),
        reduc(reducVal),
        steps_per_rev(steps_per_rev),
        max_speed(maxSpd),
        max_accel(maxAcc),
        start_angle(startAng) {}
};

StepperMotor stepper(A0, A1, 38,
             1,
             200,
             6000, // steps per second
             200,  // steps per second^2
             180);  // initial position

int angleToSteps(int angle, StepperMotor motor) {
  float deg_per_step = 360 / motor.steps_per_rev;
  return (int) angle / deg_per_step; 
}

void setup() {
  pinMode(stepper.enable_pin, OUTPUT);
  digitalWrite(stepper.enable_pin, LOW); // Enable driver (LOW = enabled on RAMPS)

  stepper.setMaxSpeed(stepper.max_speed);   
  stepper.setAcceleration(stepper.max_accel); 
  stepper.moveTo(angleToSteps(stepper.start_angle));
}

void loop() {
  if (stepper.distanceToGo() == 0) {
    // Reverse direction when target reached
    stepper.moveTo(-stepper.currentPosition());
  }
  stepper.run(); // non-blocking step execution
}
