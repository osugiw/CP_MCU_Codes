# MCU docs

Status: In development
Assign: Sugiarto Wibowo, clawchuck s
Team: Documentation

# Sally: Wearable Device

<table border="0">
  <tr>
    <td width="50%" align="center">
      <img src="MCU%20docs/sally_logo.svg" width="50%" alt="Sally Logo">
    </td>
    <td width="50%" align="center">
      <img src="MCU%20docs/RiceStackedHoriz_Blue.png" width="70%" alt="Rice Logo">
    </td>
  </tr>
</table>

This project is a core component of the [cp_wearable_ai](https://github.com/osugiw/cp_wearable_ai.git) ecosystem. We are building a robust, everyday wearable designed to augment daily life by merging continuous audio intelligence with real-time biometric insights.

## 🚀 The Vision

Our goal is to provide a seamless "second brain" that lives as your necklace. By integrating continuous audio capture with biometric monitoring, the device doesn't just listen—it understands the physical and social context of your day to provide proactive support.

---

## 🛠️ How It Works

The system firmware is built on **FreeRTOS**, utilizing binary semaphores to synchronize high-priority hardware tasks and ensure reliable real-time performance.

**🔄 Execution Flow**

1. **Initialization:** Upon boot, the device configures the digital microphone, SD card interface, and wireless (Wi-Fi/BLE) peripherals.
2. **Transmission Priority:** The system checks the SD card for buffered audio. It attempts to upload files to the central server via Wi-Fi.
    1. *Data Integrity:* Files are only purged from local storage after a successful **server acknowledgment** is received.
3. **Interrupt Monitoring:** The system listens for user input via the tactile button array.
    1. **Configuration Mode:** Triggered via button interrupt; allows settings updates from the mobile app via **BLE.**
    2. **Recording Mode:** The default state if no interrupt is detected.

**🎙️ Audio Recording Specifications**
To balance audio clarity with low-power transmission, the device uses the following encoding profile:

| **Parameter** | **Specification** |
| --- | --- |
| **Interval** | 300 seconds (5 minutes) *— Subject to optimization* |
| **Sample Rate** | 16 kHz |
| **Channels** | Mono |
| **Format** | AAC |
| **Bitrate** | 32,000 bps (32 kbps) |

![EM_flowchart_main.png](MCU%20docs/EM_flowchart_main.png)

---

## ✨ Key Features

- **Low-Power Design:** Optimized hardware for all-day wearable use.
- **Biometric Integration:** Syncs audio data with heart rate or other vitals for holistic life-tracking.
- **Actionable Insights:** Converts hours of conversation into high-value summaries and tasks.

## 📋 Tech Stack

### **Hardware**

**🧠 Processing & Connectivity**

- **MCU:** ESP32-S3 (Xtensa® 32-bit LX7 dual-core processor).
- **Memory:** 8MB PSRAM and 8MB Flash memory.
- **Wireless:** Dual-mode support for **Wi-Fi** and **Bluetooth Low Energy (BLE)** for seamless data transmission to the Raspberry Pi server.

**⚡ Power & Interface**

- **Battery:** 400 mAh Lithium Polymer (LiPo) battery, optimized for all-day efficiency.
- **User Input:** A tactile button array for manual wireless connectivity toggles and manual recording triggers.

**🏥 Physiological Monitoring (Roadmap)**[!IMPORTANT]
**Status: In Development (Target Release: Fall 2026)**
We are currently integrating a multi-modal health sensor to provide continuous tracking of:

- **Heart Rate & HRV** (Heart Rate Variability)
- **Skin Conductance** (Electrodermal Activity)
- **Body Temperature**

![EM_HW.png](MCU%20docs/EM_HW.png)

### Software

The wearable software is built using the **Espressif IoT Development Framework (ESP-IDF)**, ensuring optimized performance for the ESP32-S3. The system leverages several specialized libraries to manage audio, storage, and networking:

### 🎙️ Audio & Storage Management

- **FatFS:** Manages file system operations for reading from and writing audio recordings to the microSD card.
- **SNTP (Simple Network Time Protocol):** Synchronizes timestamps via the network to ensure accurate, time-stamped file naming for all recordings.
- **ESP Audio Codec:** Handles the real-time conversion and encoding of **Pulse Density Modulation (PDM)** data from the digital microphone into **Advanced Audio Coding (AAC)** format.

### 🌐 Wireless Networking

- **lwIP (Lightweight IP):** Manages the TCP/IP stack for robust network connectivity.
- **ESP HTTP Client:** Facilitates secure and efficient communication with the central Raspberry Pi server.
- **Bluedroid:** Powers the Bluetooth peripheral stack, enabling the device to communicate with the mobile application for configuration.

---

### 🛠️ Development Environment

- **Framework:** ESP-IDF (v5.x recommended)
- **RTOS:** FreeRTOS (Integrated with ESP-IDF)
- **Language:** C/C++

![EM_SW stack.png](MCU%20docs/EM_SW_stack.png)

---

## 🏗️ Getting Started

### Prerequisites

- A configured Raspberry Pi server (see [Server Setup Guide](https://www.google.com/search?q=%23)).
- Compatible wearable hardware.
- ESP-IDF v5.x

### Installation

```jsx
git clone https://github.com/osugiw/CP_MCU_Codes.git
cd [your-repo-name]
```

### Configuration

Some configuration needs to be changed before the software can run. Change the WiFi SSID and Password to your network, it can be changed in **main.h**

![image.png](MCU%20docs/image.png)

### Compile and Flash

Make sure to choose ESP32S3 as the target device and click build project.

![image.png](MCU%20docs/image%201.png)

Upon successful built, plug your Hardware and click flash device.

### Monitor/Debug

Click monitor to see device output and see the debug messages.