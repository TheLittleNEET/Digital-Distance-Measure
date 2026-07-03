#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin ultrasonic
#define TRIG_PIN 26
#define ECHO_PIN 33

// Pin sensor tegangan baterai
#define BATTERY_PIN 34

// Ukuran OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variabel global
float distanceCM = 0;
int batteryPercent = 0;

// WDT Variabel
float lastDistance = -999;
unsigned long lastChangeTime = 0;
const unsigned long WDT_TIMEOUT = 20000; // 20 detik

void ultrasonicTask(void *pvParameters) {
  while (1) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms
    if (duration == 0) {
      distanceCM = -1;
    } else {
      distanceCM = duration * 0.0343 / 2;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void batteryTask(void *pvParameters) {
  while (1) {
    int raw = analogRead(BATTERY_PIN);
    float voltage = (raw / 4095.0) * 3.3 * 2.0;
    batteryPercent = map(voltage * 100, 330, 420, 0, 100);
    batteryPercent = constrain(batteryPercent, 0, 100);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void displayAndWDTTask(void *pvParameters) {
  lastChangeTime = millis(); // Initialize WDT timer
  while (1) {
    // Watchdog Timer: Reset timer if distance changes
    if (abs(distanceCM - lastDistance) > 0.1) {
      lastChangeTime = millis();
      lastDistance = distanceCM;
    }
    
    // If no change for 20 seconds -> "shut down system"
    if (millis() - lastChangeTime > WDT_TIMEOUT) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Sistem Tidak Responsif");
      display.println("Mematikan...");
      display.display();
      // Infinite loop -> shut down system (can be replaced with deep sleep)
      while (1);
    } 
      
    // Display data
    display.clearDisplay();
    display.setCursor(0, 0);
    if (distanceCM < 0) {
      display.println("Jarak: Out of range");
    } else {
      display.print("Jarak: ");
      display.print(distanceCM, 1);
      display.println(" cm");
    }
    display.print("Baterai: ");
    display.print(batteryPercent);
    display.println(" %");
    display.display();
    vTaskDelay(pdMS_TO_TICKS(500)); // Delay for display update
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal inisialisasi OLED"));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Starting...");
  display.display();
  delay(1000);
  
  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(ultrasonicTask, "Ultrasonic Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(batteryTask, "Battery Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(displayAndWDTTask, "Display & WDT Task", 4096, NULL, 1, NULL, 0); // Increased stack size for display task
} 

void loop() {
// Kosong karena menggunakan rtos
}
