#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

// Replace with your network credentials
const char* ssid = "Totalplay-CE9F";
const char* password = "CE9F11A17A8HZzvs";

// GPIO pins for the RGB LED
const int redPin = 8;
const int greenPin = 9;
const int bluePin = 10;

// PWM Properties
const int freq = 5000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8; // 8-bit resolution (0-255)

// --- Animation and Color State ---
// Stores the currently selected solid color
int currentR = 255;
int currentG = 0;
int currentB = 0;
// Stores the current animation mode
String currentAnimation = "solid";

// --- Animation Timing & State Variables ---
unsigned long lastUpdate = 0;
int animationInterval = 20; // Update animation every 20ms
// For Rainbow
int hue = 0;
// For Breathe
int breatheBrightness = 0;
bool breatheUp = true;

// Create a web server object
WebServer server(80);

// --- Low-level LED Control ---
// Sets the physical color of the LED strip
void setStripColor(int r, int g, int b) {
  // Invert the values for a common-anode LED strip
  ledcWrite(redChannel, 255 - r);
  ledcWrite(greenChannel, 255 - g);
  ledcWrite(blueChannel, 255 - b);
}

// --- HSV to RGB Conversion ---
// Needed for the rainbow effect for smooth color transitions
void hsvToRgb(int h, int s, int v, int* r, int* g, int* b) {
  int i;
  float f, p, q, t;
  
  if (s == 0) { *r = *g = *b = v; return; }
  
  h %= 360;
  i = h / 60;
  f = (float)(h % 60) / 60;
  p = v * (1 - (float)s / 255);
  q = v * (1 - (float)s / 255 * f);
  t = v * (1 - (float)s / 255 * (1 - f));
  
  switch (i) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
  }
}

// --- Animation Functions ---
void runRainbow() {
  hue = (hue + 1) % 360;
  int r, g, b;
  hsvToRgb(hue, 255, 255, &r, &g, &b);
  setStripColor(r, g, b);
}

void runBreathe() {
  if (breatheUp) {
    breatheBrightness++;
    if (breatheBrightness >= 255) {
      breatheBrightness = 255;
      breatheUp = false;
    }
  } else {
    breatheBrightness--;
    if (breatheBrightness <= 0) {
      breatheBrightness = 0;
      breatheUp = true;
    }
  }
  float factor = (float)breatheBrightness / 255.0;
  setStripColor(currentR * factor, currentG * factor, currentB * factor);
}

// --- Web Server Handlers ---
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 RGB LED Control</title>
  <style>
    html { font-family: Helvetica; text-align: center; background: #f5f7fa; margin: 0; padding: 20px; }
    body { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    h1 { color: #000000ff; font-size: 24px; margin-bottom: 20px; }
    .color-box { width: 100px; height: 100px; border: 1px solid #ccc; margin: 20px auto; border-radius: 8px; }
    .form-group { margin-bottom: 20px; }
    label { font-size: 18px; color: #1f1f1fff; display: block; margin-bottom: 5px; }
    input[type="color"], input[type="submit"], select {
      padding: 10px;
      border-radius: 8px;
      border: 1px solid #ddd;
      font-size: 16px;
      cursor: pointer;
      width: 100%;
      box-sizing: border-box;
    }
    input[type="color"] { height: 50px; padding: 5px; }
    input[type="submit"] {
      background: #4c6fafff; color: white; border: none;
      padding: 12px 24px; transition: background 0.2s ease;
    }
    input[type="submit"]:hover { background: #5983cfff; }
  </style>
</head>
<body>
  <h1>ESP32 RGB LED Control</h1>
  <div class="color-box" style="background-color: rgb(%d, %d, %d);"></div>
  <form action="/set" method="POST">
    <div class="form-group">
      <label for="animation">Animation Preset</label>
      <select id="animation" name="animation">
        <option value="solid" %s>Solid Color</option>
        <option value="breathe" %s>Breathe</option>
        <option value="rainbow" %s>Rainbow Cycle</option>
      </select>
    </div>
    <div class="form-group">
      <label for="color">Select Color (for Solid/Breathe)</label>
      <input type="color" id="color" name="color" value="#%02x%02x%02x">
    </div>
    <input type="submit" value="Apply Changes">
  </form>
</body>
</html>
)rawliteral";

  // Allocate buffer for HTML
  size_t bufSize = html.length() + 200;
  char* formattedHtml = (char*) malloc(bufSize);
  if (!formattedHtml) {
    server.send(500, "text/plain", "Internal Server Error: Malloc Failed");
    return;
  }

  // Add 'selected' attribute to the correct dropdown option
  char solidSelected[10] = "";
  char breatheSelected[10] = "";
  char rainbowSelected[10] = "";
  if (currentAnimation == "solid") strcpy(solidSelected, "selected");
  else if (currentAnimation == "breathe") strcpy(breatheSelected, "selected");
  else if (currentAnimation == "rainbow") strcpy(rainbowSelected, "selected");

  sprintf(formattedHtml, html.c_str(), currentR, currentG, currentB, solidSelected, breatheSelected, rainbowSelected, currentR, currentG, currentB);
  
  server.send(200, "text/html", formattedHtml);
  free(formattedHtml);
}

void handleSet() {
  // Update animation mode
  if (server.hasArg("animation")) {
    currentAnimation = server.arg("animation");
    Serial.printf("Animation set to: %s\n", currentAnimation.c_str());
    // If we switch to solid, immediately set the color
    if (currentAnimation == "solid") {
      // continue to color processing
    } else {
      // Reset animation states when switching
      hue = 0;
      breatheBrightness = 0;
      breatheUp = true;
    }
  }

  // Update color
  if (server.hasArg("color")) {
    String colorStr = server.arg("color");
    long number = strtol(&colorStr[1], NULL, 16);
    currentR = (number >> 16) & 0xFF;
    currentG = (number >> 8) & 0xFF;
    currentB = number & 0xFF;
    Serial.printf("Base color set to: R=%d, G=%d, B=%d\n", currentR, currentG, currentB);
    if (currentAnimation == "solid") {
      setStripColor(currentR, currentG, currentB);
    }
  }
  
  // Redirect back to the root page
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);

  ledcAttachPin(redPin, redChannel);
  ledcAttachPin(greenPin, greenChannel);
  ledcAttachPin(bluePin, blueChannel);

  // Set initial state
  setStripColor(currentR, currentG, currentB);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handleSet);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (millis() - lastUpdate > animationInterval) {
    lastUpdate = millis();
    if (currentAnimation == "rainbow") {
      runRainbow();
    } else if (currentAnimation == "breathe") {
      runBreathe();
    }
    // For "solid", we do nothing here, as the color is set once in handleSet() 
  }
}
