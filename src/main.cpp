/**
 * @author Tevin Poudrier
 * @date Tuesday, August 6, 2024 9:43:15 AM
 * @details Light meter program for ASU, allows user to configure the gain and get lux from TSL2591
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <math.h>
#include <Wire.h>

//custom i2c
TwoWire myWire = TwoWire();

#define GAIN_BTN 12
#define SAMPLE_BTN 13

#define DEBOUNCE_TIME 100

void configureSensor(void);

LiquidCrystal_I2C lcd(0x3F,20,4);  // set the LCD address to 0x3F for a 16 chars and 2 line display
Adafruit_TSL2591 tsl = Adafruit_TSL2591();

//Define debounce variables
int prevSamplePress = LOW;
int prevSampleTime = millis();
int prevModePress= LOW;
int prevModeTime = millis();
int prevGainPress = LOW;
int prevGainTime = millis();

void setup(void) 
{
  Serial.begin(9600);

  //initalize buttons, use pullup resistors to simplify wiring
  //Buttons are inverted logic (pressed means LOW)
  pinMode(GAIN_BTN, INPUT_PULLUP);
  pinMode(SAMPLE_BTN, INPUT_PULLUP);

  
  //initalize lcd
  lcd.init(); 
  lcd.backlight();

  //initalize i2c for tsl2591
  myWire.setClock(10000); //10kHz to maximize wire length
  myWire.begin();
  
  //initalize tsl2591
  if (tsl.begin(&myWire, TSL2591_ADDR)) 
  {
    Serial.println(F("Found TSL2591 sensor"));
    lcd.print("Light Meter");
  } 
  else 
  {
    lcd.print("Sensor Err");
    lcd.setCursor(0,1);
    lcd.print("Fix and Reset");
    while (1);
  }
  
  //Configure light sensor
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  
  //update display
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Gain: x25");
}


void loop(void) 
{
  //Store user input
  bool getSample = false;
  bool changeGain = false;

  //Get button state of sample button, then debounce and only use falling edge
  int samplePress = digitalRead(SAMPLE_BTN);
  if (samplePress != prevSamplePress && (millis() - prevSampleTime) > DEBOUNCE_TIME) {
    prevSamplePress = samplePress;
    prevSampleTime = millis();

    //falling edge
    if (samplePress == LOW) {
      Serial.println("sample");
      getSample = true;
    }
  }

  //Get button state of gain button, then debounce and only use falling edge
  int gainPress = digitalRead(GAIN_BTN);
  if (gainPress != prevGainPress && (millis() - prevGainTime) > DEBOUNCE_TIME) {
    prevGainPress = gainPress;
    prevGainTime = millis();

    //falling edge
    if (gainPress == LOW) {
      Serial.println("gain");
      changeGain = true;
    }
  }

  //Get and print the current lux value
  if (getSample == true) {
    double lux = tsl.getLux();
    char formated_lux[10] = {0}; //string
    
    //Change significant figures printed based on gain
    //using snprintf to uses printf formatting for strings, see https://linux.die.net/man/3/sprintf
    //snprintf copieds 10 characters from the formatted string into the formated_lux string
    switch(tsl.getGain())
    {
      //round to tens place
      case TSL2591_GAIN_LOW:
        snprintf(formated_lux, 10, "%d", (int) (round(lux/10)*10));
        break;
      
      //round to ones place
      case TSL2591_GAIN_MED:
        snprintf(formated_lux, 10, "%d", (int) round(lux));
        break;
      
      //two decimal precision
      case TSL2591_GAIN_HIGH:
        snprintf(formated_lux, 10, "%d.%02d", (int) lux, (int) (lux*100) % 100);
        break;
      
      //four decimal precision
      case TSL2591_GAIN_MAX:
        snprintf(formated_lux, 10, "%d.%04d", (int) lux, (int) (lux*10000) % 10000);
        break;
    }
    
    //Print lux reading and the formated lux to terminal
    Serial.print("Lux: ");
    Serial.print(lux, 6); //print 6 decimal places
    Serial.print("Formatted Lux: ");
    Serial.println(formated_lux);

    //Print lux to LCD, lux is -1 when gain error occurs
    lcd.setCursor(0,1);
    if (lux > 0) {
      lcd.print("Lux: ");
      lcd.print(formated_lux);
      lcd.print("    ");
    }
    else {
      lcd.print("Err, gain   ");
    }
    
  }

  //Change the gain by incrementing to the next gain value
  if (changeGain == true) {
    lcd.clear();
    tsl2591Gain_t gain = tsl.getGain();
    switch(gain)
    {
      //low to med
      case TSL2591_GAIN_LOW:
        tsl.setGain(TSL2591_GAIN_MED);
        tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
        lcd.setCursor(0,0);
        lcd.print("Gain: x25");
        break;
      
      //med to high
      case TSL2591_GAIN_MED:
        tsl.setGain(TSL2591_GAIN_HIGH);
        tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
        lcd.setCursor(0,0);
        lcd.print("Gain: x428");
        break;
      
      //high to max (dim light)
      case TSL2591_GAIN_HIGH:
        tsl.setGain(TSL2591_GAIN_MAX);
        tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);

        lcd.setCursor(0,0);
        lcd.print("Gain: x9876");
        break;
      
      //max to low (bright light)
      case TSL2591_GAIN_MAX:
        tsl.setGain(TSL2591_GAIN_LOW);
        tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
        lcd.setCursor(0,0);
        lcd.print("Gain: x1");
        break;
    }
  }
  

  delay(10); //small delay to help with button debounce
}