/*
  AUTHOR = CRAIG TOPHAM

 Last update 7/9 11:17pm

  Arduino BMI270 - Simple Accelerometer

  This example reads the acceleration values from the BMI270
  sensor and continuously prints them to the Serial Monitor
  or Serial Plotter.

  The circuit:
  - Arduino Nano 33 BLE Sense Rev2
*/

// Peripheral is the Chip
// Central is the Phone (Cenral is the server)
// Peripheral just connects and writes notifies. (peripheral = Client)
// Each chip will have a unique Serviceuuid that we need to pass into the iphone app 

// Set IMU x,y,z as a BLEcharacteristic
// Update that BLEcharacteristic in a loop
// have the characteristic notify central when it changes
// have the phone app be the central and read the info, possibly use a poll

#include "Arduino_BMI270_BMM150.h"
#include <ArduinoBLE.h>

void setup() {
  Serial.begin(9600); //DETERMINE BITS
  
  if (!IMU.begin()) {  // INITIALIZE ACCELEROMETER
    while (1);
  }

// INITIALIZE BLUETOOTH
  if (!BLE.begin()) {
    while (1);
  }
  
    // Start scanning for peripherals
  BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
  
}

void loop() {

  // Check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();
  //Serial.println(peripheral);

  // Discovered a peripheral, print out address, local name, and advertised service
  if (peripheral) {
// This is the peripherals local name it sets
    if (peripheral.localName() != "IMU_Data") { 
        return;
    }
// Stop scanning
    BLE.stopScan();

    float x, y, z;
    String concatIMUString;
    if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(x, y, z);
        concatIMUString = String(x) + "," + String(y) + "," + String(z);
      //  Serial.println(concatIMUString);
        if ((x > 0.15) || (y > 0.15) || (x < -0.15) || (y < -0.15)){
        // update characteristic and notify central
        jarDetected(peripheral);
        }
        else if ((x > 0.07) || (y > 0.07) || (x < -0.07) || (y < -0.07)){
          resetWarning(peripheral);
        }

    }
    delay(200);

  // Peripheral disconnected, start scanning again
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }

}

//-------------------------------------------
//-----------JAR DETECTED-------------------
//-------------------------------------------

void jarDetected(BLEDevice peripheral){

  if (peripheral.connect()) {
  } else {
    return;
  }

  // Discover peripheral attributes
  if (peripheral.discoverAttributes()) {
  } else {
    peripheral.disconnect();
    return;
  }

  // Retrieve the IMU characteristic, this characteristic ID is set in the peripheral BLEByteCharacteristic()
  BLECharacteristic IMUCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
 
  if (!IMUCharacteristic) {
      peripheral.disconnect();
      return;
      } else if (!IMUCharacteristic.canWrite()) {
          peripheral.disconnect();
          return;
          }
  
  while (peripheral.connected()){ 
    // while the peripheral is connected
    //Serial.println("JAR Detected, send 1 to peripheral");
    IMUCharacteristic.writeValue((byte)1);
    peripheral.disconnect();
    }
  }

//-------------------------------------------
//-----------RESET WARNING-------------------
//-------------------------------------------
  void resetWarning(BLEDevice peripheral){

  if (peripheral.connect()) {
  } else {
    return;
  }

  // Discover peripheral attributes
  if (peripheral.discoverAttributes()) {
  } else {
    peripheral.disconnect();
    return;
  }

  // Retrieve the IMU characteristic, this characteristic ID is set in the peripheral BLEByteCharacteristic()
  BLECharacteristic IMUCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
 
  if (!IMUCharacteristic) {
      peripheral.disconnect();
      return;
      } else if (!IMUCharacteristic.canWrite()) {
          peripheral.disconnect();
          return;
          }
  while (peripheral.connected()){ 
    // while the peripheral is connected
    //Serial.println("Reset Warning, send 2 to peripheral");
    IMUCharacteristic.writeValue((byte)2);
    peripheral.disconnect();
    }
  }

