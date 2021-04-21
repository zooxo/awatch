# awatch
A Simple Arduino Watch


  ____________________
   PREAMBLE
  ____________________
  AWATCH is a simple watch that shows the time on an OLED display and can be
  operated with one button only. It shows hours, minutes, seconds, day and
  battery voltage.
  To save power AWATCH uses the watchdog timer of AVR microcontrollers. So the
  display remains dark and the microcontroller in sleep mode until the button
  ist pressed. Longpressing the button alters the setting state, while
  shortpressing alters the setable value.
  Have fun
  deetee
  ____________________
   OPERATION
  ____________________
  AWATCH uses an OLED display (64x48 I2C SSD1306) and because of the size of
  less then 4 Kilobytes it can be compiled for many Arduino/AVR microcontrollers
  (i.e. ATTINY or Digispark).
  Setting Modes (alter with longpressing the button):
    0 ... Show time, no setting
    1 ... Reset seconds
    2 ... Add one minute
    3 ... Add 10 minutes
    4 ... Add one hour
    5 ... Add 10 hours
    6 ... Add one day
   Note: After the timeout of 5s AWATCH enters the sleep mode and resets the
         setting mode.
   Tweaking:
   - Define the brightness of the display (0...255) with: #define CONTRAST 0x80
   - Define the timeout (in ms) with: #define TIMEOUT 5000
   - Set 12/24h-format with: #define TIME1224 24UL // 12 or 24 hours
   - Calibrate watchdog intervall time (ms) with:
     static unsigned int wdtmillisperinterrupt = 1072;
  ____________________
   CIRCUIT DIAGRAM
  ____________________
     _________________
    |   OLED-DISPLAY  |
    |  64x48 SSD1306  |
    |_GND_VCC_SCK_SDA_|
       |   |   |   |
               |   |
     __|___|___|___|__
    | GND VCC P2  P0  |
    |  AVR  ATTINY85  |
    |_P3______________|
       |   
       / Set button   
       |
      GND
              
