
#include <Arduino.h>

//stm32 modification...........................................................
#include <libmaple/libmaple_types.h>                                          //
#include <libmaple/timer.h>                                                   //
//stm32 modification...........................................................


#include "Playtune_poll.h"  // this contains, among other things, the output pin definitions



// Defines 
// A general-purpose macro joiner that allow rescanning of the result
// (Yes, we need both levels! See http://stackoverflow.com/questions/1489932)


static int tempDC = Tesla_Duty_Cycle;
static unsigned long tempMU = Tesla_Max_usec;
unsigned long tempSU = Tesla_Single_usec;
  
static byte num_chans = 0;         // how many channels are assigned to pins
static byte num_chans_used = 0;
static boolean pins_initialized = false;
static boolean timer_running = false;

static unsigned /*short*/ int scorewait_interrupt_count;
static unsigned /*short*/ int millisecond_interrupt_count;
static unsigned /*short*/ int interrupts_per_millisecond;

static int32_t accumulator [MAX_CHANS];
static int32_t decrement [MAX_CHANS];
static byte playing [MAX_CHANS];
static byte pins [MAX_CHANS];
static unsigned int max_high_ticks[MAX_CHANS];
static unsigned int max_high_ticks_single[MAX_CHANS];
static unsigned int current_high_ticks[MAX_CHANS];

// note commands in the bytestream
#define CMD_PLAYNOTE  0x90   /* play a note: low nibble is generator #, note is next byte, maybe volume */
#define CMD_STOPNOTE  0x80   /* stop a note: low nibble is generator # */
#define CMD_RESTART 350      /*0xe0 restart the score from the beginning */
#define CMD_STOP  360        /* 0xf0 stop playing */
/* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit big-endian number of msec to wait */

#define ACCUM_RESTART 1073741824L  // 2^30. Generates 1-byte arithmetic on 4-byte numbers!
#define MAX_NOTE 123 // the max note number; we thus allow MAX_NOTE+1 notes
#define MAX_POLLTIME_USEC 50
#define MIN_POLLTIME_USEC 5
const int32_t max_decrement_PGM[MAX_NOTE + 1] PROGMEM = {
  877870L, 930071L, 985375L, 1043969L, 1106047L, 1171815L, 1241495L, 1315318L,
  1393531L, 1476395L, 1564186L, 1657197L, 1755739L, 1860141L, 1970751L,
  2087938L, 2212093L, 2343631L, 2482991L, 2630637L, 2787063L, 2952790L,
  3128372L, 3314395L, 3511479L, 3720282L, 3941502L, 4175876L, 4424186L,
  4687262L, 4965981L, 5261274L, 5574125L, 5905580L, 6256744L, 6628789L,
  7022958L, 7440565L, 7883004L, 8351751L, 8848372L, 9374524L, 9931962L,
  10522547L, 11148251L, 11811160L, 12513488L, 13257579L, 14045916L, 14881129L,
  15766007L, 16703503L, 17696745L, 18749048L, 19863924L, 21045095L, 22296501L,
  23622320L, 25026976L, 26515158L, 28091831L, 29762258L, 31532014L, 33407005L,
  35393489L, 37498096L, 39727849L, 42090189L, 44593002L, 47244640L, 50053953L,
  53030316L, 56183662L, 59524517L, 63064029L, 66814011L, 70786979L, 74996192L,
  79455697L, 84180379L, 89186005L, 94489281L, 100107906L, 106060631L,
  112367325L, 119049034L, 126128057L, 133628022L, 141573958L, 149992383L,
  158911395L, 168360758L, 178372009L, 188978561L, 200215811L, 212121263L,
  224734649L, 238098067L, 252256115L, 267256044L, 283147915L, 299984767L,
  317822789L, 336721516L, 356744019L, 377957122L, 400431622L, 424242525L,
  449469299L, 476196134L, 504512230L, 534512088L, 566295831L, 599969533L,
  635645578L, 673443031L, 713488038L, 755914244L, 800863244L, 848485051L,
  898938597L, 952392268L, 1009024459L, 1069024176L
};
static int32_t decrement_table[MAX_NOTE + 1]; // values may be smaller in this actual table used

