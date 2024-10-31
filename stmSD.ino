
#include "Playtune_poll.h"
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(PA3, PA2, PA1, PA0, PB9, PB8);


// Declares
// Tesla Coil Section
#define MAX_CHANS 2 // Max Channels in this Program I have set it up for 3 but only 2 Work
#define POLLTIME  24


#define STM
// change how fast the program interrupts from 5 usec to 50usec 24 ~ 44100 khz
#define DBUG 0 // Control Debug Output 
#define USEC 0 // Get output of delay in Serial *Use with Volume to tune your songs 
/* You need USEC to put %DutyCycle,max_usec,Single_usec% into individual song header files
    for instance if %15,300,500% in the header of a song *Before the first {* then the program will load those settings
    for that song ONLY then it will load the header of the next song or globals
*/

//************************************************************************************************************************
//************************************************************************************************************************
//*********Coil Specific Variabls Change as needed************************************************************************

#define Tesla_Duty_Cycle 20    //*Recommended 10-20%* 0 to 99% adjust per song
#define Tesla_Max_usec 100     //*Recommended ~250* int 65535 max adjust per song This is max length when 2 notes are playing *Recommended ~250*
#define Tesla_Single_usec 200  //*Recommended ~450* int 65535 max This Overrides Duty Cycle up to 50% if we only have one *Recommended ~450*
// note playing we dont need low duty cycle(this can effect songs that switch quickly) adjust per song
// if Tesla Single usec is set to 0 it will not change from set Duty Cycle

//************************************************************************************************************************
//************************************************************************************************************************
//Playtune pt;

#define UpButton      PB11
#define SelectButton  PB1
#define DownButton    PB0
#define ButtonBack    PB10  




#define MAXSONGS 254 //Max number of songs to load this is needed for less memory UNO
String SDArray[MAXSONGS+1]; // Max number of Songs to load searching for .c (Do not put them in folders)


//Tu peux utiliser le SDArray pour nom de fichier
//but the SDarray crops the filename 
const String Fname[] = {" ", "20thCenturyFox", "Alone", "On My Way", "An-Der-Schonen", 
                  "Avengers", "Bad Apple", "Nachtmusik", "Let It Go", "GnR", "GOT", 
                  "Hark The Herald", "Last Dance", "Les-Toreadors", 
                  "LineWithoutAHook", "Little Einsteins", "LoveIsAnOpenDoor",
                  "Mayday", "monody", "Nutcracker", "Secrets",  
                  "Poker Face", "River Flows", "Save ur Tears", "SoldierPoetKing", 
                  "Sweet Dreams", "TheOnlyException", "The Spectre", "Unity", "Wake Me Up", 
                  "Wilhelm-Tell-(Ouverture)", "X mean", };
byte SongID = 1; // This starts as one to start at A
byte MaxSongID = 0;
bool SongPaused = false;
bool tune_playing = false;
double SDVolume = 1.00; // Scaling Control for (Duty Cycle) / Volume
float FREQ = 1;
float half_period = 0;
int ON_TIME = 100; 

bool mode = true;   //Pour le Freq et le Dcy etat ou 'FlipFlop'
bool menu = true;
bool manual = false;
bool music = false;
int menuChoice = 0;
bool musicPlay = false; //Condition/Flag to play or stop the SDmusic


// Setup for SD Card
File myFile;


//Menu choice will run an startup 
void Mmenu(){
  LCD_Print(F("Manual Mode"),0,0);
  LCD_Print(F("Music Mode"),0,1);
}


//----------------------------------

