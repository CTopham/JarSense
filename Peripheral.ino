// FINAL PERIPHERAL
//ESP32 Dev Module
#include <Arduino.h>
#include <ArduinoBLE.h>
#include <OneButton.h>
#include "DFRobotDFPlayerMini.h"
#include "esp_sleep.h"  // Include for ESP32 sleep functions

bool silence = true;
bool jarRecap = false;
static unsigned long timer = millis();
long timeInactive;
long jarTime = 0;
bool loopBreaker = false;

#define LED 2
const int senseLEDPin = 25; // red -- change to jarpin
const int recapLEDPin = 26; // blue --- change to resetpin
const int recapButtonPin = 13;

#define RXD2 16
#define TXD2 17

DFRobotDFPlayerMini player;

BLEService imuService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service
BLEByteCharacteristic jarCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify| BLEWrite);
BLEByteCharacteristic resetCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify | BLEWrite);


OneButton recapButton(recapButtonPin, true);

void setup() {
// put your setup code here, to run once:
  delay(2000);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(5000);

// Start sound player
  player.begin(Serial2);
  if(!player.begin(Serial2)){
    delay(1000);
    Serial.println("Not Connected!");
    while(true);
    };

// LED Setup
  pinMode(senseLEDPin, OUTPUT); // Sensitivity LED.. D25
  pinMode(recapLEDPin, OUTPUT); // Sensitivity LED.. D26
  pinMode(LED,OUTPUT); // Blue on board Blinks when connected


// Button setup
  recapButton.attachClick(recapSingleClick);
  recapButton.attachLongPressStop(recapLongClick);

// Start BLE
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

// Setup BLE
  BLE.setLocalName("IMU_Data");
  BLE.setAdvertisedService(imuService);
  imuService.addCharacteristic(jarCharacteristic);
  imuService.addCharacteristic(resetCharacteristic);
  BLE.addService(imuService);
  jarCharacteristic.setEventHandler(BLEWritten, onJarMessageReceived);
  resetCharacteristic.setEventHandler(BLEWritten, onResetMessageReceived);

// Advertise
  BLE.advertise();
  Serial.println("BLE IMU Peripheral Advertising imuService with name IMU_DATA");

}

void loop() {
  Serial.println("Top of Loop");
  delay(100);
  recapButton.tick();// Watcher - button clicks
  timerCheck(); // Check if we had a jar within 10 seconds to set boolean
  handleSleep(); // Check for sleep condition

  BLEDevice central = BLE.central();
  if (central) {
    BlueLightOnChip();
    resetCurrentTime();
    while (central.connected()) { 
      delay(100);
      Serial.println("In While Loop");
      recapButton.tick();// Watcher - button clicks
      BLE.poll(); // Handle BLE events
      Serial.println(loopBreaker);
      if (checkAndExitLoop()) { // Check if you need to break the loop
        break; // Break the loop based on the return value of shouldExitLoop
      }
    }
  }
}

//------------------------------------------------------
//-------------------Utility-----------------------------
//------------------------------------------------------

//Recap single click silence the audio notification system
void recapLongClick() {
  Serial.println("Recap Button Pressed");
  if(silence == false){
    playSound(5); // play quiet mode notification
    silence = true;
    digitalWrite(recapLEDPin,HIGH); // light on indicates device is silenced
    return;
  }else{
    playSound(9); 
    silence = false; // set it back to verbose.
    digitalWrite(recapLEDPin,LOW); // turn the blue led off to indicate verbose mode
    return;
  }
}


//Recap Long click identifies if a jar occurred within the last 10 seconds
void recapSingleClick(){
  Serial.println("Recap Long Button Pressed");
  jarRecap == true ? playSound(1) : playSound(11);
  return;
}

void playSound(int songVal) {
  /*
  ------New----
  1 jar detected within 10 sec
  3 attn jar detected
  5 quiet mode set
  7 Reset Warning
  9 Verbose mode activated
  11 no jar detected
  13 power saving mode
  */
  player.setTimeOut(500);
  player.volume(23);
  try{
    player.play(songVal);
    return;
  }
  catch(String error){
    Serial.println("Issue with playing song");
    printDetail(player.readType(), player.read());
  }
}

