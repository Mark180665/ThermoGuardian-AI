#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <Adafruit_MAX3010x.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// ==================== DISPLAY SETUP ====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==================== SENSOR OBJECTS ====================
Adafruit_MLX90640 mlx;
Adafruit_MAX30105 max30102;
Adafruit_BME280 bme;

// ==================== PIN DEFINITIONS ====================
const int BUZZER_PIN = 23;

// ==================== GLOBAL VARIABLES ====================
float mlxFrame[32*24]; // MLX90640 frame buffer
float patientTemp = 0.0;
float ambientTemp = 0.0;
float humidity = 0.0;
float pressure = 0.0;
int heartRate = 0;
int spo2 = 0;

bool mlxFound = false;
bool maxFound = false;
bool bmeFound = false;
bool oledFound = false;

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 2000; // Update every 2 seconds

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ThermoGuardian AI - Sensor Testing ===");
  Serial.println("Initializing all sensors...");
  
  // Initialize I2C with faster clock for MLX90640
  Wire.begin(21, 22);
  Wire.setClock(1000000);
  
  // Initialize components
  initializeOLED();
  initializeBME280();
  initializeMLX90640();
  initializeMAX30102();
  initializeBuzzer();
  
  // Show initial status
  displayTestResults();
  
  Serial.println("\n=== Sensor Testing Ready ===");
  Serial.println("All sensors will display data every 2 seconds");
  Serial.println("Check OLED display for real-time readings");
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    readAllSensors();
    updateDisplay();
    printSerialData();
    
    // Test buzzer with a short beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    
    lastUpdate = currentMillis;
  }
}

// ==================== INITIALIZATION FUNCTIONS ====================
void initializeOLED() {
  Serial.print("Initializing OLED... ");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("FAILED");
    oledFound = false;
    return;
  }
  oledFound = true;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("ThermoGuardian AI");
  display.println("Sensor Test");
  display.display();
  Serial.println("SUCCESS");
  delay(1000);
}

void initializeBME280() {
  Serial.print("Initializing BME280... ");
  if (!bme.begin(0x76)) { // Try 0x76 first
    if (!bme.begin(0x77)) { // Try 0x77 if 0x76 fails
      Serial.println("FAILED - Not found");
      bmeFound = false;
      return;
    }
  }
  bmeFound = true;
  Serial.println("SUCCESS");
}

void initializeMLX90640() {
  Serial.print("Initializing MLX90640... ");
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("FAILED - Not found");
    mlxFound = false;
    return;
  }
  mlxFound = true;
  
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_2_HZ);
  Serial.println("SUCCESS");
}

void initializeMAX30102() {
  Serial.print("Initializing MAX30102... ");
  if (!max30102.begin()) {
    Serial.println("FAILED - Not found");
    maxFound = false;
    return;
  }
  maxFound = true;
  
  // Configure sensor settings
  max30102.setup(0x7F, 4, 2, 100, 411, 4096);
  Serial.println("SUCCESS");
}

void initializeBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Test buzzer on startup
  for(int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
  Serial.println("Buzzer tested");
}

// ==================== SENSOR READING FUNCTIONS ====================
void readAllSensors() {
  readBME280();
  readMLX90640();
  readMAX30102();
}

void readBME280() {
  if (bmeFound) {
    ambientTemp = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F; // Convert to hPa
  } else {
    ambientTemp = -999;
    humidity = -999;
    pressure = -999;
  }
}

void readMLX90640() {
  if (!mlxFound) {
    patientTemp = -999;
    return;
  }
  
  if (mlx.getFrame(mlxFrame) != 0) {
    Serial.println("Failed to read MLX90640 frame");
    patientTemp = -999;
    return;
  }
  
  // Find hottest spot in the frame (simplified forehead detection)
  float maxTemp = -100;
  for (int i = 0; i < 32*24; i++) {
    if (mlxFrame[i] > maxTemp) {
      maxTemp = mlxFrame[i];
    }
  }
  
  // Simple calibration (adjust based on your testing)
  patientTemp = maxTemp; // - 2.5; // Adjust this offset based on your testing
}

