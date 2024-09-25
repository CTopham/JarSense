
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoBLE.h>
#include "esp_sleep.h"  // Include for ESP32 sleep functions


Adafruit_MPU6050 mpu;
#define LED 2

BLEService imuService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEDevice peripheral;  // Create a BLEDevice object
BLECharacteristic jarCharacteristic;  // Create a BLECharacteristic object
BLECharacteristic resetCharacteristic;  // Create a BLECharacteristic object

unsigned long previousMillis = 0;
unsigned long startTime = 0;


// local name: IMU_Data
// service: 19B10000-E8F2-537E-4F6C-D104768A1214
// jar characteristic: 19B10001-E8F2-537E-4F6C-D104768A1214
// reset characteristic: 19B10002-E8F2-537E-4F6C-D104768A1214


//-----------------------------------
//------------Sensitivity State------
//-----------------------------------

bool senseToggle = false;
const int sensePin = 33; // slide switch
bool calibrated = false;
float calibrated_x = NULL;
float calibrated_y = NULL;
long timeInactive; //*********************************
static unsigned long timer = millis(); //*********************************



void setup(void) {
  delay(2000);
  Serial.begin(115200); //DETERMINE BITS
  Serial.println("Turned ON");
  pinMode(sensePin, INPUT); 
  pinMode(LED,OUTPUT); // Blinks when connected


  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(1000);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  //Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  //Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  calibrater();
  delay(1000);


  // INITIALIZE BLUETOOTH
  if (!BLE.begin()) {
    Serial.println("Issue with BLE");
    while (1);
  }

  BLE.scanForUuid(imuService.uuid());
  Serial.println("Scanning for peripherals...");
  Serial.println(imuService.uuid());

  // Record the start time of the scan
  startTime = millis();
}

void loop() {
  BLE.poll();
  handleSleep();
  delay(200);

  // Check for available peripherals and connect
  if (!peripheral || !peripheral.connected()) {
    Serial.println("Trying to set peripheral to available");
    peripheral = BLE.available();

    if (peripheral) {
      BLE.stopScan();
      peripheral.connect();
      resetCurrentTime();
      Serial.println("Connected!");
      Serial.println(peripheral.discoverAttributes());
      }
    }

    // If connected, send periodic messages
    if (peripheral.connected()) { //this monitors the connection

    //Serial.println("WERE IN CONNECTED LOOP!");
    jarCharacteristic = peripheral.service(imuService.uuid()).characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
    resetCharacteristic = peripheral.service(imuService.uuid()).characteristic("19B10002-E8F2-537E-4F6C-D104768A1214");
    Serial.println(peripheral.service(imuService.uuid()).characteristic(0).canWrite());
 
    // ************** Main Logic in HERE *****************************
    BlueLightOnChip(); // indicates were connected to ble

  //-----------------------------------
  //---------------IMU READING---------
  //-----------------------------------
  sensitivityToggle();

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x = fabs(a.acceleration.x);
  float y = fabs(a.acceleration.y);
  //float z = a.acceleration.z;

  String concatIMUString;

  concatIMUString = String(a.acceleration.x) + "," + String(a.acceleration.y) + "," + String(a.acceleration.z);
  Serial.println(concatIMUString);
  Serial.println("The X value is " + String(x) +  " The compare value is " + String(Compare(calibrated_x,.25,"add")));


    // Y is the jar
    // X is the Reset
    if (senseToggle == false){
      if ((x > Compare(calibrated_x,1.00,"add")) || (y > Compare(calibrated_y,0.90,"add")) || (x < Compare(calibrated_x,1.00,"subtract")) || (y < Compare(calibrated_y,1.05,"subtract"))){
        Serial.println("Low Sensitivity Jar hit!"); //*************************
        jarDetected(); 
        } else if ((x > Compare(calibrated_x,0.80,"add")) || (y > Compare(calibrated_y,1.00,"add")) || (x < Compare(calibrated_x,0.90,"subtract")) || (y < Compare(calibrated_y,1.00,"subtract"))){
          Serial.println("Low Sensitivity reset hit!"); //*************************
         resetWarning(); 
          }
    } else if (senseToggle == true){
        if ((x > Compare(calibrated_x,0.90,"add")) || (y > Compare(calibrated_y,0.80,"add")) || (x < Compare(calibrated_x,0.90,"subtract")) || (y < Compare(calibrated_y,0.95,"subtract"))){
          Serial.println("High Sensitivity jar hit!"); //*************************
          jarDetected(); 
          } 
          else if ((x > Compare(calibrated_x,0.70,"add")) || (y > Compare(calibrated_y,0.90,"add")) || (x < Compare(calibrated_x,0.80,"subtract")) || (y < Compare(calibrated_y,0.90,"subtract"))){
            Serial.println("High Sensitivity reset hit!"); //*************************
            resetWarning();  
          }
      }
      delay(200);
      }
      else{
        Serial.println("Disconnected from peripheral, restarting scan...");
        BLE.scanForUuid(imuService.uuid());
      }

} // end of loop


