/**
 * @file simple_serial_interface.ino
 * @brief Controls an LED via serial commands on an Arduino-compatible board.
 *
 * This sketch listens for serial input and toggles the state of an LED connected to LED_PIN (default: pin 13).
 * - If received serial data (as integer) is 0, the LED is turned off.
 * - If received serial data is any other value, the LED is turned on.
 * The current state is reported back over serial after each update.
 *
 * Baud rate: 115200
 * Serial timeout: 1 ms
 *
 * Usage:
 *   Send an integer value over serial to control the LED.
 *   The board will respond with a message indicating the LED state.
 */
 
#define LED_PIN 13

int data = 0; // Variable to hold the incoming data

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Initialize LED off

    Serial.begin(115200); // Initialize serial communication at 115200 baud
    Serial.setTimeout(1); // Set a timeout for serial read operations
}

void loop() {
    if (Serial.available()) 
    {
        int data = Serial.readString().toInt();
        if(data == 0){
            digitalWrite(LED_PIN, LOW); // Turn off LED
        } else {
            digitalWrite(LED_PIN, HIGH); // Turn on LED
        }
    }
    Serial.println("LED state updated based on current data: " + String(data));
    delay(0.1); // Small delay to avoid overwhelming the serial buffer
}