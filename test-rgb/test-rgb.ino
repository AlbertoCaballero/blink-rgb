// RGB LED Diagnostic Sketch - Round 2 (New Pins + Inverted Logic)

// NEW GPIO pins for the RGB LED
const int redPin = 8;
const int greenPin = 9;
const int bluePin = 10;

// PWM Properties
const int freq = 5000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8;

// Helper function to set color (INVERTED logic for Common Anode)
void setColor(int r, int g, int b) {
  ledcWrite(redChannel, 255 - r);
  ledcWrite(greenChannel, 255 - g);
  ledcWrite(blueChannel, 255 - b);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("\nStarting RGB diagnostic test (Round 2)...");
  Serial.println("Using NEW PINS: R=8, G=9, B=10");
  Serial.println("Using INVERTED logic for Common Anode strip.");

  // Configure LED PWM channels
  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);

  // Attach the GPIO pins to the PWM channels
  ledcAttachPin(redPin, redChannel);
  ledcAttachPin(greenPin, greenChannel);
  ledcAttachPin(bluePin, blueChannel);

  // Start with LEDs off
  setColor(0, 0, 0);
  delay(1000);
}

void loop() {
  // Test RED
  Serial.println("Testing RED channel (should be bright red)");
  setColor(255, 0, 0);
  delay(3000);

  // Test GREEN
  Serial.println("Testing GREEN channel (should be bright green)");
  setColor(0, 255, 0);
  delay(3000);

  // Test BLUE
  Serial.println("Testing BLUE channel (should be bright blue)");
  setColor(0, 0, 255);
  delay(3000);
}