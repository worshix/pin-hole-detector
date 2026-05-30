# Chapter : Methodology

* Yor job is to do chapter 3 for @karen_document.pdf using @template.pdf as a guide
* The pictures are in this folder @pictures
## Overview
- There is already a chapter 3 in there but please change all of it.
- We are using latex @chapter_3.tex 
- The systems components are below.
- In the document the pictures are required for each component and its purpose. 
- In the Pinhole detection process, the light shines on the paper, if there is more light discovered by the LDR on the other side it means there is a pinhole
- A model will betrained with python for the software which will use flask
- The software will be on a computer that will be connected to the esp32 via wifi
- The ESPCAM will send images to the software via an API route  
- Using MQTT, the software can send back information to the detection system.
- We used solidworks for the design of the model
- the  mechanical structure will have that wiper motor because ir moves slower without the need of a VFD.
- Buck converter will be used to step down the voltage from 12v to 5v for the esp32
- power supply is 12V for the wiper motor
- relay is for the wiper motor

## Components
- LDRs
- Bright Light
- Esp32 CAM
- Wiper Motor
- Power Supply
- Relay Module (12V, 5Amps)
- LCD 20 x 4 I2C
- Esp32
- buck Converter