const uint16_t min_ticks_per_period_PGM[MAX_NOTE + 1] PROGMEM = {
  2446, 2308, 2178, 2056, 1940, 1832, 1728, 1632, 1540, 1454, 1372,
  1294, 1222, 1154, 1088, 1028, 970, 916, 864, 816, 770, 726, 686, 646,
  610, 576, 544, 514, 484, 458, 432, 408, 384, 362, 342, 322, 304, 288,
  272, 256, 242, 228, 216, 204, 192, 180, 170, 160, 152, 144, 136, 128,
  120, 114, 108, 102, 96, 90, 84, 80, 76, 72, 68, 64, 60, 56, 54, 50,
  48, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 24, 22, 20, 20, 18,
  18, 16, 16, 14, 14, 12, 12, 12, 10, 10, 10, 8, 8, 8, 8, 6, 6, 6, 6, 6,
  4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};
static uint16_t ticks_per_period[MAX_NOTE + 1];  // values may be larger in this actual table used



//------------------------------------------------------------------------------
// Initialize and start the timer
//------------------------------------------------------------------------------

//stm32 modification...........................................................
#define MY_TIMER TIMER1                                                       //
void timer_setup(uint32_t p_time){                                            //
  timer_pause(MY_TIMER);                                                      //
  // Set prescaler (assuming 72MHz clock, 1MHz timer clock)                   //
  timer_set_prescaler(MY_TIMER, 72);                                          //
  //uint32_t x = 1000000;                                                     //
  // Set reload value (period_us interval at 1MHz timer clock)                //
  timer_set_reload(MY_TIMER, p_time);                                         //
  // Generate an update event to reload the prescaler value immediately       //
  timer_generate_update(MY_TIMER);                                            //
  // Resume the timer                                                         //
  timer_resume(MY_TIMER);                                                     //                                                          
}                                                                             //
void TimerAttachInterrupt(void (*callback)()) {                               //
  // Attach the interrupt callback function                                   //
  timer_attach_interrupt(MY_TIMER, TIMER_UPDATE_INTERRUPT, callback);         //
}                                                                             //
//stm32 modification...........................................................






void tune_start_timer(int polltime) {
  // Decide on an interrupt poll time
  interrupts_per_millisecond = 1000 / polltime;  // this is a truncated approximation
  millisecond_interrupt_count = interrupts_per_millisecond;
#if DBUG
  Serial.print("polltime = "); Serial.println(polltime);
#endif

  // Set up the real tables we use based on our chosen polling rate, which may be faster
  // than the 50 microseconds that the table was computed for. We use 32- or 64-bit arithmetic
  // for intermediate results to avoid errors, but we only do this during initialization.

  for (int note = 0; note <= MAX_NOTE; ++note) {
    decrement_table[note] =
      ((uint64_t) pgm_read_dword(max_decrement_PGM + note) * (uint64_t)polltime) / MAX_POLLTIME_USEC
      >> 1; // if we're doing volume, use the full note period, not the half period
    ticks_per_period[note] = ((uint32_t)pgm_read_word(min_ticks_per_period_PGM + note) * MAX_POLLTIME_USEC) / polltime;
    ;
  }

//stm32 modification.....................................................
  timer_setup(polltime);                                                //
  TimerAttachInterrupt(timer_ISR);                                      //
  /*                                                                    //
  Timer1.initialize(polltime); // start the timer                       //
  Timer1.attachInterrupt(timer_ISR);                                    //
  */                                                                    //
//stm32 modification.....................................................
  timer_running = true;
}

//------------------------------------------------------------------------------
// Set up output pins, compile-time macros for manipulating them,
// and runtime bit set and clear routines that are faster than digitalWrite
//------------------------------------------------------------------------------



