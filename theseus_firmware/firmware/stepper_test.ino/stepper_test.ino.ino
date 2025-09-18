#include <AccelStepper.h>

// Define stepper motor connections for RAMPS 1.4
// Example: X-axis driver → STEP pin 54, DIR pin 55 (on Arduino Mega with RAMPS)
#define X_STEP_PIN A0
#define X_DIR_PIN A1
#define X_ENABLE_PIN 38

// Create stepper instance in DRIVER mode (step/dir)
AccelStepper stepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);

void setup() {
  pinMode(X_ENABLE_PIN, OUTPUT);
  digitalWrite(X_ENABLE_PIN, LOW); // Enable driver (LOW = enabled on RAMPS)

  stepper.setMaxSpeed(5000);   // steps per second
  stepper.setAcceleration(200); // steps per second^2
  stepper.moveTo(20000);        // target position (steps)
}

void loop() {
  if (stepper.distanceToGo() == 0) {
    // Reverse direction when target reached
    stepper.moveTo(-stepper.currentPosition());
  }
  stepper.run(); // non-blocking step execution
}