void readMAX30102() {
  if (!maxFound) {
    heartRate = -999;
    spo2 = -999;
    return;
  }
  
  // For testing, we'll use simulated data
  // In actual use, you would implement proper HR/SpO2 algorithms
  heartRate = random(65, 85); // Simulated heart rate
  spo2 = random(95, 99);     // Simulated SpO2
  
  // Note: Actual HR/SpO2 calculation requires collecting and processing
  // multiple samples over time. This is just for basic sensor testing.
}

// ==================== DISPLAY FUNCTIONS ====================
void updateDisplay() {
  if (!oledFound) return;
  
  display.clearDisplay();
  display.setCursor(0,0);
  
  // Header
  display.println("Sensor Test Mode");
  display.println("----------------");
  
  // MLX90640 Data
  display.print("Body: ");
  if (patientTemp == -999) display.println("FAIL");
  else {
    display.print(patientTemp, 1);
    display.println(" C");
  }
  
  // BME280 Data
  display.print("Amb:  ");
  if (ambientTemp == -999) display.println("FAIL");
  else {
    display.print(ambientTemp, 1);
    display.println(" C");
  }
  
  // MAX30102 Data
  display.print("HR:   ");
  if (heartRate == -999) display.println("FAIL");
  else {
    display.print(heartRate);
    display.println(" bpm");
  }
  
  display.print("SpO2: ");
  if (spo2 == -999) display.println("FAIL");
  else {
    display.print(spo2);
    display.println(" %");
  }
  
  // Status
  display.print("Status: ");
  int workingSensors = (mlxFound?1:0) + (bmeFound?1:0) + (maxFound?1:0);
  display.print(workingSensors);
  display.println("/3 OK");
  
  display.display();
}

void displayTestResults() {
  if (!oledFound) return;
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  
  display.println("=== SENSOR TEST ===");
  display.println();
  
  display.print("OLED: ");
  display.println(oledFound ? "OK" : "FAIL");
  
  display.print("MLX90640: ");
  display.println(mlxFound ? "OK" : "FAIL");
  
  display.print("BME280: ");
  display.println(bmeFound ? "OK" : "FAIL");
  
  display.print("MAX30102: ");
  display.println(maxFound ? "OK" : "FAIL");
  
  display.println();
  display.println("Starting readings...");
  
  display.display();
  delay(3000);
}

// ==================== SERIAL OUTPUT FUNCTIONS ====================
void printSerialData() {
  Serial.println("\n=== SENSOR READINGS ===");
  
  // MLX90640 Data
  Serial.print("MLX90640 - Patient Temp: ");
  if (patientTemp == -999) Serial.println("SENSOR NOT FOUND");
  else {
    Serial.print(patientTemp, 1);
    Serial.println(" °C");
    
    // Print a small thermal preview (4x4 downsampled)
    Serial.println("Thermal Preview (4x4):");
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        int idx = (y * 8 * 32) + (x * 8);
        Serial.print(mlxFrame[idx], 1);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
  
  // BME280 Data
  Serial.print("BME280 - Ambient Temp: ");
  if (ambientTemp == -999) Serial.println("SENSOR NOT FOUND");
  else {
    Serial.print(ambientTemp, 1);
    Serial.println(" °C");
  }
  
  Serial.print("BME280 - Humidity: ");
  if (humidity == -999) Serial.println("SENSOR NOT FOUND");
  else {
    Serial.print(humidity, 1);
    Serial.println(" %");
  }
  
  Serial.print("BME280 - Pressure: ");
  if (pressure == -999) Serial.println("SENSOR NOT FOUND");
  else {
    Serial.print(pressure, 1);
    Serial.println(" hPa");
  }
  
  // MAX30102 Data
  Serial.print("MAX30102 - Heart Rate: ");
  if (heartRate == -999) Serial.println("SENSOR NOT FOUND");
  else {
    Serial.print(heartRate);
    Serial.println(" BPM");
  }
  
  Serial.print("MAX30102 - SpO2: ");
  if (spo2 == -999) Serial.println("SENSOR NOT FOUND");
  else {
    Serial.print(spo2);
    Serial.println(" %");
  }
  
  Serial.println("======================");
}

// ==================== BUZZER TEST FUNCTION ====================
void testBuzzerPattern() {
  Serial.println("Testing buzzer patterns...");
  
  // Slow beep pattern
  for(int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }
  
  // Fast beep pattern (emergency)
  for(int i = 0; i < 10; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