//stm32 modification. Output pins
void tune_init_pins(void) {


#define tune_initchan(pin)   \
  if (num_chans < MAX_CHANS) {  \
    pins[num_chans] = pin; \
    pinMode(pin, OUTPUT);  \
    ++num_chans;  \
  }

#ifndef CHAN_0_PIN
#define CHAN_0_PIN PB12
#else
  tune_initchan(CHAN_0_PIN)
#endif


#ifndef CHAN_1_PIN
#define CHAN_1_PIN PB13
#else
  tune_initchan(CHAN_1_PIN)
#endif


#ifndef CHAN_2_PIN
#define CHAN_2_PIN PB14
#else
  tune_initchan(CHAN_2_PIN)
#endif


#ifndef CHAN_3_PIN
#define CHAN_3_PIN PB15
#else
  tune_initchan(CHAN_3_PIN)
#endif


#ifndef CHAN_4_PIN
#define CHAN_4_PIN PA8
#else
  tune_initchan(CHAN_4_PIN)
#endif


#ifndef CHAN_5_PIN
#define CHAN_5_PIN PA9
#else
  tune_initchan(CHAN_5_PIN)
#endif


#ifndef CHAN_6_PIN
#define CHAN_6_PIN PA10
#else
  tune_initchan(CHAN_6_PIN)
#endif


#ifndef CHAN_7_PIN
#define CHAN_7_PIN PA11
#else
  tune_initchan(CHAN_7_PIN)
#endif


  pins_initialized = true;
}

//stm32 modification...........................................................
void chan_set_high(byte c){                                                   //
  digitalWrite(pins[c], HIGH); // Initialize pin to LOW state                 //
}                                                                             //
void chan_set_low(byte c){                                                    //
  digitalWrite(pins[c], LOW); // Initialize pin to LOW state                  //
}                                                                             //
//stm32 modification...........................................................



//------------------------------------------------------------------------------
// Start playing a note on a particular channel
//------------------------------------------------------------------------------


void tune_playnote (byte chan, byte note, byte volume) {
#if DBUG
  Serial.print("chan="); Serial.print(chan);
  Serial.print(" note="); Serial.print(note);
#endif
// Changes for Tesla Coil Volume 
  if (chan < num_chans) {
    if (note <= MAX_NOTE) {
      if (volume > 127) volume = 127;
      decrement[chan] = decrement_table[note];
      
      max_high_ticks[chan] = .01 * tempDC * ticks_per_period[note]; // Sets Duty Cycle % from 0 to 100
      if (max_high_ticks[chan] > tempMU * 1/POLLTIME) max_high_ticks[chan] = tempMU * 1/POLLTIME; // sets Max duty cycle Needed for High Notes
      max_high_ticks[chan] *= SDVolume; // Lets Multiply by our Volume
      if (max_high_ticks[chan] >= ticks_per_period[note]) max_high_ticks[chan] = ticks_per_period[note]-1; // Max high ticks check 
      if (max_high_ticks[chan] < 1) max_high_ticks[chan] = 1; // max low ticks check 
      max_high_ticks_single[chan] = max(.75 * ticks_per_period[note] * SDVolume , .01 * tempDC * ticks_per_period[note]); // Sets Duty Cycle % from 0 to 100
      if (max_high_ticks_single[chan] > tempSU * 1/POLLTIME && tempSU != 0) max_high_ticks_single[chan] = tempSU * 1/POLLTIME; // sets Max duty cycle Needed for High Notes
       max_high_ticks_single[chan] *= SDVolume; // Lets Multiply by our Volume
      if (tempSU <= tempMU) max_high_ticks_single[chan] = max_high_ticks[chan];
      if (max_high_ticks_single[chan] >= ticks_per_period[note]) max_high_ticks_single[chan] = ticks_per_period[note]-1 ; // Max high ticks check 
      if (max_high_ticks_single[chan] < 1) max_high_ticks_single[chan] = 1; // max low ticks check
       
      // Lets make sure the note does not invert by making it low before we start. 
      digitalWrite(pins[chan], LOW);
      current_high_ticks[chan] = 0; // we're starting with output low
      
#if DBUG
      Serial.print("  max high ticks="); Serial.print(max_high_ticks[chan]); Serial.print("  max single="); Serial.print(max_high_ticks_single[chan]);Serial.print("  Max Ticks Note="); Serial.println(ticks_per_period[note]);
#endif

#if USEC
    Serial.print("  uSec Multipule="); Serial.print(max_high_ticks[chan]*POLLTIME); Serial.print("  uSec Single="); Serial.println(max_high_ticks_single[chan]*POLLTIME);
#endif

      accumulator[chan] = ACCUM_RESTART;
      playing[chan] = true;
    }
  }
}


