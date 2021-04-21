/*

  AWATCH - A Simple Arduino Watch

  This software is covered by the 3-clause BSD license.
  See also: https://github.com/zooxo/awatch

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
              
*/


// ***** I N C L U D E S

#include <TinyWireM.h> // I2C Communication
#include <avr/sleep.h> // Power management
#include <avr/power.h> // Power management


// *****  F O N T

// FONT 3x7 (simple, upper-case letters only)
#define FWIDTH 3
#define FOFFSET '0'
const byte font[] PROGMEM = {
  0x7f, 0x41, 0x7f, // 0
  0x42, 0x7f, 0x40, // 1
  0x79, 0x49, 0x4f, // 2
  0x49, 0x49, 0x7f, // 3
  0x0f, 0x08, 0x7f, // 4
  0x4f, 0x49, 0x79, // 5
  0x7f, 0x49, 0x78, // 6
  0x03, 0x01, 0x7f, // 7
  0x7f, 0x49, 0x7f, // 8
  0x0f, 0x49, 0x7f, // 9
  0x00, 0x14, 0x00, // :
  0x00, 0x00, 0x00, // ; space
  0x08, 0x1c, 0x3e, // < left
  0x00, 0x40, 0x00, // = dot
  0x3e, 0x1c, 0x08, // > right
  0x04, 0x06, 0x7f, // ? arrowL
  0x7f, 0x06, 0x04, // @ arrowR
  0x7f, 0x09, 0x7f, // A
  0x7f, 0x49, 0x7f, // B
  0x7f, 0x41, 0x41, // C
  0x7f, 0x41, 0x7e, // D
  0x7f, 0x49, 0x49, // E
  0x7f, 0x09, 0x09, // F
  0x7f, 0x41, 0x79, // G
  0x7f, 0x08, 0x7f, // H
  0x41, 0x7f, 0x41, // I
  0x60, 0x40, 0x7f, // J
  0x7f, 0x1c, 0x77, // K
  0x7f, 0x40, 0x40, // L
  0x7f, 0x07, 0x7f, // M
  0x7f, 0x0e, 0x7f, // N
  0x7f, 0x41, 0x7f, // O
  0x7f, 0x09, 0x0f, // P
  0x7f, 0x61, 0x7f, // Q
  0x7f, 0x19, 0x6f, // R
  0x4f, 0x49, 0x79, // S
  0x01, 0x7f, 0x01, // T
  0x7f, 0x40, 0x7f, // U
  0x3f, 0x60, 0x3f, // V
  0x7f, 0x70, 0x7f, // W
  0x77, 0x1c, 0x77, // X
  0x0f, 0x78, 0x0f, // Y
  0x71, 0x49, 0x47, // Z
};


// *****  D I S P L A Y

// DEFINES
#define DADDR 0x3C // Display address
#define CONTRAST 0x80 // Initial contrast/brightness
#define DWIDTH 32 // Virtual display width
#define DLINES 3 // Virtual display "doubled" lines
#define XOFF 32 // X-Offset

// VARIABLES
static byte dbuf[DWIDTH * DLINES]; // Buffer for virtual screen (costs 96 bytes of dynamic memory)

// INIT
static const byte dinit [] PROGMEM = { // Initialization sequence
  0xA8, 0x3F,     // Set mux ratio (N+1) where N goes from 15 to 63 (0F to 3F)
  0xDA, 0x12,     // COM config pin mapping:  02, 12, 22 or 32
  0x8D, 0x14,     // Charge pump (0x14 enable or 0x10 disable)
  0x81, CONTRAST, // Contrast
  0xAF,           // Display on
};
static void bootscreen(void) { // Boot screen - reset the display
  for (uint8_t i = 0; i < sizeof(dinit); i++) dsendcmd(pgm_read_byte(dinit + i));
}

