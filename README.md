# Parking sensor:
  Wireless parking assistance System 

## Features:
  Wireless connection
  Attachment to any metal point of the vehicle
  Quick-release design
  Displaying the distance in meters and showing through the light strip
  The system is waterproof with IP - 65 protection

## Composition:
  The system includes
    Outdoor unit (the one that is mounted on the vehicle body)
    Indoor unit (the one that is located in the cabin)

## System logic:
The distance sensor collects data, removes junk values and transmits NRF
  The NRF of the outdoor unit transmits data to the NRF of the indoor unit
  It is being processed in the form of conversion to meters and to the code for 74HC595, which controls the LED display.
  There is also sound processing and sending a signal to the piezo squeaker (BUZ)
  End! And so on through the cycle.

## Components:
### Outdoor Unit
  AJ - SR04M
  Arduino Nano
  BMS module
  2 Li-ion 18650 3.7V each
  The 74HC595 chip
  nRF24L01
  Charging module TZT teng DDTCCRUB
  Button with rubber gasket
  2 neodymium magnets (I'm already planning to make 4 for greater reliability)
### Indoor unit
  Arduino Nano
  BMS module
  2 Li-ion 18650 3.7V each
  The 74HC595 chip
  nRF24L01
  Charging module TZT teng DDTCCRUB
Button with rubber gasket
    Piez squeaker (BUZ)

Product photo:
https://github.com/user-attachments/assets/96d1c9fd-ff93-4996-920d-564c63c06a23
https://github.com/user-attachments/assets/f8f21fb6-308a-4b19-b9ac-bf73f55bd6ab
https://github.com/user-attachments/assets/d208c4cd-26d4-4563-955e-dc7528ba7d5b

Photos of 3D models (layout):
https://github.com/user-attachments/assets/9b1c6de0-56ad-46bd-90fe-751394716301
https://github.com/user-attachments/assets/3a5fda4e-28cd-48c2-95ce-8e8e8b001a86
https://github.com/user-attachments/assets/6ea48209-74a3-413e-850c-3dc23e67226a

Photos of 3D models (printed version):
https://github.com/user-attachments/assets/b57e1896-0d03-428c-ad0a-ace15aea84e6
https://github.com/user-attachments/assets/a7808c1e-6ca8-4690-aa55-c06ada343056