//------------------------------------------------------------------------------
// Stop playing a note on a particular channel
//------------------------------------------------------------------------------

void tune_stopnote (byte chan) {
  if (chan < num_chans) {
    if (playing[chan]) {
      playing[chan] = false;
      chan_set_low(chan);
#if DBUG
      Serial.print("  stopping chan "); Serial.println(chan);
#endif
    }
  }
}

//------------------------------------------------------------------------------
// Start playing a score
//------------------------------------------------------------------------------

void tune_playscore (String score,int Tesla_DC, int Tesla_uS, int Tesla_Single_uS) {
  tempDC = Tesla_DC; // Set Tesla Duty Cycle per song
  tempMU = Tesla_uS; // set max Tesla on time per song 
  tempSU = Tesla_Single_uS;
  tune_SD_Initial(score); // Lets Initialize a song 
  
  if (!pins_initialized) tune_init_pins();
  if (tune_playing) tune_stopscore();
  if (!timer_running) tune_start_timer(POLLTIME);
  num_chans_used = MAX_CHANS;

  tune_stepscore();  /* execute initial commands */
  tune_playing = true;
}

void tune_stepscore (void) { // continue in the score
  byte chan, note, vol;
  int cmd;
  /* Do score commands until a "wait" is found, or the score is stopped.
    This is called initially from tune_playscore, but then is called
    from the slow interrupt routine when waits expire.
  */
  while (1) {
    cmd = tune_SD_GetData();
#if DBUG
      Serial.print("CMD="); Serial.println(cmd);
#endif    
    if (cmd < 340) { /* wait count in msec. */
      /* wait count is in msec. */
      unsigned temp1 = (cmd*256);
      unsigned temp2 = tune_SD_GetData();
      scorewait_interrupt_count = temp1+temp2;
#if DBUG
      Serial.print("waitcount="); Serial.println(scorewait_interrupt_count);
#endif
      break;
    } else if (cmd == 380 || cmd == 381) { /* stop note */
      chan = cmd - 380;
      tune_stopnote (chan);
    } else if (cmd == 390 || cmd == 391) { /* play note */
      chan = cmd - 390;
      note = tune_SD_GetData(); // argument evaluation order is undefined in C!
      tune_playnote (chan, note, vol);
    }  else if (cmd == CMD_RESTART) { /* restart score */
      //***** FIX THIS ****
//      score_cursor = score_start;
    } else if (cmd == CMD_STOP) { /* stop score */
      SongID = SongID + 1;
      if (SongID > MaxSongID ) SongID = 0;
      tune_playing = false;
      tune_SD_end();     
      break;
    }
  }
}

//------------------------------------------------------------------------------
// Stop playing a score
//------------------------------------------------------------------------------

void tune_stopscore (void) {
  int i;
  for (i = 0; i < num_chans; ++i)
    tune_stopnote(i);
    myFile.close();
  tune_playing = false;
  delay(100);
}

//------------------------------------------------------------------------------
// Stop all channels
//------------------------------------------------------------------------------


//stm32 modification.....................................................
void tune_stop_timer(void) {                                            //
  tune_stopscore();                                                     //
  /*                                                                    //
  Timer1.stop();                                                        //
  Timer1.detachInterrupt();                                             //
  */                                                                    //
  timer_pause(MY_TIMER);                                                //
  timer_detach_interrupt(MY_TIMER, TIMER_UPDATE_INTERRUPT);             //    
  timer_running = false;                                                //
}                                                                       //
//stm32 modification.....................................................

