// Playtune_poll.h
// Next, choose which set of pins in this file you want to use. This should match the processor 
// chosen in the tools/board menu of the Arduino IDE (Integrated Development Environment).
#define ARDUINO_MEGA     // which set of pin definitions, which are below, to use
// For Arduino microcontrollers using an AVX processor, it's a little more complicated.
// For each channel, you need to give the pin number, the data register that it is wired
// to, and the number of the bit in that register that corresponds to the specific pin.
// You can get that information by looking at the schematic for the board, or from one
// of the great cheat-sheets at http://www.pighixxx.com/test/pinoutspg/boards/
// I've included several examples below.

// Stay off PIN 4 as its used for SD Card 

#ifdef ARDUNIO_MICRO  // define 8 channels on an Arduino Micro
#define CHAN_0_PIN 5    // channel 0 outputs on pin 5,
#define CHAN_0_REG C    // pin 5 is wired to register C,
#define CHAN_0_BIT 6    // and is bit 6 in that register

#define CHAN_1_PIN 6
#define CHAN_1_REG D
#define CHAN_1_BIT 7

#define CHAN_2_PIN 7
#define CHAN_2_REG E
#define CHAN_2_BIT 6

#define CHAN_3_PIN 8
#define CHAN_3_REG B
#define CHAN_3_BIT 4

#define CHAN_4_PIN 9
#define CHAN_4_REG B
#define CHAN_4_BIT 5

#define CHAN_5_PIN 10
#define CHAN_5_REG B
#define CHAN_5_BIT 6

#define CHAN_6_PIN 11
#define CHAN_6_REG B
#define CHAN_6_BIT 7

#define CHAN_7_PIN 12
#define CHAN_7_REG D
#define CHAN_7_BIT 6
#endif // Arduino Micro

#ifdef ARDUINO_NANO  // define 8 channels on an Arduino Nano
#define CHAN_0_PIN 5   // channel 0 outputs on pin 5,
#define CHAN_0_REG D   // pin 5 is wired to register D,
#define CHAN_0_BIT 5   // and is bit 5 in that register

#define CHAN_1_PIN 6
#define CHAN_1_REG D
#define CHAN_1_BIT 6

#define CHAN_2_PIN 7
#define CHAN_2_REG D
#define CHAN_2_BIT 7

#define CHAN_3_PIN 8
#define CHAN_3_REG B
#define CHAN_3_BIT 0

#define CHAN_4_PIN 9
#define CHAN_4_REG B
#define CHAN_4_BIT 1

#define CHAN_5_PIN 10
#define CHAN_5_REG B
#define CHAN_5_BIT 2

#define CHAN_6_PIN 11
#define CHAN_6_REG B
#define CHAN_6_BIT 3

#define CHAN_7_PIN 12
#define CHAN_7_REG B
#define CHAN_7_BIT 4
#endif // Arduino Nano

#ifdef ARDUINO_MEGA  // define 8 channels on an Arduino Mega
#define CHAN_0_PIN 5   // channel zero outputs on pin 53,
#define CHAN_0_REG E    // pin 53 is wired to register B,
#define CHAN_0_BIT 3    // and is bit 0 in that register

#define CHAN_1_PIN 6 // 
#define CHAN_1_REG H
#define CHAN_1_BIT 3

#define CHAN_2_PIN 7
#define CHAN_2_REG H
#define CHAN_2_BIT 4

#define CHAN_3_PIN 8
#define CHAN_3_REG H
#define CHAN_3_BIT 5

#define CHAN_4_PIN 45
#define CHAN_4_REG L
#define CHAN_4_BIT 4

#define CHAN_5_PIN 43
#define CHAN_5_REG L
#define CHAN_5_BIT 6

#define CHAN_6_PIN 41
#define CHAN_6_REG G
#define CHAN_6_BIT 0

#define CHAN_7_PIN 39
#define CHAN_7_REG G
#define CHAN_7_BIT 2
#endif // Arduino Mega

#ifdef ARDUINO_UNO  // define 8 channels on an Arduino Mega
#define CHAN_0_PIN 5   // channel zero outputs on pin 53,
#define CHAN_0_REG D    // pin 53 is wired to register B,
#define CHAN_0_BIT 5    // and is bit 0 in that register

#define CHAN_1_PIN 6
#define CHAN_1_REG D
#define CHAN_1_BIT 6

#define CHAN_2_PIN 7
#define CHAN_2_REG D
#define CHAN_2_BIT 7

#define CHAN_3_PIN 8
#define CHAN_3_REG B
#define CHAN_3_BIT 0

#define CHAN_4_PIN 9
#define CHAN_4_REG B
#define CHAN_4_BIT 1

#define CHAN_5_PIN 10
#define CHAN_5_REG B
#define CHAN_5_BIT 2

#define CHAN_6_PIN 11
#define CHAN_6_REG B
#define CHAN_6_BIT 3

#define CHAN_7_PIN 12
#define CHAN_7_REG B
#define CHAN_7_BIT 4
#endif // Arduino Uno
//stm32 modification...........................................................
#ifdef STM  // define 8 channels on an STM32
#define CHAN_0_PIN PB12    // channel 0 outputs on pin D1 (GPIO5)
#define CHAN_1_PIN PB13    // channel 1 outputs on pin D2 (GPIO4)
#define CHAN_2_PIN PB14    // channel 2 outputs on pin D3 (GPIO0)
#define CHAN_3_PIN PB15    // channel 3 outputs on pin D4 (GPIO2)
#define CHAN_4_PIN PA8    // channel 4 outputs on pin D5 (GPIO14)
#define CHAN_5_PIN PA9    // channel 5 outputs on pin D6 (GPIO12)
#define CHAN_6_PIN PA10    // channel 6 outputs on pin D7 (GPIO13)
#define CHAN_7_PIN PA11    // channel 7 outputs on pin D8 (GPIO15)
#endif
