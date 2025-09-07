/*
  -----------------------
  ElegantOTA with WiFiManager - Dynamic WiFi Configuration
  -----------------------

  This implementation uses WiFiManager to allow users to configure WiFi credentials
  through a captive portal interface, eliminating the need for hardcoded credentials.

  Features:
  - Automatic captive portal when no WiFi credentials are saved
  - Web-based credential configuration
  - Persistent credential storage
  - Fallback configuration mode via button press
  - Seamless integration with ElegantOTA

  Github: https://github.com/ayushsharma82/ElegantOTA
  WiFiManager: https://github.com/tzapu/WiFiManager

  Works with ESP32

*/
#include <WiFi.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// WiFiManager instance for handling dynamic WiFi configuration
WiFiManager wifiManager;

AsyncWebServer server(8080);

unsigned long ota_progress_millis = 0;

// Flag to indicate if configuration portal should be started
bool shouldStartConfigPortal = false;

// Forward declarations
void setupWebServerAndOTA();

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code -here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();+
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

/**
 * Request WiFi configuration portal to be started
 * 
 * This function can be called from main.cpp when the configuration button
 * is pressed to trigger the WiFi setup portal.
 */
void startConfigPortal() {
  shouldStartConfigPortal = true;
  Serial.println("CONFIG: Configuration portal requested");
}

/**
 * Configure WiFiManager settings and behavior
 * 
 * Sets up WiFiManager with appropriate timeouts, custom parameters,
 * and configuration portal behavior.
 */
void configureWiFiManager() {
  // Set configuration portal timeout (3 minutes)
  wifiManager.setConfigPortalTimeout(180);
  
  // Set connection timeout (30 seconds)
  wifiManager.setConnectTimeout(30);
  
  // Set device hostname and access point name - using autoConnect parameter instead
  // The AP name will be set when calling autoConnect() or startConfigPortal()
  
  // Set minimum signal quality (0-100%)
  wifiManager.setMinimumSignalQuality(20);
  
  // Show password in configuration portal (set to false for security)
  wifiManager.setShowPassword(true);
  
  // Set debug output
  wifiManager.setDebugOutput(true);
  
  Serial.println("CONFIG: WiFiManager configured");
}

void setupOTA() {
  Serial.println("SETUP: Configuring WiFiManager...");
  configureWiFiManager();
  
  // Try to connect with saved credentials first
  Serial.println("SETUP: Attempting to connect with saved credentials...");
  
  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  
  // Try auto-connecting with saved credentials (non-blocking)
  wifiManager.setConfigPortalBlocking(false);
  
  // Attempt connection with saved credentials
  bool connected = wifiManager.autoConnect("ESP32-ElegantOTA-Config");
  
  if (connected) {
    Serial.println("");
    Serial.println("WIFI: Connected successfully!");
    Serial.print("WIFI: Connected to: ");
    Serial.println(WiFi.SSID());
    Serial.print("WIFI: IP address: ");
    Serial.println(WiFi.localIP());
    
    // Start the web server and OTA
    setupWebServerAndOTA();
  } else {
    Serial.println("WIFI: No saved credentials or connection failed");
    Serial.println("WIFI: Starting in configuration mode...");
    Serial.println("WIFI: Connect to 'ESP32-ElegantOTA-Config' WiFi network to configure");
  }
}

/**
 * Setup web server and ElegantOTA functionality
 * 
 * Called after successful WiFi connection to initialize the web server
 * and ElegantOTA components.
 */
void setupWebServerAndOTA() {
  // Basic web server route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><title>ESP32 ElegantOTA</title></head><body>";
    html += "<h1>ESP32 ElegantOTA Device</h1>";
    html += "<p><strong>WiFi Status:</strong> Connected</p>";
    html += "<p><strong>SSID:</strong> " + WiFi.SSID() + "</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
    html += "<hr>";
    html += "<p><a href='/update'>OTA Update Portal</a></p>";
    html += "<p><em>Device is ready for Over-The-Air updates</em></p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);
  
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  // Start the web server
  server.begin();
  Serial.println("HTTP: Web server started");
  Serial.println("OTA: ElegantOTA ready for updates");
}

/**
 * Handle WiFiManager operations and configuration portal
 * 
 * This function should be called from the main loop to handle:
 * - Configuration portal requests
 * - WiFi connection monitoring
 * - Automatic reconnection attempts
 * 
 * Call this function regularly from loop() for proper operation.
 */
void handleOTA() {
  // Process WiFiManager operations
  wifiManager.process();
  
  // Check if configuration portal should be started
  if (shouldStartConfigPortal) {
    shouldStartConfigPortal = false;
    
    Serial.println("CONFIG: Starting configuration portal...");
    Serial.println("CONFIG: Connect to 'ESP32-ElegantOTA-Config' WiFi network");
    Serial.println("CONFIG: Portal will timeout after 3 minutes");
    
    // Start configuration portal (blocking)
    wifiManager.setConfigPortalBlocking(true);
    bool result = wifiManager.startConfigPortal("ESP32-ElegantOTA-Config");
    
    if (result) {
      Serial.println("CONFIG: WiFi configured successfully!");
      Serial.print("CONFIG: Connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("CONFIG: IP address: ");
      Serial.println(WiFi.localIP());
      
      // Setup web server and OTA after successful configuration
      setupWebServerAndOTA();
    } else {
      Serial.println("CONFIG: Configuration portal timed out or failed");
    }
    
    // Reset to non-blocking mode
    wifiManager.setConfigPortalBlocking(false);
  }
  
  // Monitor WiFi connection status
  static unsigned long lastWiFiCheck = 0;
  static bool wasConnected = false;
  
  if (millis() - lastWiFiCheck >= 5000) { // Check every 5 seconds
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    
    if (wasConnected && !isConnected) {
      Serial.println("WIFI: Connection lost - attempting reconnection...");
    } else if (!wasConnected && isConnected) {
      Serial.println("WIFI: Connection restored!");
      Serial.print("WIFI: IP address: ");
      Serial.println(WiFi.localIP());
    }
    
    wasConnected = isConnected;
    lastWiFiCheck = millis();
  }
}
