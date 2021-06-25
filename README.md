# Activity Recognition System

## How to use

1. Enter your WiFi name `NETWORK_SSID` and password `NETWORK_PASSWORD` in `activity_recognition.ino`
2. Compile the code and upload it to the board, pay attention to the serial monitor
3. While the board is starting, put it in a horizontal position and don't move it until calibration is over (when the LEDs stop blinking, the board is ready)
4. The IP of the board will be printed to the serial output
5. Connect to the IP through a browser (internet access is required for JavaScript libraries)


## Features

* Detect 3 types of activities (none, walking, jumping)
* Plot accelerometer measurements and their standard deviations using Chart.js
* Display the orientation of the board in 3D using WebGL

