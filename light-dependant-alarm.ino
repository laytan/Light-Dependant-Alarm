#include <TimedAction.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "sound.h"

Adafruit_SSD1306 display;

// Configuration
#define TIMEOUT_TIME 100

// Pins
#define LEFT 13
#define MIDDLE 12
#define RIGHT 9
#define LDR 4

// Button state
bool prevLeftClick = 0;
bool leftClick = 0;
bool prevRightClick = 0;
bool rightClick = 0;
bool prevMiddleClick = 0;
bool middleClick = 0;

// Variable declaration
bool isDisplaying = true;
bool isTimeSet = false;
bool alarmSet = false;
bool isAlarmOn = false;
bool ldrValue = true; // No light
unsigned char displayTime = 0;

// The selected alarm time
unsigned char selectedAlarmTimeHours = 0;
unsigned char selectedAlarmTimeMinutes = 0;
// The set alarm time
unsigned char activeAlarmTimeHours = 0;
unsigned char activeAlarmTimeMinutes = 0;
// The actual time
unsigned char actualHours = 0;
unsigned char actualMinutes = 0;

char timeToDisplay[10];
char timeToDisplayCurrent[10];
char timeToDisplayActive[30];

// Run everyMinute() every minute / 60000ms
TimedAction timeThread = TimedAction(60000, everyMinute);

