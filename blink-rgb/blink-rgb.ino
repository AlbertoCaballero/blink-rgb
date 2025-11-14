#include <WiFi.h>
#include <WebServer.h>

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

// Current color state
int currentR = 255;
int currentG = 255;
int currentB = 255;

// Create a web server object
WebServer server(80);

// Function to set the color of the RGB LED
void setColor(int r, int g, int b) {
  currentR = r;
  currentG = g;
  currentB = b;
  // Invert the values for a common-anode LED strip
  ledcWrite(redChannel, 255 - r);
  ledcWrite(greenChannel, 255 - g);
  ledcWrite(blueChannel, 255 - b);
  Serial.printf("Set color to: R=%d, G=%d, B=%d (Inverted for Common Anode)\n", r, g, b);
}

// Function to handle the root URL
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
    input[type="color"], input[type="submit"] {
      padding: 10px;
      border-radius: 8px;
      border: 1px solid #ddd;
      font-size: 16px;
      cursor: pointer;
    }
    input[type="color"] { width: 100px; height: 50px; padding: 5px; }
    input[type="submit"] {
      background: #4c6fafff;
      color: white;
      border: none;
      padding: 12px 24px;
      transition: background 0.2s ease;
    }
    input[type="submit"]:hover { background: #5983cfff; }
  </style>
</head>
<body>
  <h1>ESP32 RGB LED Control</h1>
  <div class="color-box" style="background-color: rgb(%d, %d, %d);"></div>
  <form action="/set" method="POST">
    <div class="form-group">
      <label for="color">Select Color</label>
      <input type="color" id="color" name="color" value="#%02x%02x%02x">
    </div>
    <input type="submit" value="Set Color">
  </form>
</body>
</html>
)rawliteral";

  // Allocate buffer dynamically to avoid stack overflow
  size_t bufSize = html.length() + 100; // Base size + buffer for formatted values
  char* formattedHtml = (char*) malloc(bufSize);
  if (!formattedHtml) {
    server.send(500, "text/plain", "Internal Server Error: Malloc Failed");
    return;
  }
  sprintf(formattedHtml, html.c_str(), currentR, currentG, currentB, currentR, currentG, currentB);
  
  server.send(200, "text/html", formattedHtml);

  // Free the allocated memory
  free(formattedHtml);
}

// Function to handle setting the color
void handleSetColor() {
  if (server.hasArg("color")) {
    String colorStr = server.arg("color");
    // Color comes in #RRGGBB format
    long number = strtol(&colorStr[1], NULL, 16);
    int r = (number >> 16) & 0xFF;
    int g = (number >> 8) & 0xFF;
    int b = number & 0xFF;
    setColor(r, g, b);
  }
  // Redirect back to the root page
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  // Configure LED PWM channels
  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);

  // Attach the GPIO pins to the PWM channels
  ledcAttachPin(redPin, redChannel);
  ledcAttachPin(greenPin, greenChannel);
  ledcAttachPin(bluePin, blueChannel);

  // Set initial color to off (black)
  setColor(0, 0, 0);

  // Connect to Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up the web server routes
  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handleSetColor);

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}