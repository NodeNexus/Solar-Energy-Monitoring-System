// ----------------------------------------------------
// Solar Energy Monitoring System: ESP32 Code
// Microcontroller: ESP32/ESP8266
// ----------------------------------------------------

#include <WiFi.h>
#include <WebServer.h>

// --- Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin Definitions
const int VOLTAGE_PIN = 34;    // ESP32 ADC1_CH6 (Voltage Sensor Output)
const int CURRENT_PIN = 35;    // ESP32 ADC1_CH7 (Current Sensor Output - ACS712)
const int LOAD_RELAY_PIN = 27; // Relay pin (HIGH = Load ON)

// --- Calibration Values ---
// 1. Voltage Sensor Calibration (based on a 5V analog reference, scaled for 3.3V ADC on ESP32)
// Example: 10k/30k voltage divider ratio (1:4). If input is 12V, ADC reading is 12 * (10k / 40k) = 3V.
// The real-world voltage (V_in) = ADC_Reading * ADC_Max_Voltage / ADC_Resolution * Scaling_Factor
// Since ESP32 ADC is 3.3V max, and 4095 resolution (for 12-bit)
const float VOLTAGE_SCALE_FACTOR = 4.0; // The ratio of the voltage divider (R1+R2)/R2
const float ADC_MAX_VOLTAGE = 3.3; 
const int ADC_RESOLUTION = 4095;

// 2. Current Sensor Calibration (ACS712 5A version example)
const float CURRENT_SENSITIVITY = 0.185; // mV/A for ACS712 5A module (185 mV/A)
const int ZERO_CURRENT_OFFSET = 2048;    // Theoretical zero point for 10-bit ADC (512 for 10-bit)
                                         // For ESP32 (12-bit), center should be around 2048 (if powered by 5V and connected to 3.3V ADC via divider/shifter)
                                         // **NOTE:** Measure the actual offset when no current flows and set this value.

// Load Control Threshold
const float MIN_POWER_TO_CONNECT_W = 10.0; // Minimum required solar panel Power (W) to connect the load

// State Variables
float currentVoltage = 0.0;
float currentCurrent = 0.0;
float currentPower = 0.0;
bool loadConnected = false;
WebServer server(80);

// --- Sensor Reading Functions ---

/**
 * Reads voltage from the analog pin and converts it to real-world volts (V).
 */
float readVoltage() {
  int adcValue = analogRead(VOLTAGE_PIN);
  float voltage = ((float)adcValue / ADC_RESOLUTION) * ADC_MAX_VOLTAGE * VOLTAGE_SCALE_FACTOR;
  return voltage;
}

/**
 * Reads current from the ACS712 sensor and converts it to Amperes (A).
 * Assumes a DC measurement scenario (requires filtering/averaging for AC).
 */
float readCurrent() {
  // Read a sample of readings for better stability
  long totalAdc = 0;
  for (int i = 0; i < 20; i++) {
    totalAdc += analogRead(CURRENT_PIN);
    delay(1);
  }
  int avgAdc = totalAdc / 20;
  
  // Calculate voltage drop from zero reference
  float sensorVoltage = ((float)avgAdc / ADC_RESOLUTION) * ADC_MAX_VOLTAGE;
  
  // Convert sensor voltage to current (I = (V_sense - V_offset) / Sensitivity)
  // V_offset for ESP32/ACS712 setup must be empirically determined. 
  // For simplicity, we assume 1.65V (half of 3.3V) as the zero-current reference
  float voltageOffset = ADC_MAX_VOLTAGE / 2.0; // Theoretical center reference
  
  float currentAmps = (sensorVoltage - voltageOffset) / CURRENT_SENSITIVITY;

  // The ACS712 is highly sensitive to noise; this is a simplified DC reading.
  // We take the absolute value as we are generally concerned with magnitude in solar output.
  return abs(currentAmps);
}

/**
 * Core control logic for the load relay.
 */
