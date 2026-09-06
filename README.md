# AI-Based Early Warning and Landslide Risk Monitoring System (NER)

## Overview
An IoT-based landslide early warning system simulated on Wokwi using ESP32.
Monitors soil movement (simulated) and environmental data (DHT22), applies a
5-point moving average trend-detection algorithm, and sends real-time alerts
via Telegram Bot API.

## Features
- 3-stage risk classification (Safe / Warning / Danger)
- Moving average algorithm for noise filtering & trend confirmation
- Real-time LCD status display
- Telegram bot instant alerts
- RTC-based event timestamping
- Push-button alert acknowledgment/silence
- DHT22 humidity/temperature monitoring

## Hardware (Simulated on Wokwi)
- ESP32
- LCD1602 (I2C)
- DS1307 RTC
- DHT22 Temperature & Humidity Sensor
- 3x LED (Green / Yellow / Red)
- Buzzer
- Push Button
- Potentiometer (soil movement simulation)

## How It Works
1. Soil movement value is auto-simulated (0-4095 range)
2. Last 5 readings are averaged (moving average) to filter noise
3. Risk is classified: Safe (<2000), Warning (2000-3000), Danger (>3000)
4. LED + buzzer indicate status; LCD shows live values
5. On Danger, a Telegram alert is sent (10-second cooldown to avoid spam)
6. Push button lets a user acknowledge/silence an active alert

## Setup Instructions
1. Open the project on [Wokwi](https://wokwi.com)
2. Replace `YOUR_BOT_TOKEN_HERE` with your Telegram bot token
3. Replace `YOUR_CHAT_ID_HERE` with your Telegram chat ID
4. Run the simulation

## Demo
[Add your Wokwi project share link here]

## Future Enhancements
- Real hardware deployment with actual soil/rain sensors
- Multi-zone sensor network for hillside coverage
- LoRa communication for remote areas without WiFi
- Machine learning-based prediction model
- Solar-powered field deployment

## Disclaimer
This is a simulated prototype built for educational purposes, demonstrating
IoT and embedded systems concepts. It has not been tested on real hardware
in a live landslide-prone environment.
