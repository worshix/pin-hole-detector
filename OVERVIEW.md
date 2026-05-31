# Pinhole Detector In Plastic Roll

- We are using an esp32 to check for pinholes in a plastic roll.

## ESP32Pin Connections

| Pin | Component |
|-----|----------|
| 4    | Light Relay |
| 5 | Motor (Moves the Plastic) |
| 18 | Start / Stop Button   |
| 21 | SDA 20 x 4 LCD Display   |
| 22 | SCL 20 x 4 LCD Display   |
| 25  | Buzzer |
| 26 | Stop Indicator |
| 27 | Start Indicator |
| 34 | LDR 1  |
| 35 | LDR 2  |

## ESP32-CAM-MB
noconnections here just power

wifi name is "pinhole-detector"
wifi password is "pinhole123"
borker Ip Address is 192.168.137.1
I am using mosquitto broker for communication

## How it works

- The job of the esp32 is hole detection. If there is a hole the light will increase.
- Youy start by recording the ambient light then you start checking if light has increased. 
- We are using an LDR to detect the light.
- An increase in light means there has been a pinhole detected. If that happens you do not stop the motor but you do alert the user using buzzer and stop LED.
- The espcam is only used for taking pictures of the plastic every second and sending it to the application for dfect detection using A trained model
- esp32 cam only starts when taking pics when we hit start.
- It will get this information from the mqtt broker

## Application

- The application is made using django.
- The application has a dashboard showing the status of the esp32 and the images from the espcam
- The dashboard also displays a summary of all defects found and pinholes found. 
- The application also has a history page showing all the defects and pinholes found. 
-  All images should be stored in a folder as well.
-  I will finish the model training later but for now I need you to develop a full user interface and a route that receives images from esp32-cam-mb and stores them in a folder. 
- The route should be /api/images/receive
- I need high quality images so the model can be trained on them.
- There should also be a start and stop button on the dashboard to control the esp32.
- The esp32 should be able to send messages to the application via mqtt broker.
- The esp32 should be able to receive messages from the application via mqtt broker.