void setup()
{
  // Wait so the lcd initializes correctly
  delay(100);
  // Setup for Sound.h
  Sound();
  // Start up serial
  Serial.begin(9600);
  Serial.println("Started program");

  // Set pinmodes
  pinMode(LEFT, INPUT);
  pinMode(MIDDLE, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(LDR, INPUT);

  // Set up the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Clear buffer
  display.clearDisplay();                    
  display.setTextColor(WHITE);               
  display.setRotation(0); 
  // Wether to wrap the text to a newline or not                   
  display.setTextWrap(false);                
  // How much to dim the screen                       
  display.dim(0);

  // Turn display of as a default
  stopDisplaying();
}

void loop()
{
  // Check if a thread needs to do anything
  timeThread.check();
  // Check for button clicks
  checkLeftClick();
  checkMiddleClick();
  checkRightClick();
  // If the alarm is on, run the sing method and turn off the alarm if there is enough light
  if (isAlarmOn == true)
  {
    sing();
    if (isLight() == true)
    {
      alarmOff();
    }
  }
  // If an alarm time is set and the alarm time is the same as the current time, turn on the alarm
  else if (alarmSet == true && activeAlarmTimeMinutes == actualMinutes && activeAlarmTimeHours == actualHours)
  {
    alarmOn();
  }
  // If the display is on, check if the display should go to sleep.
  // Else: display the ui
  if (isDisplaying == true)
  {
    displayTime++;
    if (displayTime > TIMEOUT_TIME)
    {
      stopDisplaying();
      return;
    }
    display.clearDisplay(); // Clear the display so we can refresh
    display.setTextSize(0); // Set text size. We are using a custom font so you should always use the text size of 0
    // Print text:
    display.setCursor(50, 10); // (x,y);
    sprintf(timeToDisplay, "%d:%d", selectedAlarmTimeHours, selectedAlarmTimeMinutes);
    display.println(timeToDisplay); // Text or value to print
    display.setCursor(54, 20);
    display.println("Set");
    display.setCursor(100, 20);
    display.println("^ 1m");
    display.setCursor(0, 20);
    display.println("^ 1h");
    display.setCursor(0, 0);
    // if there is no time set, let the user set a time.
    if (isTimeSet == false)
    {
      sprintf(timeToDisplayActive, "Enter current time");
    } // If there is an alarm set, show the alarm time
    else if (alarmSet == true)
    {
      sprintf(timeToDisplayActive, "Current alarm: %d:%d", activeAlarmTimeHours, activeAlarmTimeMinutes);
    } // Let the user set an alarm
    else
    {
      sprintf(timeToDisplayActive, "Enter alarm time");
    }
    display.println(timeToDisplayActive);
    // If the time is set, display the current time
    if (isTimeSet == true)
    {
      display.setCursor(0, 10);
      sprintf(timeToDisplayCurrent, "%d:%d", actualHours, actualMinutes);
      display.println(timeToDisplayCurrent);
    }
    display.display();
  }
}

// Checks if the left button is clicked, if it is it wakes up the screen or runs onLeftclick()
void checkLeftClick()
{
  prevLeftClick = leftClick;
  leftClick = digitalRead(LEFT);

  if (leftClick != prevLeftClick && leftClick == 1)
  {
    if (isDisplaying == false)
    {
      startDisplaying();
    }
    else
    {
      onLeftClick();
    }
  }
}

// Checks if the middle button is clicked, if it is it wakes up the screen or runs onMiddleclick()
void checkMiddleClick()
{
  prevMiddleClick = middleClick;
  middleClick = digitalRead(MIDDLE);

  if (middleClick != prevMiddleClick && middleClick == 1)
  {
    if (isDisplaying == false)
    {
      startDisplaying();
    }
    else
    {
      onMiddleClick();
    }
  }
}

// Checks if the right button is clicked, if it is it wakes up the screen or runs onRightClick()
void checkRightClick()
{
  prevRightClick = rightClick;
  rightClick = digitalRead(RIGHT);

  if (rightClick != prevRightClick && rightClick == 1)
  {
    if (isDisplaying == false)
    {
      startDisplaying();
    }
    else
    {
      onRightClick();
    }
  }
}

// Adds 1 to the selected time hours and resets the screen timeout
void onLeftClick()
{
  displayTime = 0;
  if (selectedAlarmTimeHours == 23)
  {
    selectedAlarmTimeHours = 0;
  }
  else
  {
    selectedAlarmTimeHours++;
  }
}

// Resets the screen timeout and if no time is set set the time as the selected time, if there is a time set: set an alarm on the selected time
void onMiddleClick()
{
  displayTime = 0;
  if (isTimeSet == false)
  {
    actualHours = selectedAlarmTimeHours;
    actualMinutes = selectedAlarmTimeMinutes;
    isTimeSet = true;
  }
  else
  {
    activeAlarmTimeMinutes = selectedAlarmTimeMinutes;
    activeAlarmTimeHours = selectedAlarmTimeHours;
    alarmSet = true;
  }
}

// Resets the screen timeout and adds 1 to the selected minutes
void onRightClick()
{
  displayTime = 0;
  if (selectedAlarmTimeMinutes == 59)
  {
    selectedAlarmTimeMinutes = 0;
  }
  else
  {
    selectedAlarmTimeMinutes++;
  }
}

// Increments the time by a minute
void everyMinute()
{
  if (actualHours == 23 && actualMinutes == 59)
  {
    actualHours = 0;
    actualMinutes = 0;
  }
  else if (actualMinutes == 59)
  {
    actualHours++;
    actualMinutes = 0;
  }
  else
  {
    actualMinutes++;
  }
}

// Checks if the ldr detects light, 1 is that there is no light and 0 is that there is light
bool isLight()
{
  ldrValue = digitalRead(LDR);
  if (ldrValue == 1)
  {
    return false;
  }
  else
  {
    return true;
  }
}

// Turns off the display
void stopDisplaying()
{
  isDisplaying = false;
  displayTime = 0;
  display.clearDisplay();
  display.display();
}

// Turns on the display
void startDisplaying()
{
  isDisplaying = true;
}

// Turns on the alarm
void alarmOn()
{
  Serial.println("Souding the alarm");
  stopDisplaying();
  alarmSet = false;
  isAlarmOn = true;
}

// Turns off the alarm
void alarmOff()
{
  Serial.println("Woken up?");
  startDisplaying();
  isAlarmOn = false;
}