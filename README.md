
# STM32_LoRa_Communication_IOT_Data_Logging_System

## Overview

This project is a collection of scripts and firmware designed for secure data transfer, device firmware updates, and bootloader functionalities for embedded systems. Below are detailed descriptions of each script and their purposes.

## Contents

1. [Updater Server Script](#Updater_Server_Script)
2. [NTP Server Script](#ntp-server-script)
3. [Binary File Encryptor](#binary-file-encryptor)
4. [Bootloader Firmware](#bootloader-firmware)
5. [Main Firmware](#main-firmware)

## Updater Server Script

### Description

The Updater Server Script is a Python-based server that handles firmware update requests from connected devices. It verifies authentication credentials, checks the software version, and sends the update data in chunks.

### Key Features

- **Authentication:** Requests username and password from the connected client.
- **Version Check:** Compares the client's software version with the available version.
- **Data Transfer:** Sends the update file in specified line chunks with error handling and retry mechanisms.

### Configuration

- `lines_num`: Number of lines sent in one data chunk.
- `try_num`: Number of retries for failed transmissions.
- `end_of_lines`: Identifier for the end of a data chunk.

### Usage

1. Select the update file via the file dialog.
2. The script will start listening for incoming connections and handle updates based on client requests.

```python
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ip = socket.gethostbyname("192.168.200.40")
port = 2841
sock.bind((ip, port))
sock.listen(1000)
```

## NTP Server Script

### Description

The NTP Server Script is a Python-based implementation that requests the current time from an NTP server. It is used for synchronizing time across devices.

### Key Features

- **NTP Request:** Sends a request to the specified NTP server to fetch the current time.
- **Time Parsing:** Converts the NTP response to a human-readable format.

### Usage

1. Specify the NTP server address.
2. Run the script to get the current time from the server.

```python
print(RequestTimefromNtp('0.ir.pool.ntp.org'))
```

## Binary File Encryptor

### Description

The Binary File Encryptor is a C-based tool designed to encrypt and decrypt binary files using the AES encryption standard. It provides data protection for sensitive information.

### Key Features

- **AES Encryption/Decryption:** Uses AES-256 encryption to secure binary data.
- **PKCS7 Padding:** Implements PKCS7 padding for block size alignment.
- **File Handling:** Reads from and writes to specified files.

### Usage

1. Compile the C file and run the executable.
2. Provide the file paths for the hex, python, and code files as inputs.

```c
uint8_t iv[AES_IV_SIZE] = "yyyyyyyyyyyyyyyy";  // Initialization Vector
uint8_t key[AES_KEYLEN] = "xxxxxxxxxxxxxxxx";  // Encryption Key
```

## Bootloader Firmware

### Description

The Bootloader Firmware is an STM32-based firmware that facilitates the secure and reliable update of device firmware. It includes features for flashing, updating, and executing new firmware versions.

### Key Features

- **File Handling:** Reads and writes update files from the SD card.
- **Flash Programming:** Programs the new firmware into the STM32 flash memory.
- **Integrity Check:** Verifies the integrity of the received update before programming.

### Usage

1. Place the update file on the SD card.
2. The bootloader will read and program the new firmware automatically or manually based on the mode selected.

```c
void flash_update(char* st) {
    // Function to update flash with new firmware data
}
```

# Main Firmware


## Overview
The `Main_Firmware` is a comprehensive program designed to control and manage various hardware peripherals and functionalities of a microcontroller-based system. It is written in C and leverages several libraries and hardware interfaces to perform complex tasks such as data logging, communication, and real-time operations. The firmware is designed to operate on a microcontroller with features like UART, SPI, SD card interface, Real-Time Clock (RTC), and GPIO control.

## Key Features

### 1. **Real-Time Clock (RTC) Management**
- The firmware uses an RTC to maintain accurate timekeeping. This is crucial for time-stamping logged data and scheduling tasks.
- Time is read from the RTC and used throughout the firmware for various functions, ensuring synchronized operations.

### 2. **File Management on SD Card**
- Supports reading from and writing to an SD card using the FATFS library.
- Functions for opening, creating, reading, writing, and managing files on the SD card.
- File operations are performed using two file objects (`fil1` and `fil2`), allowing simultaneous access to multiple files.

### 3. **LoRa Communication**
- Implements communication using the LoRa (Long Range) protocol, which is ideal for low-power, long-range data transmission.
- Configurable LoRa settings are managed through the `LoRa` object, enabling customized operation for different use cases.
- Handles data reception and transmission over LoRa, including string parsing and command execution based on received data.

### 4. **SIM Module Integration**
- Manages a SIM module for GSM/GPRS communication, allowing the device to connect to the internet or send/receive SMS messages.
- Handles communication with the SIM module using UART, including sending AT commands and parsing responses.

### 5. **UART Communication**
- The firmware supports communication over UART for debugging and interaction with other devices.
- Data is received using DMA (Direct Memory Access) to ensure non-blocking operation, improving performance and reliability.
- Implements a circular buffer mechanism to handle incoming data efficiently without data loss.

### 6. **Interrupt Handling and DMA**
- Configured DMA for efficient data transfer between peripherals and memory, reducing CPU load and improving performance.
- Uses interrupt callbacks for real-time handling of data events, such as data reception over UART or SPI.

### 7. **Watchdog Timer Integration**
- The Independent Watchdog Timer (IWDG) is used to ensure system reliability. It resets the system in case of a software malfunction or hang, preventing the system from entering an undefined state.

### 8. **SPI Communication**
- Supports SPI communication for interfacing with devices such as sensors, displays, or additional memory modules.
- DMA is used for SPI data transfer to minimize CPU usage and speed up data handling.

### 9. **Real-Time Data Logging**
- Logs data such as sensor readings, system status, and communication messages to an SD card with time stamps.
- Ensures data integrity by synchronizing file operations and handling errors gracefully.

### 10. **LCD Display Support**
- Displays status messages, error codes, and other information on an LCD screen connected via I2C.
- Provides visual feedback for system operations, making it easier to debug and monitor the system in real-time.

### 11. **Modular Code Structure**
- The firmware is designed with modularity in mind, with separate modules for SIM management, LoRa communication, file operations, and system configuration.
- This makes the code easier to maintain, extend, and debug.

### 12. **Custom Data Structures and Algorithms**
- Implements custom data structures, such as dictionaries and linked lists, to manage configuration settings, command parsing, and other dynamic data.
- These structures enhance the flexibility and scalability of the firmware.

### 13. **Error Handling and Debugging**
- Comprehensive error handling mechanisms are in place for all critical operations, including file management, communication, and hardware interactions.
- Error messages and system states are logged to the SD card and displayed on the LCD for easier troubleshooting.

### 14. **System Initialization and Configuration**
- The firmware initializes all peripherals and configures system settings during startup.
- Configures GPIO pins, communication protocols, and other hardware components to ensure the system is ready for operation.

### 15. **Data Encryption and Security**
- Implements AES (Advanced Encryption Standard) encryption for secure data storage and communication.
- Ensures that sensitive data is protected during transmission and when stored on the SD card.

## Usage Scenarios

- **Remote Data Logging:** The firmware can be used to log environmental or system data remotely, transmitting it over LoRa or storing it on an SD card for later retrieval.
- **IoT Gateway:** With SIM and LoRa integration, the firmware can act as an IoT gateway, collecting data from various sensors and transmitting it to a central server.
- **Stand-Alone Controller:** The firmware can operate as a stand-alone controller for various applications, such as automation, monitoring, and control systems.

## Configuration and Customization

- **User Configurable Parameters:** Key parameters such as LoRa settings, file paths, and operation modes can be customized through configuration files stored on the SD card or commands sent via UART.
- **Modular Design:** Developers can easily modify or extend specific modules, such as adding new communication protocols or integrating additional hardware peripherals.

## Conclusion

The `Main_Firmware` provides a robust and flexible platform for developing embedded applications that require real-time data logging, remote communication, and efficient hardware control. Its modular structure, comprehensive feature set, and reliable operation make it suitable for a wide range of embedded systems and IoT applications.
## How to Compile and Run

### Python Scripts:
- Ensure Python 3.x is installed.
- Run the scripts using the command: `python script_name.py`.

### C/C++ Files:
- Use an IDE like Keil or STM32CubeIDE for compilation.
- Flash the compiled binary onto the STM32 device using ST-Link or other programming tools.

## License

This project is licensed under the MIT License.


For more information, please contact me.