//-------------------------------------------
//-----------JAR DETECTED--------------------
//-------------------------------------------

void jarDetected(){
      const char* message = "Jar";
    if (jarCharacteristic.writeValue(message)) {
        Serial.println("Message sent to peripheral: Jar");
    } else {
        Serial.println("Failed to write to characteristic");
    }
  }

//-------------------------------------------
//-----------RESET WARNING-------------------
//-------------------------------------------
  void resetWarning(){
    const char* message = "Reset";
    if (resetCharacteristic.writeValue(message)) {
        Serial.println("Message sent to peripheral: Reset");
    } else {
        Serial.println("Failed to write to characteristic");
    }

  }

//-------------------------------------------
//-----------Sensitivity---------------------
//-------------------------------------------
void sensitivityToggle(){
  if(digitalRead(sensePin) == HIGH){
    senseToggle ? NULL : Serial.println("Sensitivity On");
    senseToggle = true;
    return;
  }else{
    senseToggle ? Serial.println("Sensitivity Off") : NULL;
    senseToggle = false;
    return;
  }
}

// we turn the one when were connected
void BlueLightOnChip(){
  delay(300);
  digitalWrite(LED,HIGH);
  delay(300);
  digitalWrite(LED,LOW);
  delay(300);
  digitalWrite(LED,HIGH);
  return;
}

void calibrater(){
  sensors_event_t a, g, temp;
  float x;
  float y;

  //float z = a.acceleration.z;

  if(calibrated == false){
    delay(15000);
    float x_array[5];
    float y_array[5];
    for (int i = 0 ; i < 5 ; i++){
      mpu.getEvent(&a, &g, &temp);
      x = fabs(a.acceleration.x);
      y = fabs(a.acceleration.y);
      Serial.println("Storing X ");
      Serial.println(x);
      Serial.println("Storing Y ");
      Serial.println(y);
      x_array[i] = x;
      y_array[i] = y;
      delay(500);
    } 
    calibrated = true;
    calibrated_x = calculateAverage(x_array, 5);
    calibrated_y = calculateAverage(y_array, 5);
    Serial.println(" --- calibrated_x = ");
    Serial.println(calibrated_x);
        Serial.println(" --- calibrated_y = ");
    Serial.println(calibrated_y);

  } 
}

float calculateAverage (float values[], int size) {
  float sum = 0.0;  // sum will be larger than an item, long for safety.
  for (int i = 0; i < size; i++){
    sum += values[i];
  }return sum / size;
}

float Compare(float val,float perc,String addOrSub){
float value;
{
  //example 100 + 100 * .15 returns 115'
  if (addOrSub == "add"){
    value = val + (val * perc);
  }
  else if (addOrSub == "subtract"){
     value = val - (val * perc);
  }
  return value;
 }
}

//

void resetCurrentTime(){
  timeInactive = millis() - timer;
  return;
}

// Function to handle sleep logic
void handleSleep() {
    long currentTime = millis() - timer; // set current time
  //Serial.println("current Time " + String(currentTime));
  //Serial.println("timeInactive " + String(timeInactive));
    if(currentTime - timeInactive >= 15 * 60000){// if timeInactive > 10 min
    // place in sleep mode
    //Serial.println("Going to Sleep for 1 minute");
    esp_sleep_enable_timer_wakeup(60000000);  // Set the wakeup interval 1 minutes
    delay(1000);
    Serial.flush(); 
    esp_light_sleep_start();  // Enter light sleep
    Serial.println("Woke up from light sleep!");
    delay(1000);
    return;
  } else{
    // scan for uuid
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
    return;
  }
}