void BUTTON_UP(){

  if(menu && !manual && !music && !tune_playing){   //startup choice for manual/SDmusic
    Mmenu();
    LCD_Print(F("  <"),12,0);
    LCD_Print(F("    "),11,1);
    menuChoice = 0;    
  }
 
  //Volume choice. Can only be triggered when a song is active
  else if(tune_playing && !menu){
    if (SDVolume < 1) {
    SDVolume += .05;
    }else if(SDVolume > 1){
      SDVolume = 1;
    }
    LCD_Print(SDVolume,0,1);
  }

  else if(!music && manual && menu && !tune_playing){
    if(mode){

      FREQ++; //increment le freq
      if(FREQ > 100)FREQ = 100;
      LCD_Print(FREQ,9,0);

    }else{
      ON_TIME++; //increment le on_time
      if(ON_TIME > 200)ON_TIME = 200;
      LCD_Print(ON_TIME,12,1);
    }
  }

  //Sets up the music choice and prints it
  else if(music && !manual && !tune_playing && !menu){
    lcd.clear();
    SongID = SongID - 1;
    if (SongID < 1 ) SongID = 1;
    //tune_stopscore();
    LCD_Print(F(">"),0,0);
    LCD_Print(Fname[SongID],1,0);
    LCD_Print(Fname[SongID+1],0,1);
    

  }
 
}

void BUTTON_DOWN(){

  if(menu && !manual && !music && !tune_playing){
    Mmenu();
    LCD_Print(F("  <"),11,1);
    LCD_Print(F("    "),12,0);
    menuChoice = 1; //if 0 manual mode if 1 music mode    
  }

  //Volume choice. Can only be triggered when a song is active
  else if(tune_playing && !menu ){

    if (SDVolume > .02 && SDVolume < .8) {
    SDVolume -= .02;
    } else if (SDVolume >= .8) {
    SDVolume -= .05;
    }  else if (SDVolume < .02) {
    SDVolume = .01;
    }
    LCD_Print(SDVolume,0,1);

  }
  else if(!music && manual && menu && !tune_playing){
    if(mode){

      FREQ--; //decrement le freq
      if(FREQ < 1)FREQ = 1;
      LCD_Print(FREQ,9,0);

    }else{
      ON_TIME--; //decrement le on_time
      if(ON_TIME < 10)ON_TIME = 10;
      LCD_Print(ON_TIME,12,1);
    }

  }
  else if(music && !manual && !tune_playing && !menu){
    lcd.clear();
    SongID = SongID + 1;
    if (SongID > MaxSongID ) SongID = 1;
    //tune_stopscore();
    LCD_Print(F(">"),0,0);
    LCD_Print(Fname[SongID],1,0);
    if(SongID < MaxSongID)LCD_Print(Fname[SongID+1],0,1);
    

  }
}


//---------------------------

void BUTTON_SELECT(){


  if(!tune_playing && menu){  //Selecting the startup choice manual/SDmusic


    //lcd.clear();
    //`````````````````````````````````
    //Starts Manual Mode
    if(menuChoice == 0){
      manual = true;
      music = false;
      musicPlay = false;

      // FlipFlops between Freq & Dcy
      static bool FreqOrDcy = false;
      FreqOrDcy = !FreqOrDcy;
      if(!FreqOrDcy){

        LCD_Print(F(">Freq = "),0,0);
        LCD_Print(F("On T(us) = "),0,1);
        LCD_Print(FREQ,9,0);
        LCD_Print(ON_TIME,12,1);
        mode = true;
      }
      else if(FreqOrDcy){

        LCD_Print(F("Freq = "),0,0);
        LCD_Print(F(">On T(us) = "),0,1);
        LCD_Print(FREQ,8,0);
        LCD_Print(ON_TIME,12,1);
        mode = false;
      }
    }
    //`````````````````````````````````

    //Music Mode ###############################################

    //Starts Music mode and Music List
    else if(menuChoice == 1){
      lcd.clear();
      manual = false;
      music = true;
      menu = false;
      LCD_Print(F(">"),0,0);
      LCD_Print(Fname[SongID],1,0);
      LCD_Print(Fname[SongID+1],0,1);
    }

    //Play || Stop songs
    }else if(!menu && music && !manual){
      if(musicPlay){
        lcd.clear();
        tune_stopscore();
        LCD_Print(F(">"),0,0);
        LCD_Print(Fname[SongID],1,0);
        LCD_Print(Fname[SongID+1],0,1);
      }else{
      musicPlay = true;
      }
    }
    //Music Mode ###############################################


}


