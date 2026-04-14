# 2ServoFlapOrnithopter-by-ESPnow-WiFi-on-ESP32S3
Running 2ServoFlapOrnithopter system using Wi-Fi (ESP-Now system) on Seeed Studio XIAO ESP32S3

The Seeed Studio ESP32S3 is a high-speed board operating at up to 240MHz and features 2.4GHz Wi-Fi capabilities.

By connecting two ESP32S3s using the ESP32-now system, the receiver can be omitted, thus reducing the weight of the Ornithopter.

Code was created while interacting with the AI. Since the existing PMMReader cannot be used with the ESP32S3, this was created first, followed by code using the ESP-now system.

The 2.4GHz antenna hasn't arrived yet, so a short-range connection without the antenna was tested, and it seems to be working well.

The radio waves seem to reach about 50-100 meters, so it might be usable with a small Ornithopter. 

![Wiring](image/260409%20ESP32S3WiFiSFOfor2SFOV3CODE.jpg)

TX16S-ESP32S3-WiFi-ESP32S3-Servo system for 2Servo Flap Ornithopter
(https://www.youtube.com/watch?v=kpqIa_A7cuo)