void tune_SD_Initial (String FileOpen) {
  // open the file for reading:
  tune_SD_end();
  myFile = SD.open(FileOpen); 
  delay(200);
  if (myFile) {
#if DBUG
    Serial.println("File Open");
#endif
  // Lets OpenRead to get through Header start at {
    while (myFile.available()) { // Lets get past the start of the file
      char test = myFile.read();
      // %15,750,1200% This is a header for TC_SD It looks for %DutyCycle,Max_usec,Single_usec% 
      if (test=='%') {
               String buff="";
               int holdi =0;
              while (myFile.available()) { // Lets find first byte to load in and save 
                char test = (char)myFile.read();
                // 390 391 are channels on follow by a note
                // 380 381 are channels off 
                // 360 = end of song
                   if (test==',') {
                    if (holdi == 0) tempDC = buff.toInt();
                    if (holdi == 1) tempMU = buff.toInt();
                   //int testB = buff.toInt();
                   buff="";
                   holdi++;
                  }  else if (test=='%') {
                   tempSU = buff.toInt();
                   buff="";
                   break;
                  } else {
                    buff +=  test;
                   }
             } 
      }
      if (test=='{') {
#if DBUG
        Serial.println("Header Clear");
#endif
        break;
      }
    }  
  } else {
#if DBUG
    Serial.println("File did not Open");
#endif
  }
}

int tune_SD_GetData(void) {
 // read from the file until there's nothing else in it:
    String buff="";
    while (myFile.available()) { // Lets find first byte to load in and save 
      char test = (char)myFile.read();
      // 390 391 are channels on follow by a note
      // 380 381 are channels off 
      // 360 = end of song
         if (test=='x') {
          buff += 3;
         } else if (test=='c') {
          buff += 4; // 0xc0 = 340
         } else if (test=='e') {
          buff += 5; // 0xe0 = 350
         } else if (test=='f') {
          buff += 6; // 0xf0 = 360
         } else if (test==',') {
         int testB = buff.toInt();
         buff="";
         return testB;
        }  else if (test=='}') {
         int testB = buff.toInt();
         buff="";
         return testB;
        }else {
          buff +=  test;
         }
   } 
   return 360; // end song if file not open 
}

void tune_SD_end(void) { // End of SD file so lets close itaaaaas  aa
    myFile.close();
#if DBUG
    Serial.println("File Closed");
#endif
    delay(50);
  
}

//------------------------------------------------------------------------------
//  Timer interrupt Service Routine
//
// We look at each playing note on the (up to) 3 channels,
// and determine whether it is time to create the next edge of the square wave
//------------------------------------------------------------------------------


//stm32 modification...........................................................
void dospeaker(int chan) {                                                    //
  if (chan < MAX_CHANS && playing[chan]) {                                    //
    if (current_high_ticks[chan] && --current_high_ticks[chan] == 0) {        //
      digitalWrite(pins[chan], LOW); /* end of high pulse */                  //
    }                                                                         //
    accumulator[chan] -= decrement[chan];                                     //
    if (accumulator[chan] < 0) {                                              //      
      digitalWrite(pins[chan], HIGH);                                         //
      accumulator[chan] += ACCUM_RESTART;                                     //  
      if ((playing[0] && !playing[1]) || (!playing[0] && playing[1])) {       //
        current_high_ticks[chan] = max_high_ticks_single[chan];               //
      } else {                                                                //
        current_high_ticks[chan] = max_high_ticks[chan];                      //
      }                                                                       //
    }                                                                         //
  }                                                                           //
}                                                                             //
//stm32 modification...........................................................

void timer_ISR(void) {  //**** THE TIMER INTERRUPT COMES HERE ****
  // first, check for millisecond timing events
  if (--millisecond_interrupt_count == 0) {
    millisecond_interrupt_count = interrupts_per_millisecond;
    // decrement score wait counter
    if (tune_playing && scorewait_interrupt_count && --scorewait_interrupt_count == 0) {
      // end of a score wait, so execute more score commands
      tune_stepscore ();  // execute commands
    }
  }

  if (tune_playing) {
    dospeaker(0);
    dospeaker(1);
    dospeaker(2);
    dospeaker(3);
    //dospeaker(4);
    //dospeaker(5);
    //dospeaker(6);
    
  }
}