# 🌱 Plantify AI

**Plantify AI** is an IoT and AI-based smart agriculture system that monitors plant health in real-time and detects plant diseases using image processing and machine learning.

---

## 🚀 Project Overview

Plantify AI combines environmental sensors, ESP32 microcontrollers, and AI-based image classification to help farmers monitor plant conditions and detect diseases early.

The system measures environmental parameters, captures plant images, identifies plant types, detects diseases, and provides treatment recommendations.

---

## 🔍 Features

* 🌡 Real-time Temperature Monitoring
* 💧 Humidity Detection
* 🌱 Soil Moisture Monitoring
* 📸 Plant Image Capture (ESP32-CAM)
* 🌳 Plant/Tree Identification
* 🦠 Disease Detection using AI
* 💊 Treatment & Prevention Suggestions
* 🌐 Web-based Interface
* 🗄 Database Storage (PHP & MySQL)

---

## 🛠 Technologies Used

* ESP32 & ESP32-CAM
* DHT11 Sensors
* Soil Moisture Sensor
* Wi-Fi Communication
* PHP & MySQL Database
* HTML, CSS, JavaScript
* Machine Learning (Image Classification Model)

---

## 🎯 Objective

The main goal of Plantify AI is to provide a low-cost, intelligent farming assistant that helps farmers detect plant diseases early, reduce crop loss, and improve agricultural productivity using smart technology.

---

## 🔌 Circuit Diagram

![Plantify AI Circuit](https://github.com/FatemaYesminMaroya/Plantify-AI/blob/790d2a553eacf9d50990d6839ff5cfcebffce154/circuit_image.png)

---

## 📌 Circuit RM Code (Pin Mapping)

| Component       | ESP32 Pin | Notes                  |
|-----------------|-----------|------------------------|
| DHT11 Sensor 1  | GPIO 13   | Connected with 10k Ω resistor |
| DHT11 Sensor 2  | GPIO 14   | Connected with 10k Ω resistor |
| Soil Sensor     | GPIO 32   | Analog Input           |
| Servo 1         | GPIO 4    | External 5V supply     |
| Servo 2         | GPIO 5    | External 5V supply     |
| Motor IN1       | GPIO 26   | L298N Driver           |
| Motor IN2       | GPIO 25   | L298N Driver           |
| Motor IN3       | GPIO 33   | L298N Driver           |
| Motor IN4       | GPIO 27   | L298N Driver           |

---