// SUBPROGRAMS
void dsendcmdstart(void) { // Command mode
  TinyWireM.beginTransmission(DADDR);
  TinyWireM.send(0x00);
}
void dsendcmd(uint8_t cmd) { // Send command
  dsendcmdstart();
  TinyWireM.send(cmd);
  dsendstop();
}
void dsenddatastart(void) { // Data mode
  TinyWireM.beginTransmission(DADDR);
  TinyWireM.send(0x40);
}
void dsenddatabyte(uint8_t data) { // Send data
  if (!TinyWireM.write(data)) {
    dsendstop();
    dsenddatastart();
    TinyWireM.write(data);
  }
}
void dsendstop(void) { // Stop command or data mode
  TinyWireM.endTransmission();
}

static void setpage(uint8_t page) { // Set display area (max. 64 bytes)
  dsendcmdstart();
  TinyWireM.send(0x20); TinyWireM.send(0x01); // Set display mode
  TinyWireM.send(0x21); TinyWireM.send(XOFF); TinyWireM.send(XOFF + 64); // Set col range
  TinyWireM.send(0x22); TinyWireM.send(page); TinyWireM.send(page); // Set page range
  dsendstop();
}

static void dbuffill(byte n) { // Fill display buffer with pattern
  for (byte i = 0; i < DWIDTH * DLINES; i++) dbuf[i] = n;
}
static void doff(void) { // Display off
  dsendcmd(0xAE);
}
static void don(void) { // Display on
  dsendcmd(0xAF);
}

static byte expand4bit(byte b) { // 0000abcd  Expand 4 bits (lower nibble)
  b = (b | (b << 2)) & 0x33;     // 00ab00cd
  b = (b | (b << 1)) & 0x55;     // 0a0b0c0d
  return (b | (b << 1));         // aabbccdd
}

static void ddisplaypage(byte page) { // Write dbuf-page to display
  setpage(page);
  dsenddatastart();
  for (byte j = 0; j < DWIDTH; j++) { // Columns
    byte tmp = expand4bit((dbuf[(page / 2) * DWIDTH + j] >> (page % 2) * 4) & 0x0f);
    dsenddatabyte(tmp); dsenddatabyte(tmp); // Double width
  }
  dsendstop();
}
static void ddisplay(void) { // Print display
  for (byte i = 0; i < DLINES; i++) { // Double height pages
    ddisplaypage(2 * i); // Upper page
    ddisplaypage(2 * i + 1); // Lower page
  }
}

static void printcat(byte c, byte x, byte y, byte width, byte height) { // Print character at (x|y)
  for (byte k = 0; k < height; k++) { // Height (1 or 2)
    for (int i = 0; i < FWIDTH; i++) {
      byte tmp = pgm_read_byte(font + FWIDTH * (c - FOFFSET) + i);
      if (height == 2) tmp = expand4bit(tmp >> (4 * k) & 0x0f);
      for (byte j = 0; j < width; j++) dbuf[x + (width * i + j) + (y + k) * DWIDTH] = tmp;
    }
  }
}

/*static void printpixel(byte x, byte y) { // Print pixel at (x|y)
  dbuf[x + (y / 8)*DWIDTH] |= 1 << (y % 8);
  }
  const byte sine[] = {0, 21, 42, 62, 81, 100, 118, 134, 149, 162, 173, 183, 190, 196, 199, 200};
  static void printcircle(byte X, byte Y, byte r) { // Print circle at (X|Y) with radius r
  for (byte i = 0; i < 16; i++) {
    byte x = r * sine[15 - i] / 200, y = r * sine[i] / 200;
    byte a = X + x, b = X - x, c = Y + y, d = Y - y;
    printpixel(a, c); printpixel(a, d); printpixel(b, d); printpixel(b, c);
  }
  }*/


// ***** K E Y B O A R D

#define BSET 3
#define BOFF1 1
#define BOFF2 4


// ***** A P P L I C A T I O N

// MACROS
#define _ones(x) ((x)%10)        // Calculates ones unit
#define _tens(x) (((x)/10)%10)   // Calculates tens unit
#define _huns(x) (((x)/100)%10)  // Calculates hundreds unit
#define _tsds(x) (((x)/1000)%10) // Calculates thousands unit
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit)) // Clear bit
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit)) // Set bit

// DEFINES
#define TIMEOUT 5000 // Display ontime in ms
#define WDT_INTERVAL 6 // Watchdog intervall (~1s) (0~9: 16/32/64/128/250/500/1k/2k/4k/8k ms)
#define TIME1224 24UL // 12 or 24 hours

