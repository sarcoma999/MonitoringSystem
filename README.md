Monitoring System for production processes

References:
  OV7670 non-FIFO scripts are based on: https://github.com/alankrantas/OV7670-ESP32-TFT/tree/main?tab=readme-ov-file (based on https://github.com/kobatan/OV7670-ESP32)
  ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer/tree/master

Hardware:
  ESP-WROOM-32 (LuaNode32)
  non-FIFO OV7670 camera module
  DHT11 (humidity and temperature sensor module)
  HC-SR04 (ultrasonic distance sensor)
  LED (implemented in alarm system)
  BUZZER (implemented in alarm system)
  3V DC Motor (simulates the stirring process)
  L293D (motor driver for motor connection)
  SG90 (simulates the valve`s behavior)

Hardware connection (wiring scheme)

  ![image](https://github.com/sarcoma999/MonitoringSystem/assets/104567515/29845281-4575-491f-a1e6-e84a5e2ebfd5)

Developed a web interface using an asynchronous web server to read data from sensors, display them on the client, receive a video stream from the camera, and generate output signals to actuators

  ![image](https://github.com/sarcoma999/MonitoringSystem/assets/104567515/1ceaf3ca-dcee-488e-8349-5667cb5fc5cc)

  