//Return everything to default 
void BACK(){
  if(!tune_playing || menu || manual || music || tune_playing || musicPlay){
    tune_stopscore();
    digitalWrite(PB6,0);
    lcd.clear();
    mode = true;   //Pour le Freq et le Dcy etat ou 'FlipFlop'
    menu = true;
    manual = false;
    music = false;
    menuChoice = 0;
    musicPlay = false;
    FREQ = 1;
    ON_TIME = 100;
    Mmenu();
  }
}


//======================================================================================

void setup() {
  pinMode(PC13, OUTPUT);
  pinMode(PB6, OUTPUT); //Output of the manual mode
  pinMode(UpButton, INPUT_PULLUP);
  pinMode(DownButton, INPUT_PULLUP);
  pinMode(SelectButton, INPUT_PULLUP);


  attachInterrupt(digitalPinToInterrupt(SelectButton), BUTTON_SELECT, FALLING);     //Pin PB1
  attachInterrupt(digitalPinToInterrupt(UpButton), BUTTON_UP, FALLING);             //Pin PB11
  attachInterrupt(digitalPinToInterrupt(DownButton), BUTTON_DOWN, FALLING);         //Pin PB0
  attachInterrupt(digitalPinToInterrupt(ButtonBack), BACK, FALLING);                //Pin PB10


  lcd.begin(16, 2);
  LCD_Print(F("Loading ..."),0,0);
  delay(100);
  SD.begin(PA4); // This May need to be Changed for your SD Card Pinout to the CCS

  myFile = SD.open("/");
  MaxSongID = printDirectory(myFile, 0) - 1;
  if(!SD.begin(PA4)){
  LCD_Print(F("error ..."),0,0);
  }else{
  LCD_Print(F("Playing ..."),0,0);
  }
  Mmenu();
}

void loop () {
digitalWrite(PC13,1); //pour L'etat Du LED ou debug

if(manual){//manual mode
  float half_period = ((1/FREQ)*1000000)/2;
  digitalWrite(PB6,0);
  delayMicroseconds(half_period);
  digitalWrite(PB6,1);
  delayMicroseconds(ON_TIME);
  digitalWrite(PB6,0);
  delayMicroseconds(half_period - ON_TIME);
}
else if(musicPlay){//SDmusic mode
  PlaySong();
  musicPlay = false;
}


}

//==========================================================

void PlaySong() { //Lets Play a Song 
  lcd.clear();
  LCD_Print(Fname[SongID],0,0);
  LCD_Print(SDVolume,0,1);
  tune_playscore (SDArray[SongID], Tesla_Duty_Cycle, Tesla_Max_usec, Tesla_Single_usec);  /* start playing */

  while(tune_playing){
    digitalWrite(PC13,0);
  }

}




// Load songs into memeory only done once at startup !
// The SD card must have .c files Not in folders up to 255 songs (can be changed)
byte printDirectory(File dir, int numTabs) { // Search SD Card for Songs 
  String SDDir;
  byte SDArraySize = 1; // lets start at 1 so 0 can be a turn for byte
  while (true) {
    File root =  dir.openNextFile();
    if (!root || SDArraySize >= MAXSONGS+1) {
      break;
    }
    if (root.isDirectory()) {

      SDDir += root.name();
      SDDir += '/';
      printDirectory(root, numTabs + 1);
    } else {
      if ( isFnMusic(root.name()) ) { // Here is the magic This loads only .c files 
        SDDir += root.name();
        SDArray[SDArraySize] = SDDir;
        SDArraySize++;
#if DBUG
        Serial.println(SDArray[SDArraySize]);
#endif
        SDDir = "";
      }
    }
    SDDir = "";
    root.close();
  }
  return SDArraySize;
}

// Checks files names for .c extension 
bool isFnMusic(char* filename) {
  int8_t len = strlen(filename);
  bool result;
  if (strstr(strlwr(filename + (len - 2)), ".c")) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

void LCD_Print(String Storage,byte x,byte y) {
  lcd.setCursor(x, y);
  lcd.print("           ");
  lcd.setCursor(x, y);
  lcd.print(Storage);  
}
void LCD_Print(double Storage,byte x,byte y) {
  lcd.setCursor(x, y);
  lcd.print("           ");
  lcd.setCursor(x, y);
  lcd.print(Storage);  
}