void controlLoad() {
  if (currentPower >= MIN_POWER_TO_CONNECT_W) {
    // Solar power is sufficient, connect the load
    if (digitalRead(LOAD_RELAY_PIN) == LOW) {
      digitalWrite(LOAD_RELAY_PIN, HIGH);
      loadConnected = true;
      Serial.printf("LOAD ON: Power %.2f W (above %.1f W threshold).\n", currentPower, MIN_POWER_TO_CONNECT_W);
    }
  } else {
    // Solar power is too low, disconnect the load
    if (digitalRead(LOAD_RELAY_PIN) == HIGH) {
      digitalWrite(LOAD_RELAY_PIN, LOW);
      loadConnected = false;
      Serial.printf("LOAD OFF: Power %.2f W (below %.1f W threshold).\n", currentPower, MIN_POWER_TO_CONNECT_W);
    }
  }
}

// --- Web Server Handler ---

String getPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="refresh" content="5"> 
<title>Solar Monitor</title>
<style>
body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #e0f2f1; margin: 0; padding: 20px;}
.container { max-width: 600px; margin: auto; background: #ffffff; padding: 35px; border-radius: 12px; box-shadow: 0 6px 15px rgba(0,0,0,0.15); text-align: center; }
h1 { color: #004d40; margin-bottom: 25px; }
.metric-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; margin-bottom: 30px; }
.metric-box { background: #f0f4c3; padding: 20px 10px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.05); }
.metric-box h2 { font-size: 1.1em; color: #4caf50; margin: 0 0 5px 0; }
.metric-box p { font-size: 2.0em; font-weight: bold; color: #1b5e20; margin: 0; }
.status-box { padding: 15px; border-radius: 8px; font-weight: bold; font-size: 1.1em; }
.status-box.on { background-color: #a5d6a7; color: #1b5e20; }
.status-box.off { background-color: #ffccbc; color: #d84315; }
.threshold { margin-top: 15px; font-size: 0.9em; color: #757575; }
</style>
</head>
<body>
<div class="container">
<h1>🌞 Solar Energy Monitor</h1>

<div class="metric-grid">
    <div class="metric-box">
        <h2>Voltage (V)</h2>
        <p>%.2f</p>
    </div>
    <div class="metric-box">
        <h2>Current (A)</h2>
        <p>%.2f</p>
    </div>
    <div class="metric-box">
        <h2>Power (W)</h2>
        <p>%.2f</p>
    </div>
</div>

<div class="status-box %s">
    Load Status: %s
</div>

<div class="threshold">
    Load Control Threshold: %.1f W
</div>

<p style="font-size: 0.8em; margin-top: 20px;">Data updates every 5 seconds.</p>

</div>
</body>
</html>
)rawliteral";

  // Determine load status for display
  String status_class = loadConnected ? "on" : "off";
  String status_text = loadConnected ? "CONNECTED" : "DISCONNECTED (Low Power)";
  
  // Format the HTML content
  return String::format(
    html.c_str(),
    currentVoltage,
    currentCurrent,
    currentPower,
    status_class.c_str(),
    status_text.c_str(),
    MIN_POWER_TO_CONNECT_W
  );
}

// Handler for the root page ("/")
void handleRoot() {
  server.send(200, "text/html", getPage());
}

void setup() {
  Serial.begin(115200);
  
  // Set up ADC pins (ESP32 ADC configuration)
  // For ESP32, it's good practice to set attenuation, though not strictly required for basic reading
  // analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db); 
  // analogSetPinAttenuation(CURRENT_PIN, ADC_11db);

  // Pin setup
  pinMode(LOAD_RELAY_PIN, OUTPUT);
  digitalWrite(LOAD_RELAY_PIN, LOW); // Start Load OFF

  // --- WiFi Connection ---
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Web Server Initialization ---
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("HTTP Server started.");
}

void loop() {
  server.handleClient(); // Handle incoming web requests

  // 1. Read Sensors
  currentVoltage = readVoltage();
  currentCurrent = readCurrent();
  
  // 2. Calculate Power (P = V * I)
  currentPower = currentVoltage * currentCurrent;
  
  // 3. Control Load
  controlLoad(); 
  
  Serial.printf("V: %.2f V | I: %.2f A | P: %.2f W | Load: %s\n", 
                currentVoltage, currentCurrent, currentPower, 
                loadConnected ? "ON" : "OFF");
  
  delay(5000); // Update metrics and webpage every 5 seconds
}
