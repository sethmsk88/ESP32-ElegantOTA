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

const int OTA_SERVER_PORT = 8080;

AsyncWebServer server(OTA_SERVER_PORT);

unsigned long ota_progress_millis = 0;

// Flag to indicate if configuration portal should be started
bool shouldStartConfigPortal = false;

// Forward declarations
void setupWebServerAndOTA();
void startWiFiConnection();

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
    Serial.println("Rebooting device in 3 seconds...");
    delay(3000); // Give time to see the message
    ESP.restart(); // Automatically reboot the device
  } else {
    Serial.println("There was an error during OTA update!");
    Serial.println("Device will continue running with previous firmware");
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
  Serial.println("CONFIG: Button pressed - Starting WiFi connection process");
  Serial.println("WIFI: Starting configuration portal...");
  // Set the flag to start config portal in the main loop
  shouldStartConfigPortal = true;

  startWiFiConnection();
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
  
  // Set connection timeout (15 seconds - reduced from 30)
  wifiManager.setConnectTimeout(15);
  
  // Set minimum signal quality (0-100%)
  wifiManager.setMinimumSignalQuality(20);
  
  // Show password in configuration portal (set to false for security)
  wifiManager.setShowPassword(true);
  
  // Set debug output
  wifiManager.setDebugOutput(true);
  
  // Optimize WiFi settings for faster AP startup
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  wifiManager.setWiFiAutoReconnect(false); // Don't auto-reconnect during portal
  wifiManager.setCleanConnect(true); // Clean previous WiFi connection before starting AP
  
  // Don't auto-close the portal after successful connection
  // This allows users to see the success message and access other menu items
  wifiManager.setBreakAfterConfig(false);
  
  // Configure which menu items (buttons) to show
  // Available options: "wifi", "wifinoscan", "info", "param", "close", "restart", "exit", "erase", "update"
  std::vector<const char *> menu = {
    "wifi",     // Configure WiFi (main configuration page)
    "info",     // Device information page
    "restart",  // Restart ESP32 button
    "erase"     // Erase WiFi credentials button (factory reset)
  };
  wifiManager.setMenu(menu);
  
  // Add custom HTML to show device information
  String customHTML = "<div style='text-align:center; margin: 20px; padding: 15px; background-color: #f0f8ff; border-radius: 10px; border: 2px solid #4CAF50;'>";
  customHTML += "<h3 style='color: #2E8B57; margin: 0 0 10px 0;'>ESP32 ElegantOTA Configuration</h3>";
  customHTML += "</div>";
  
  // Set the custom HTML (this appears at the top of the config page)
  wifiManager.setCustomHeadElement(customHTML.c_str());
  
  // Add a save config callback to show connection details
  wifiManager.setSaveConfigCallback([]() {
    Serial.println("CONFIG: WiFi credentials saved successfully!");
  });
  
  // Add a callback for when WiFi connects during config portal
  wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
    Serial.println("CONFIG: Configuration portal started");
    Serial.print("CONFIG: Connect to WiFi network: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
    Serial.println("CONFIG: Portal will timeout after 3 minutes");
  });
  
  Serial.println("CONFIG: WiFiManager configured");
}

void setupOTA() {
  Serial.println("SETUP: Configuring WiFiManager...");
  configureWiFiManager();
  
  Serial.println("SETUP: WiFiManager configured and ready");
  Serial.println("SETUP: Device started - Hold config button for 3 seconds to start WiFi configuration");
  Serial.println("SETUP: No automatic WiFi connection will be attempted");
}

/**
 * Attempt WiFi connection with saved credentials or start configuration portal
 * 
 * This function is called when the user presses the configuration button.
 * It will try to connect with saved credentials first, and if that fails,
 * it will start the configuration portal.
 */