void blinkAction(String actionType){ // action type is either jar or reset
if(actionType == "jar"){
  digitalWrite(senseLEDPin,HIGH);
  delay(250);
  digitalWrite(senseLEDPin,LOW);
  delay(250);
    digitalWrite(senseLEDPin,HIGH);
  delay(250);
  digitalWrite(senseLEDPin,LOW);
  delay(250);
    digitalWrite(senseLEDPin,HIGH);
  delay(250);
  digitalWrite(senseLEDPin,LOW);
  silence == true ? digitalWrite(recapLEDPin,HIGH) : digitalWrite(recapLEDPin,LOW);
  delay(250);
  return;
}
if (actionType == "reset"){
  digitalWrite(recapLEDPin,HIGH);
  delay(250);
  digitalWrite(recapLEDPin,LOW);
  delay(250);
    digitalWrite(recapLEDPin,HIGH);
  delay(250);
  digitalWrite(recapLEDPin,LOW);
  delay(250);
    digitalWrite(recapLEDPin,HIGH);
  delay(250);
  digitalWrite(recapLEDPin,LOW);
  delay(250);
  silence == true ? digitalWrite(recapLEDPin,HIGH) : digitalWrite(recapLEDPin,LOW);
  return;
}
}

void timerCheck(){
  long currentTime = millis() - timer; // set current time
  if (jarRecap == true) {
    if(currentTime - jarTime >= 15000){
    jarRecap = false; // set the jar recap back to false
    //Serial.println("Jar Recap set back to False"); 
    return;
  }
}
}


void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

// Function to handle when a message is received
void onJarMessageReceived(BLEDevice device, BLECharacteristic characteristic) {
    String message = String((const char*)characteristic.value());
    Serial.print("Received message: ");
    Serial.println(message);

    if (silence == true){
      Serial.println("Silenced Jar");
      }else{
        playSound(3);
        }
    jarRecap = true;
    jarTime = millis() - timer; // time stamp the time of the jar, to check for 15 sec passing
    blinkAction("jar"); // blink led pin 25
    digitalWrite(senseLEDPin,LOW); // lightindicates device is disconnected
    loopBreaker = true;
}

void onResetMessageReceived(BLEDevice device, BLECharacteristic characteristic) {
    String message = String((const char*)characteristic.value());
    Serial.print("Received message: ");
    Serial.println(message);
  
    if (silence == true){
      Serial.println("Silenced Reset");
      }else{
        playSound(7);
        }
    blinkAction("reset"); // blink led pin 26
    digitalWrite(senseLEDPin,LOW); // lightindicates device is disconnected
    loopBreaker = true;
}

void resetCurrentTime(){
  timeInactive = millis() - timer;
  return;
}

// Function to handle sleep logic
void handleSleep() {
    long currentTime = millis() - timer; // set current time
  //Serial.println("current Time " + String(currentTime));
  //Serial.println("timeInactive " + String(timeInactive));
    if(currentTime - timeInactive >= 10 * 60000){// if timeInactive > 1 min
    // place in sleep mode
    Serial.println("Going to Sleep for 1 minute");
    playSound(13);
    digitalWrite(LED,LOW);
    esp_sleep_enable_timer_wakeup(60000000);  // Set the wakeup interval 1 minutes
    delay(1000);
    Serial.flush(); 
    esp_light_sleep_start();  // Enter light sleep
    Serial.println("Woke up from light sleep!");
    delay(1000);
  } else{
    // scan for uuid
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
    return;
  }
}

// we turn the one when were connected
void BlueLightOnChip(){
  delay(300);
  digitalWrite(LED,HIGH);
  return;
}

bool checkAndExitLoop() {
  // Add a condition to break out of the while loop
  if (loopBreaker == true) {
    loopBreaker = false;
    Serial.println("Breaking out of while loop...");
    return true; // This breaks the nearest loop, in this case the while(central.connected())
  }
  else{
    return false;
  }
}


