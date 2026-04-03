# ESP32 Weather Monitoring System

This project is an ESP32-based IoT weather monitoring system that reads environmental data from multiple sensors and shows the results on a local web dashboard over Wi-Fi.

## Features

- Temperature monitoring using DHT11
- Humidity monitoring using DHT11
- Atmospheric pressure monitoring using BMP180
- Air quality indication using MQ135
- Rain detection using a rain sensor
- Local web server hosted on ESP32
- Live browser dashboard with auto-refreshing data
- JSON data endpoint for sensor values

## Hardware Used

- ESP32 development board
- DHT11 temperature and humidity sensor
- BMP180 pressure sensor
- MQ135 gas sensor
- Rain sensor module
- Jumper wires
- Breadboard
- Wi-Fi network

## Pin Configuration

| Sensor | ESP32 Pin |
|--------|-----------|
| Rain Sensor | GPIO14 |
| MQ135 Analog Output | GPIO34 |
| DHT11 Data Pin | GPIO4 |
| BMP180 | I2C |

## Libraries Used

- WiFi.h
- Adafruit_BMP085.h
- Adafruit_Sensor.h
- DHT.h
- DHT_U.h

## How It Works

The ESP32 reads data from the connected sensors every few seconds and stores the values in variables. It also runs a local web server on port 80. When a user connects to the ESP32 IP address through a browser, the board serves a dashboard page showing live weather data.

The dashboard displays:

- Temperature
- Humidity
- Pressure
- Air quality indicator
- Rainfall status

A JSON endpoint is also available at:

/data

This allows the webpage to fetch updated sensor data and display it dynamically.

## Project Structure

```text
ESP32_Weather_Station_Code.ino
README.md
