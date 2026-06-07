
# Smart Dustbin — ESP32 IoT Project

> Automatic lid control using motion detection and sound/voice trigger

## Demo
- Walk within 10cm → lid opens automatically
- Clap or speak → lid opens for 3 seconds then closes
- OLED shows real-time distance and sound level

## Hardware Used
| Component | Model | Purpose |
|---|---|---|
| MCU | ESP32 Dev Module | Main controller |
| Mic | MD0220 Sound Sensor | Voice/clap detection |
| Ultrasonic | HC-SR04 | Motion/distance detection |
| Servo | SG90 | Lid mechanism |
| Display | OLED SSD1306 I2C | Status display |

## Wiring
| Component | Pin | ESP32 GPIO |
|---|---|---|
| MD0220 | AO | 34 |
| MD0220 | DO | 35 |
| HC-SR04 | TRIG | 18 |
| HC-SR04 | ECHO | 19 |
| OLED | SDA | 21 |
| OLED | SCL | 22 |
| SG90 | Signal | 26 |

## Setup
1. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/)
2. Clone this repo: `git clone https://github.com/umayanga23/smart_dushbin.git`
3. Open folder in PlatformIO
4. Connect ESP32 via USB
5. Click Upload

## Built With
- PlatformIO + Arduino framework
- Adafruit SSD1306 library
- ESP32Servo library

## Author
**Umayanga** — [GitHub](https://github.com/umayanga23) · [LinkedIn](https://linkedin.com/in/YOUR-PROFILE)
EOF

