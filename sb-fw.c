/*
  SpinnyB - 2014 WeakNet Labs
  GNU (c) 2014, WeakNetLabs@Gmail.com
  Hardware: v1.0
  Firmware: v0.1 -- (for Arduino Micro - update Tone.cpp)
  Rotary dial code modified from original author: Guidomax, instructables.com
    http://www.instructables.com/id/Interface-a-rotary-phone-dial-to-an-Arduino/
  Hardware idea: Matthew Hesse
  Hardware design and code: Douglas Berdeaux
  Hardware donation: Victoria Weis
*/
#include <Tone.h>
boolean playTone = false;
int count;
int lastState = LOW;
int trueState = LOW;
long lastStateChangeTime = 0;
boolean record = 0; // record mode, default - turned off
boolean pdm = 0; // pulse dial mode!
int prevDigit = 0; // keep track of previously pressed digit
int stored[16] = { // store up to 16 digits
  13,13,13,13,13,13,13,13,
  13,13,13,13,13,13,13,13,
};
boolean storedDigits = false;
/* Constants Below: */
const int in = 7; // output for rotary device
const int holdTime = 3000; // milliseconds long to consider KP as "HELD"
const int spinDelay = 100; // Rotary spin delay (requires tuning)
const int debounceDelay = 10;
Tone pin[2]; /* 3 lines for the tone output pins:  */
const int pin0 = 11;  // for low tone in MF
const int pin1 = 12;  // for high tone in MF
const int b2600 = 4; // 2600Hz button pin (INPUT)
const int bkp = 5; // KP button pin (INPUT)
const int bst = 6; // ST button pin (INPUT)
const int mfDuration = 75; // tone duration for MF tones
const int bb[16][2] = { // MF 0,1,2,3,4,5,6,7,8,9,kp,st,2400+2600,kp2,st2,ss4 super
  {1300,1500},{700,900},{700,1100},      // 0,1,2
  {900,1100},{700,1300},{900,1300},      // 3,4,5
  {1100,1300},{700,1500},{900,1500},     // 6,7,8
  {1100,1500},{1100,1700},{1500,1700},   // 9,kp,st
};
 
void setup(){ // BEGIN{}
  Serial.begin(9600); // DEBUG MODE ONLY REMOVE FOR PROD
  pinMode(in, INPUT); // input for rotary device
  pin[0].begin(pin0); // Initialize our first tone generator
  pin[1].begin(pin1); // Initialize our second tone generator
  pinMode(b2600, INPUT); // 2600 button
  pinMode(bst, INPUT); // 2600 button
  pinMode(bkp, INPUT); // 2600 button
  startUp(); // play startup tones
  return;
}
 
void loop(){ /* Event driven listen functions, etc */
  buttons(); // Handle buttons first
  rotary(); // process teh rotary dial mechanism
  return;
}

void rotary(void){
  int reading = digitalRead(in);  
  if ((millis() - lastStateChangeTime) > spinDelay){
    if (playTone){
      if(count==10) count=0;
      prevDigitCheck(count); // was a special button pressed twice?
      if(pdm){
        if(count==0){ count=10; }
        pulse(count);  
      }else{
        mf(count);
      }
      playTone = false;
      count = 0;
    }
  }
  if (reading != lastState){
    lastStateChangeTime = millis(); // grab the time to check against later
  }
  if ((millis() - lastStateChangeTime) > debounceDelay){
    // debounce - this happens once it's stablized
    if (reading != trueState){
      // this means that the switch has either just gone from closed->open or vice versa.
      trueState = reading;
      if (trueState == HIGH){
        // increment the count of pulses if it's gone high.
        count++;
        playTone = true;
      }
    }
  }
  lastState = reading;
  return;
}

void prevDigitCheck(int newDigit){ // called by buttons AND rotary :)
  if(record){
    Serial.print("caught digit: ");
    Serial.println(newDigit);
    if(newDigit==10&&prevDigit==0){ // ONLY store the first KP
      storeDigit(newDigit); 
    }else if(newDigit != 10){ // store 0-9,ST
      storeDigit(newDigit); 
    }
  }
  if(prevDigit==newDigit&&prevDigit==10){ // KP x2 in a row
    if(record){ 
      record = false;
      prevDigit = 0; // reset this
      notify(0); // notify off
      Serial.println("record mode is now off");
      if(stored[1]==13){
        resetStored(); // reset, nothing was recorded
        storedDigits = false;
      }else{
        storedDigits = true; 
      }
      return;
    }else{
      resetStored(); // truncate the stored digits
      record = true; //   to make room for the new
      prevDigit = 0; // reset this
      notify(1);
      Serial.println("record mode is now on");
      return;
    }
    return; // don't overwrite the prevDigit again
  }else if(prevDigit==newDigit&&prevDigit==11){ // ST twice
    //pdm ? pdm = false : pdm = true;
    if(pdm){ // turn PDM off, it's on:
      pdm = false; 
      prevDigit = 0; // reset this
      notify(0);
    }else{
      pdm = true;
      notify(1);
    }
    return; // don't overwrite the prevDigit again
  }
  prevDigit = newDigit;
}

void buttons(void){ // process any pushed buttons
  if(digitalRead(b2600)){ // 2600 pressed
    sf(2600,1000);
    delay(300);
    if(storedDigits){
      for(int i=0;i<16;i++){
        if(stored[i]==13){ 
          return;
        }else{
          mf(stored[i]); 
        }
      } 
    }
  }else if(digitalRead(bkp)){ // KP Pressed
    prevDigitCheck(10);
    if(prevDigit==0){ // should never have a 0 then a KP 
      return;
    }else{
      mf(10); 
    }
    delay(300);
  }else if(digitalRead(bst)){ // ST pressed
    prevDigitCheck(11);
    mf(11);
    delay(300);
  }
  return;
}

void pulse(int digit){ // pulse out 2600 chirps (and flash LED)
  for(int i=0;i<digit;i++){
    sf(2600,66);
    delay(34);
  }
  return;
}
 
void mf(int digit){ /* simply play MF tones */
  pin[0].play(bb[digit][0],mfDuration);
  pin[1].play(bb[digit][1],mfDuration);
  delay(mfDuration + 100);
  return;  
}

void startUp(){ // play the startup tones
  for(int i=0;i<=4;i++){
    sf(2600,33);
    delay(66);
  }
  return; 
}

void resetStored(){ // Reset the stored digits
  for(int i=0;i<16;i++){
    stored[i]=13;
  }
  return; 
}

void sf(int freq,int dur){ // send me the frequency and duration
  pin[1].play(freq,dur);
  delay(dur);
  return;
}

void notify(int mode){ // 1 == on or 0 == off 
  if(mode){ // start record mode notification:
    pin[0].play(1700,100);
    delay(100);
    pin[0].play(2200,500);
    delay(700);
  }else{
    pin[0].play(2200,100);
    delay(100);
    pin[0].play(1700,500);
    delay(700);
  }
  return;
}

void storeDigit(int digit){
 for(int i=0;i<16;i++){
   if(stored[i]==13){
     stored[i]=digit;
     return;
   }
 }
}
