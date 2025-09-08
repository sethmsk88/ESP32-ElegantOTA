/*
  ESP32 ElegantOTA Main Application
  
  This is the main application file that handles:
  - Hardware button monitoring for WiFi configuration trigger
  - LED status indication (heartbeat blink)
  - Serial output for debugging and system status
  - Integration with non-blocking OTA/WiFi management system
  
  The application runs a continuous loop that doesn't block for WiFi operations,
  allowing the main functionality to continue regardless of network status.
*/

#include <Arduino.h>
#include "OTA.h"

// Button configuration - Hardware button to trigger WiFi configuration
const int CONFIG_BUTTON_PIN = 11;                   // GPIO pin for configuration button (active low)
const unsigned long BUTTON_PRESS_TIME = 3000;     // Required hold time (3 seconds) to trigger config

// Button state variables - Used for button debouncing and timing
unsigned long buttonPressStart = 0;  // Timestamp when button was first pressed
bool buttonPressed = false;          // Current button state tracking

/**
 * Check hardware button state and trigger actions based on press duration
 * 
 * This function implements button debouncing and timing logic:
 * - Detects button press/release transitions
 * - Differentiates between a short "press" and a long "hold"
 * - Triggers WiFi configuration portal if button is held for 3+ seconds
 * - Triggers a different action for a short press and release
 * 
 * Called continuously from main loop for responsive button detection.
 */
void checkButton() {
  bool currentButtonState = digitalRead(CONFIG_BUTTON_PIN) == LOW; // Active low button
  
  if (currentButtonState && !buttonPressed) {
    // Button transition: not pressed -> pressed
    buttonPressed = true;
    buttonPressStart = millis();  // Record when button press started
    Serial.println(F("DEBUG: Config button press detected..."));
  } else if (!currentButtonState && buttonPressed) {
    // Button transition: pressed -> not pressed (released)
    buttonPressed = false;
    unsigned long pressDuration = millis() - buttonPressStart;
    
    // Action for a "button hold"
    if (pressDuration >= BUTTON_PRESS_TIME) {
      Serial.printf("DEBUG: Button held for %lu ms - Starting WiFi config portal\n", pressDuration);
      startConfigPortal(); // Trigger WiFiManager configuration portal
    
    // Action for a "button press" (and release)
    } else if (pressDuration > 50) { // Debounce threshold of 50ms
      Serial.println(F("DEBUG: Button clicked, disabling WiFi services..."));
      disableWiFi(); // Call the new function to shut down WiFi
    }
  }
}

/**
 * LED heartbeat pattern and system status display (non-blocking)
 * 
 * This function provides visual confirmation that the main loop is running
 * and displays system status information periodically:
 * - LED blinks every second (1 second on, 1 second off)
 * - Status display every 2 seconds showing counter and WiFi connectivity
 * Uses static variables to maintain state between function calls.
 */
void heartbeat() {
  static unsigned long lastLedTime = 0;
  static bool ledState = false;
  
  // LED heartbeat - blink every 1 second
  if (millis() - lastLedTime >= 1000) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    lastLedTime = millis();
  }
  
  // Application status tracking and display
  static unsigned long counter = 0;        // Simple counter to show system is running
  static unsigned long lastCounterTime = 0; // Timestamp for periodic counter display
  
  // Display system status every 2 seconds including WiFi connectivity
  if (millis() - lastCounterTime >= 2000) {
    String wifiStatus;
    if (WiFi.status() == WL_CONNECTED) {
      wifiStatus = "Connected (" + WiFi.SSID() + " - " + WiFi.localIP().toString() + ")";
    } else {
      wifiStatus = "Disconnected";
    }
    Serial.printf("Counter: %lu (WiFi: %s)\n", counter, wifiStatus.c_str());
    counter++;
    lastCounterTime = millis();
  }
}

/**
 * System initialization and setup
 * 
 * Initializes all hardware and software components needed for the application:
 * - Serial communication for debugging
 * - Built-in LED for status indication
 * - Configuration button with pull-up resistor
 * - OTA/WiFi management system
 * 
 * This runs once at startup before entering the main loop.
 */
void setup(void) {
  Serial.begin(115200);           // Initialize serial communication at 115200 baud
  pinMode(LED_BUILTIN, OUTPUT);   // Configure built-in LED as output for status indication

  delay(1000); // Give time for serial monitor to initialize and connect

  // Configure button pin with internal pull-up resistor (active-low button)
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  Serial.printf("Config button on pin %d (hold for %d seconds)\n", CONFIG_BUTTON_PIN, BUTTON_PRESS_TIME / 1000);

  Serial.println(F("=== ESP32 Starting Up ==="));
  Serial.println(F("About to call setupOTA()..."));
  
  // Initialize the non-blocking OTA/WiFi management system
  setupOTA();
  
  Serial.println(F("setupOTA() completed successfully!"));
  Serial.println(F("=== Entering Main Loop ==="));
}

/**
 * Main application loop - runs continuously after setup()
 * 
 * This loop handles multiple concurrent tasks in a non-blocking manner:
 * 1. Button monitoring - Check for configuration button presses
 * 2. OTA/WiFi management - Handle network operations and web server
 * 3. Status display - Show system status and connectivity info
 * 4. LED heartbeat - Visual indication that system is running
 * 
 * The loop is designed to be non-blocking, meaning WiFi/network operations
 * don't prevent the main application logic from running continuously.
 */
void loop(void) {
  // Monitor hardware button for WiFi configuration requests
  checkButton();
  
  // Handle WiFiManager operations and configuration portal
  handleOTA();

  // LED heartbeat pattern and status display
  // This provides visual confirmation that the main loop is running
  heartbeat();
}
