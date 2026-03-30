# ESP32-CAM Tank RC Vehicle

**Personalized build using DRV8833 motor driver**

Esp32 controlling tracked vehicle while streaming video via HTTP.

## Setup Instructions

### Initial Configuration
1. **WiFi Credentials** - Edit `ESP32CAM_RCTANK.ino`:
   - Line: `const char* ssid = "YOUR_SSID";` 
   - Line: `const char* password = "YOUR_PASSWORD";`

2. **Serial Monitor** - Open after upload to see:
   - WiFi connection status
   - Local IP address to access camera
   - Connection instructions

## Wiring

### Motor Driver: DRV8833 (Dual Motor Driver)
The DRV8833 provides efficient bidirectional control for two motors:

```
DRV8833 Pin Connections:
├─ IN1 (Motor A PWM)    → GPIO 12  ← Controls LEFT track speed
├─ IN2 (Motor A DIR)    → GPIO 13  ← Controls LEFT track direction
├─ IN3 (Motor B PWM)    → GPIO 14  ← Controls RIGHT track speed
└─ IN4 (Motor B DIR)    → GPIO 15  ← Controls RIGHT track direction

GND connections required on: DRV8833 GND pins
Power: 3.3V-11V supply (recommended 7.2V for good torque)
```

### DRV8833 Control Logic
- **Forward**: Both DIR pins HIGH, both PWM pins at speed value
- **Reverse**: Both DIR pins LOW, both PWM pins at speed value  
- **Turn**: Reduce speed on inside track (left or right)
- **Stop**: Both PWM values to 0

### Camera: ESP32-CAM (AI-Thinker Module)
See pinout diagram: `ESP32-CAM-pinout-new.png`

### Optional Servo (Turret/Launcher)
- GPIO 2: Servo control (50Hz PWM)
- Range: 325-650 (mapped to servo positions)

### LED Flash
- GPIO 4: LED control

## Camera Settings

### WiFi Issues
If experiencing streaming problems or low WiFi range, try adjusting xclk frequency in setup():
```cpp
config.xclk_freq_hz = 16000000;  // Default 20MHz, reduce if unstable
```

## Control Interface

Access via web browser: `http://<ESP32_LOCAL_IP>/`

### Control Options
- **Arrow Buttons**: Forward, Backward, Turn Left, Turn Right
- **Speed Slider**: 0-255 motor PWM
- **Servo Slider**: Turret/launcher position  
- **Stream**: Real-time video feed
- **Quality**: JPEG compression level
- **Resolution**: Frame size (higher = more detail, less FPS)
- **Flash**: LED brightness
- **No Stop**: Toggles continuous movement vs commands

## Build Configuration

### Camera Model
Currently configured for: **AI-Thinker Module**

To use different camera:
1. Edit `ESP32CAM_RCTANK.ino` line 18
2. Uncomment desired model (#define CAMERA_MODEL_*)
3. Re-upload

### Board Settings (Arduino IDE)
- **Board**: ESP32 Wrover Module
- **PSRAM**: Enabled (required for camera streaming)
- **Upload Speed**: 921600

## Power Considerations

### Supply Voltage
- **DRV8833**: 3.3V-11V input recommended (7.2V ideal)
- **ESP32-CAM**: 5V USB or regulated 3.3V
- **Motors**: Match supply voltage (7.2V-11.1V LiPo common)

### Current Draw
- ESP32-CAM: ~200mA (idle) / ~500mA (streaming)
- DRV8833 + Motors: Depends on motor stall current
- **Recommended Power**: 5000mAh+ battery for 2-3 hour runtime

## Performance Tuning

### PWM Frequency
DRV8833 runs at 5kHz - optimal balance between motor response and noise.

### Motor Commands
1 = Forward  
2 = Turn Left  
3 = Stop  
4 = Turn Right  
5 = Backward  

Sent via HTTP: `http://<IP>/control?var=car&val=<1-5>`

## Video
[Build & Operation Video](https://youtu.be/qUAGnk382mc)

---

**Last Modified**: Adapted for DRV8833, cleaned up configuration