// VARIABLES
static unsigned long tim = 0UL; // Global time
static unsigned long outtime; // Time after display times out (millis + TIMEOUT)
static boolean issleep = false; // Sleeping stat
static byte setstate = 0; // Setting state (0~6: none/s/m/10m/h/10h/d)
static int vcc; // Vcc
static unsigned int wdtmillisperinterrupt = 1072; // To be calibrated [slow]1071......1072[fast]
static unsigned long wdtmillis = 0;
static byte weekday = 0; // Day (0~6: MO/TU/WE/TH/FR/SA/SU)
const char wday[] PROGMEM = {'M', 'O', 'T', 'U', 'W', 'E', 'T', 'H', 'F', 'R', 'S', 'A', 'S', 'U'};


// SUBPROGRAMS
static void delayshort(byte ms) { // Delay (with timer) in ms with 8 bit duration
  long t = millis(); while ((byte)(millis() - t) < ms) ;
}

static void delaylong(byte n) { // Delay n quarters of a second
  for (byte i = 0; i < n; i++) delayshort(250);
}

static int batt(void) { // Measure battery voltage
  ADMUX = _BV(MUX3) | _BV(MUX2); // Set voltage bits for ATTINY85
  delayshort(2); // Settle Vref
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC)) ; // Measuring
  byte low = ADCL, high = ADCH;
  return (112530L / ((high << 8) | low));
}

static void setdtimeout() { // Set offtime for display
  outtime = millis() + TIMEOUT;
}

/*
  static byte eachframemillis, thisframestart, lastframedurationms; // Framing times
  static boolean justrendered; // True if frame was just rendered

  static boolean nextframe(void) { // Wait (idle) for next frame
  byte now = (byte) millis(), framedurationms = now - thisframestart;
  if (justrendered) {
    lastframedurationms = framedurationms;
    justrendered = false;
    return false;
  }
  else if (framedurationms < eachframemillis) {
    if (++framedurationms < eachframemillis) idle();
    return false;
  }
  justrendered = true;
  thisframestart = now;
  return true;
  }

  static void idle(void) { // Idle, while waiting for next frame
  set_sleep_mode(SLEEP_MODE_IDLE);
  power_adc_disable(); // Disable ADC
  execsleep();
  }

  static void execsleep(void) { // Execute sleep mode
  sleep_enable();
  sleep_cpu();
  sleep_disable();
  power_all_enable();
  power_timer1_disable(); // Never using timer1
  }
*/

static void setsleep() { // Prepare sleep
  setstate = 0;
  dbuffill(0x00); // Avoid displaying old time when wake up
  doff();
  delayshort(2); // wait oled stable
  issleep = true;
}

void sleep() { // Sleep
  //cbi(ADCSRA, ADEN); // Switch off AD-converter
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode
  sleep_mode();
  // SLEEP ... until watchdog wakeup (ISR(WDT_vect)) or interrupt pin changes (ISR(PCINT0_vect))
  //sbi(ADCSRA, ADEN); // Switch on AD-converter
}

static void setwatchdog(byte n) { // Set watchdog timer
  if (n > 9 ) n = 9;
  byte wv = n & 0x07; if (n > 0x07) wv |= (1 << 5); wv |= (1 << WDCE); // Calc watchdog value
  MCUSR &= ~(1 << WDRF);
  WDTCR |= (1 << WDCE) | (1 << WDE); WDTCR = wv; WDTCR |= _BV(WDIE); // Set watchdog value
  sbi(GIMSK, PCIE); sbi(PCMSK, PCINT3); sbi(PCMSK, PCINT4); // Turn pin change interupts on
  sei(); // Enable interrupts
}

ISR(WDT_vect) { // Watchdog timer wakeup
  sleep_disable();
  wdtmillis += wdtmillisperinterrupt;
  sleep_enable();
}

ISR(PCINT0_vect) { // Interrupt pin change wakeup
  sleep_disable();
  if (issleep) {
    setdtimeout(); // Extend display timeout while printing/setting
    //delayshort(2);
    don();
    issleep = false;
  }
  sleep_enable();
}


