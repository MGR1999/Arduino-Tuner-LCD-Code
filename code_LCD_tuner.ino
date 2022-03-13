/*
 * Arduino Tuner LCD code v.2.0
 * Date: 3/13/22
 * Author:
 * 
 * Michael Guske, Oregon State Univeristy 
 * 
 *
 *
 * Based upon:
 * 
 * Arduino Guitar Tuner
 * created September 12, 2013
 * by Nicole Grimwood
 * https://www.instructables.com/id/Arduino-Guitar-Tuner/
 * 
 * Arduino Frequency Detection
 * created October 7, 2012
 * by Amanda Ghassaei
 *
 * This code is in the public domain. 
*/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>


//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//storage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

//variables for deciding whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

//variables for tuning
int correctFrequency;//the correct frequency for the string being played
LiquidCrystal_I2C lcd(0x27,16,2);

void setup(){
  
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(2,0);
  lcd.print("Tuner is ON");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  E A D G B E ");

 
  
  
  cli();//disable interrupts
  
  //set up continuous sampling of analog pin 0 at 38.5kHz
 
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready
  
  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
  
  time++;//increment timer at rate of 38.5kHz
  
  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp){
    maxAmp = abs(127-ADCH);
  }
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }
  
}

void reset(){//clean out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}


//Determine the correct frequency and print
//a pointer on the LCD indicating the current note
void stringCheck(){
  
  if(frequency>78.3&frequency<86.5){
    klear();
    lcd.setCursor(2, 1);
    lcd.print("^"); 
  }
  if(frequency>104.5&frequency<115.5){
     klear();
     lcd.setCursor(4, 1);
     lcd.print("^");
  }
  if(frequency>139.5&frequency<154.2){
    klear();
    lcd.setCursor(6, 1);
    lcd.print("^");
  }
  if(frequency>186.2&frequency<205.8){
    klear();
    lcd.setCursor(8, 1);
    lcd.print("^");
  }
  if(frequency>234.5&frequency<259.2){
    klear();
    lcd.setCursor(10, 1);
    lcd.print("^");
  }
  if(frequency>313.1&frequency<346.1){
    klear();
    lcd.setCursor(12, 1);
    lcd.print("^");
  }

}

//Clear the bottom line of the LCD. 
void klear(){
  lcd.setCursor(0,1);
  lcd.print("               ");
 
}



void loop(){
  
  
  if (checkMaxAmp>ampThreshold){
    frequency = 38462/float(period);//calculate frequency timer rate/period
  }
 
  
  stringCheck();
  
  delay(100);
 
}