void startWiFiConnection() {
  Serial.println("WIFI: Starting WiFi connection process...");
  
  // Ensure WiFi is properly disconnected and cleaned up first
  WiFi.disconnect(true);
  delay(100);
  
  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // First, try to connect with saved credentials without starting a portal
  Serial.println("WIFI: Attempting to connect with saved credentials...");
  
  // Check if we have saved credentials
  if (wifiManager.getWiFiIsSaved()) {
    Serial.println("WIFI: Found saved credentials, attempting connection...");
    WiFi.begin(); // Use saved credentials
    
    // Wait up to 10 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WIFI: Connected successfully with saved credentials!");
      Serial.print("WIFI: Connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("WIFI: IP address: ");
      Serial.println(WiFi.localIP());
      
      // Start the web server and OTA
      setupWebServerAndOTA();
      return; // Successfully connected, exit function
    } else {
      Serial.println("");
      Serial.println("WIFI: Failed to connect with saved credentials");
    }
  } else {
    Serial.println("WIFI: No saved credentials found");
  }
  
  // Clean up WiFi before starting AP mode
  Serial.println("WIFI: Cleaning up WiFi connection...");
  WiFi.disconnect(true);
  delay(500); // Give more time for cleanup
  
  // If we get here, either no saved credentials or connection failed
  Serial.println("WIFI: Starting configuration portal...");
  shouldStartConfigPortal = true; // Let the main loop handle the portal
}

/**
 * Setup web server and ElegantOTA functionality
 * 
 * Called after successful WiFi connection to initialize the web server
 * and ElegantOTA components.
 */
void setupWebServerAndOTA() {
  // Basic web server route - automatically redirect to OTA update page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/update");
  });

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);
  
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  // Start the web server
  server.begin();
  Serial.println("OTA Web server started");
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
    
    Serial.println("CONFIG: Starting WiFi configuration portal...");
    
    // Ensure portal stays open after successful connection
    wifiManager.setBreakAfterConfig(false);
    
    // Add custom HTML with connection success message that will show IP
    String successHTML = "<div style='text-align:center; margin: 20px; padding: 15px; background-color: #d4edda; border-radius: 10px; border: 2px solid #28a745;'>";
    successHTML += "<h3 style='color: #155724; margin: 0 0 10px 0;'>✅ Connection Successful!</h3>";
    successHTML += "<p style='margin: 5px 0; font-size: 16px;'><strong>Your ESP32 is now online!</strong></p>";
    successHTML += "<p style='margin: 5px 0; font-size: 14px;'>Portal will remain open until 3-minute timeout</p>";
    successHTML += "<p style='margin: 5px 0; font-size: 12px; color: #666;'>You can now use other menu options or wait for automatic timeout</p>";
    // successHTML += "<p style='margin: 5px 0; font-size: 14px;'>OTA Update Portal will be available at:<br />";
    // successHTML += "<a href='http://" + WiFi.localIP().toString() + ":" + String(OTA_SERVER_PORT) + "/update' target='_blank'>http://" + WiFi.localIP().toString() + ":" + String(OTA_SERVER_PORT) + "/update</a></p>";
    successHTML += "</div>";
    
    // Set the success page HTML
    wifiManager.setCustomHeadElement(successHTML.c_str());
    
    // Start configuration portal (blocking)
    wifiManager.setConfigPortalBlocking(true);
    bool result = wifiManager.startConfigPortal("LL-MorphStaff");
    
    // Portal has finished (either timed out, user exited, or error occurred)
    Serial.println("CONFIG: Configuration portal has ended");
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("CONFIG: WiFi configured successfully!");
      Serial.print("CONFIG: Connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("CONFIG: IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("=========================================");
      Serial.println("OTA UPDATE PORTAL READY!");
      Serial.printf("Access from browser: http://%s:%s\n", WiFi.localIP().toString().c_str(), String(OTA_SERVER_PORT));
      Serial.printf("Direct OTA link: http://%s:%s/update\n", WiFi.localIP().toString().c_str(), String(OTA_SERVER_PORT));
      Serial.println("=========================================");
      
      // Setup web server and OTA after portal ends and we have connection
      setupWebServerAndOTA();
    } else if (result) {
      Serial.println("CONFIG: Configuration portal timed out - no connection established");
    } else {
      Serial.println("CONFIG: Configuration portal failed or was cancelled");
    }
    
    // Reset to non-blocking mode
    // wifiManager.setConfigPortalBlocking(false);
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
