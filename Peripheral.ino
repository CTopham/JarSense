//ESP32 Dev Module
#include <Arduino.h>
#include <ArduinoBLE.h>
#include <OneButton.h>
#include "DFRobotDFPlayerMini.h"

const int senseLEDPin = 25;
const int recapLEDPin = 26;
const int senseButtonPin = 12;
const int recapButtonPin = 13;

#define RXD2 16
#define TXD2 17

DFRobotDFPlayerMini player;

BLEService imuService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service
BLEByteCharacteristic jarCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic ResetCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic SensitivityCharacteristic("19B10003-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

OneButton senseButton(senseButtonPin, true);
OneButton QuietButton(recapButtonPin, true);


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

// Button setup
  senseButton.attachClick(senseSingleClick);
  QuietButton.attachClick(recapSingleClick);


// Start BLE
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

// Setup BLE
  BLE.setLocalName("IMU_Data");
  BLE.setAdvertisedService(imuService);
  imuService.addCharacteristic(jarCharacteristic);
  imuService.addCharacteristic(ResetCharacteristic);
  imuService.addCharacteristic(SensitivityCharacteristic);
  BLE.addService(imuService);
  jarCharacteristic.writeValue(0);
  ResetCharacteristic.writeValue(0);
  SensitivityCharacteristic.writeValue(0);
  BLE.advertise();

  Serial.println("BLE IMU Peripheral Advertising imuService with name IMU_DATA");

}

void loop() {
  senseButton.tick(); // check the button
  QuietButton.tick();
  delay(100);
  BLEDevice central = BLE.central();

    if (central) {
    while (central.connected()) {
      int newJarState = jarCharacteristic.value();
      if (newJarState != 0){
        Serial.println("JAR Characteristic value is: ");
        Serial.println(jarCharacteristic.value());
        jarCharacteristic.writeValue(0);
        central.disconnect();
    }
    int newResetState = ResetCharacteristic.value();
    if (newResetState != 0){
        Serial.println("RESET Characteristic value is: ");
        Serial.println(ResetCharacteristic.value());
        ResetCharacteristic.writeValue(0);
        central.disconnect();

    }
  }
}

}

//------------------------------------------------------
//-------------------Utility-----------------------------
//------------------------------------------------------

  void senseSingleClick(){
    Serial.println("Sense Button Pressed");
    playSound(2);
    digitalWrite(senseLEDPin,HIGH);
    delay(1000);
    digitalWrite(senseLEDPin,LOW);

    /*
    int currentSenseState = SensitivityCharacteristic.value();
    // if currentState 0 then set to 1 - Change the LED ON, Announce on speaker sensitivity set to high
    if(currentSenseState == 0){ // if currentState 0 then set to 1
      SensitivityCharacteristic.writeValue(1);
        // Change the LED ON
      digitalWrite(senseLED,HIGH);
        // Announce on speaker sensitivity set to high
        // if currenState 1 set to 0 - Change the LED OFF, Announce on speakerb sensitivity normal
      }
      */
}


void recapSingleClick() {
  Serial.println("Recap Button Pressed");
    digitalWrite(recapLEDPin,HIGH);
    delay(1000);
    digitalWrite(recapLEDPin,LOW);
}

void playSound(int songVal) {
  /*
  2 Sensitivity high
  4 Sensitivity low
  6 Quiet mode
  8 Reset warning
  9 Attention jar detected
  11 15 sec jar
  */
  player.setTimeOut(500);
  player.volume(23);
  try{
    player.play(songVal);
  }
  catch(String error){
    Serial.println("Issue with playing song");
    printDetail(player.readType(), player.read());
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