// ***** P R I N T I N G

static void printhm(byte n, byte x) { // Print (big) hours/minutes
  printcat(_tens(n) + FOFFSET, x, 0, 2, 2); printcat(_ones(n) + FOFFSET, x + 8, 0, 2, 2);
}

static void prints(byte s) { // Print seconds
  printcat(_tens(s) + FOFFSET, 0, 2, 1, 1); printcat(_ones(s) + FOFFSET, 4, 2, 1, 1);
}

static void printarrow(byte x) { // Print setting arrow (h/m)
  printcat('?', x, 2, 1, 1); printcat('@', x + 3, 2, 1, 1);
}

static void printscreen() { // Print screen
  dbuffill(0x00);

  printcat(':', 13, 0, 2, 2); // Colon
  printhm((tim / 1000UL / 60UL / 60UL) % TIME1224, 0); // Hours
  printhm((tim / 1000UL / 60UL) % 60UL, 18); // Minutes
  if (setstate < 2 && setstate != 6) prints((tim / 1000UL) % 60UL); // Seconds
  if (setstate == 0 || setstate == 6) { // Weekday
    weekday = (tim / 1000UL / 60UL / 60UL / 24UL) % 7UL;
    printcat(pgm_read_byte(&wday[2 * weekday]), 11, 2, 1, 1);
    printcat(pgm_read_byte(&wday[2 * weekday + 1]), 15, 2, 1, 1);
  }

  if (setstate == 1) printcat('<', 8, 2, 1, 1); // Set arrows
  else if (setstate == 2) printarrow(26);
  else if (setstate == 3) printarrow(18);
  else if (setstate == 4) printarrow(8);
  else if (setstate == 5) printarrow(0);
  else if (setstate == 6) printcat('>', 7, 2, 1, 1);

  else { // Vcc (print when setstate=0)
    printcat(_huns(vcc) + FOFFSET, 23, 2, 1, 1);
    printcat('=', 26, 2, 1, 1);
    printcat(_tens(vcc) + FOFFSET, 29, 2, 1, 1);
  }

  ddisplay();
}


// *****  S E T U P ,   L O O P

void setup() {
  pinMode(BOFF1, INPUT);  pinMode(BOFF2, INPUT); // Disable unused pins (and digispark-LED)
  pinMode(BSET, INPUT_PULLUP);// Init buttons

  TinyWireM.begin(); bootscreen(); dbuffill(0x00); // Init display

  setwatchdog(WDT_INTERVAL); // Init WDT
  setdtimeout(); // Reset timeout

  vcc = batt(); // Read voltage initially

  power_timer0_disable(); // Power off timer0 (timer1 is used)
  ADCSRA &= ~bit(ADEN); // Power off (not used) AD converter
}


void loop() {

  if (issleep) sleep(); // Go to sleep or stay sleeping

  else {
    if (millis() > outtime) setsleep(); // Prepare sleeping

    else { // Printing and setting
      //if (!(nextframe())) return; // Pause render (idle) until next frame
      tim += wdtmillis; // Calculate time
      wdtmillis = 0; // Reset sleeping time
      printscreen();
      if (digitalRead(BSET) == LOW) { // Set pressed
        setdtimeout();
        delaylong(2); // Wait for potentially longpress
        vcc = batt(); // Read voltage
        if (digitalRead(BSET) == LOW) { // Longpressed
          delaylong(2); // Wait to avoid next shortpress
          if (setstate++ > 5) setstate = 0; // 6 setstati
        }
        else { // Key was shortpressed
          if (setstate == 1) tim -= ((tim / 1000UL) % 60UL) * 1000UL; // Reset seconds
          else if (setstate == 2) tim += 60000UL; // Add one minute
          else if (setstate == 3) tim += 600000UL; // Add 10 minutes
          else if (setstate == 4) tim += 3600000UL; // Add 1 hour
          else if (setstate == 5) tim += 36000000UL; // Add 10 hours
          else if (setstate == 6) if (weekday++ > 5) weekday = 0; // Add day, 7 weekdays
        }
      }
    }
  }
}
