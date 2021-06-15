# Changelog for rESCue firmware

## 15.06.2021

- implemented CANBUS-only read and write mode
- prepared release 1.2.0

## 24.05.2021

- prepared release 1.1.1
- improved OTA update, package resend and delay to give BLE stack time
- LightBar fixed
- bugfixing idle light

## 20.05.2021

- improved OTA and enrollment
- initial Lightbar
- startsound for OTA
- prepared release 1.1.0

## 15.05.2021

- updated README
- fixed wron macro definition

## 03.05.2021

- added OTA documentation

## 01.05.2021

- more configuration options

## 30.04.2021

- Updated Blynk App to v1.1
- Updated QR code

## 29.04.2021

- updated PIN configuration according to rev3 of the rESCue PCB
- updated documentation
- some more light magic

## 28.04.2021

- sound can be turned off
- added more sounds
- sound "preview" when selected
- Blynk app reads settings on connect
- fixed the fading bugs and made it smoother

## 27.04.2021

- fixed some bugs in fading lights
- smoother fading

## 20.04.2021

- updated doc, PCB image and sizing

## 19.04.2021 (v1.0.0)

- released version 1.0.0
- added Over The Air (OTA) update
- changed partition layout (min-spiff)
- fixed blynk mapping

## 15.04.2021

- Fixed "braking lights"

## 14.04.2021

- updated outdated Adafruit Neopixel lib to 1.7.0

## 12.04.2021

- added syncPreferences to BleServer to sync saved preferences with Blynk onConnect
- added "brake light" detection when current <-4.0A (regen)
- read some more props from VESC ("totalCurrentIn" and "pidPosition")

- updated Blynk QR-Code
