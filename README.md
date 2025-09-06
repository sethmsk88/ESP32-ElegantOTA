# ESP32 ElegantOTA Project

This project demonstrates how to implement Over-The-Air (OTA) updates for ESP32 microcontrollers using the ElegantOTA library with AsyncWebServer.

## Overview

ElegantOTA provides a user-friendly web interface for uploading firmware updates to ESP32 devices over WiFi, eliminating the need for physical USB connections during development and deployment.

## Features

- **Web-based OTA Updates**: Upload new firmware through a web browser
- **Async Web Server**: Non-blocking web server implementation for better performance
- **Progress Monitoring**: Real-time OTA upload progress tracking
- **Status LED**: Visual feedback through built-in LED blinking
- **Callback Functions**: Custom actions during OTA start, progress, and completion

## Hardware Requirements

- ESP32 development board (configured for Adafruit Feather ESP32-S3)
- WiFi network connection
- USB cable for initial programming

## Software Dependencies

This project uses PlatformIO and includes the following libraries:

- **AsyncTCP**: Asynchronous TCP library for ESP32
- **ESPAsyncWebServer**: Async HTTP and WebSocket server
- **ElegantOTA**: Web-based OTA update interface

## Project Structure

```
├── platformio.ini          # PlatformIO configuration
├── src/
│   ├── main.cpp            # Main application entry point
│   ├── OTA.h               # OTA setup and WiFi configuration
│   └── arduino_secrets.h   # WiFi credentials (keep private!)
├── include/                # Header files directory
├── lib/                    # Local libraries
└── test/                   # Unit tests
```

## Setup Instructions

### 1. Configure WiFi Credentials

Edit `src/arduino_secrets.h` with your WiFi network details:

```cpp
#define SECRET_SSID "YourWiFiNetwork"
#define SECRET_PW "YourWiFiPassword"
```

**⚠️ Security Note**: Never commit WiFi credentials to version control. Consider adding `arduino_secrets.h` to `.gitignore`.

### 2. Hardware Configuration

The project is configured for the Adafruit Feather ESP32-S3, but can be adapted for other ESP32 boards by modifying the `board` setting in `platformio.ini`.

### 3. Build and Upload

Using PlatformIO:

```bash
# Build the project
pio run

# Upload to ESP32 (first time via USB)
pio run --target upload

# Monitor serial output
pio device monitor
```

## How It Works

### Main Application (`main.cpp`)

- Initializes serial communication at 115200 baud
- Sets up the built-in LED as an output
- Calls `setupOTA()` to initialize WiFi and web server
- Runs a simple LED blink pattern in the main loop
- Calls `ElegantOTA.loop()` to handle OTA functionality

### OTA Implementation (`OTA.h`)

1. **WiFi Connection**: Connects to the specified WiFi network
2. **Web Server Setup**: Creates an AsyncWebServer on port 80
3. **ElegantOTA Integration**: Initializes the OTA web interface
4. **Callback Functions**: Defines custom actions for OTA events:
   - `onOTAStart()`: Called when OTA update begins
   - `onOTAProgress()`: Called during upload (logged every second)
   - `onOTAEnd()`: Called when OTA update completes (success/failure)

## Using the OTA Interface

1. **Initial Setup**: Upload the firmware to ESP32 via USB cable
2. **Connect to WiFi**: The device automatically connects to your WiFi network
3. **Find IP Address**: Check the serial monitor for the assigned IP address
4. **Access Web Interface**: 
   - Navigate to `http://[ESP32_IP_ADDRESS]` for basic status
   - Navigate to `http://[ESP32_IP_ADDRESS]/update` for OTA interface
5. **Upload Firmware**: Select your compiled `.bin` file and upload

## OTA Update Process

1. Compile your updated code using PlatformIO
2. Locate the compiled binary (typically in `.pio/build/[environment]/firmware.bin`)
3. Open the ElegantOTA web interface in your browser
4. Select the firmware file and click upload
5. Monitor progress through the web interface and serial output
6. Device automatically reboots with new firmware

## Serial Monitor Output

The device provides detailed logging:

```
Connected to YourWiFiNetwork
IP address: 192.168.1.100
HTTP server started
OTA update started!
OTA Progress Current: 12345 bytes, Final: 67890 bytes
OTA update finished successfully!
```

## Troubleshooting

### Common Issues

- **WiFi Connection Failed**: Verify SSID and password in `arduino_secrets.h`
- **OTA Upload Fails**: Ensure the ESP32 is connected and accessible on the network
- **Web Interface Not Accessible**: Check the IP address in serial monitor output
- **Upload Timeout**: Try uploading smaller firmware files or check network stability

### Debug Tips

- Monitor serial output during OTA updates for error messages
- Verify the compiled binary file size and format
- Ensure sufficient flash memory for the new firmware
- Check network firewall settings if upload fails

## Security Considerations

- **WiFi Credentials**: Store credentials securely, avoid hardcoding in production
- **Network Security**: OTA interface is unprotected by default - consider adding authentication
- **Firmware Validation**: Implement firmware signature verification for production use

## Customization

### Adding Authentication

Consider implementing authentication for the OTA interface in production environments.

### Custom Web Pages

Extend the web server with additional routes:

```cpp
server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"running\"}");
});
```

### Advanced OTA Features

- Pre-update validation
- Rollback functionality
- Multiple partition support
- Encrypted firmware updates

## License

This project is based on the ElegantOTA library example code. Check individual library licenses for specific terms.

## References

- [ElegantOTA Documentation](https://docs.elegantota.pro)
- [ElegantOTA GitHub Repository](https://github.com/ayushsharma82/ElegantOTA)
- [PlatformIO Documentation](https://docs.platformio.org)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)

## Contributing

Feel free to submit issues and enhancement requests. When contributing:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

---

**Note**: This project is configured for development and testing. Additional security measures should be implemented for production deployments.
