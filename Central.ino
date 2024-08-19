
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoBLE.h>

Adafruit_MPU6050 mpu;
#define LED 2

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
  Serial.begin(115200); //DETERMINE BITS
  pinMode(sensePin, INPUT); 
  pinMode(LED,OUTPUT); // keep alive thing


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
  
    // Start scanning for peripherals
  BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
}

void loop() {

  // Check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  // Discovered a peripheral, print out address, local name, and advertised service
  if (peripheral) {
    Serial.println("Connected!");
// This is the peripherals local name it sets
    if (peripheral.localName() != "IMU_Data") { 
        return;
    }
// Stop scanning
    BLE.stopScan();

// Zero out the timeInactive var*********************************
resetCurrentTime();
BlueLightOnChip();

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
  //Serial.println(concatIMUString);
  //Serial.println("The X value is " + String(x) +  " The compare value is " + String(Compare(calibrated_x,.25,"add")));


  if (senseToggle == false){
    if ((x > Compare(calibrated_x,1.00,"add")) || (y > Compare(calibrated_y,1.00,"add")) || (x < Compare(calibrated_x,1.00,"subtract")) || (y < Compare(calibrated_y,1.00,"subtract"))){
      Serial.println("Low Sensitivity Jar hit!"); //*************************
      jarDetected(peripheral); 
      } else if ((x > Compare(calibrated_x,0.90,"add")) || (y > Compare(calibrated_y,0.90,"add")) || (x < Compare(calibrated_x,0.90,"subtract")) || (y < Compare(calibrated_y,0.90,"subtract"))){
        Serial.println("Low Sensitivity reset hit!"); //*************************
        resetWarning(peripheral); 
        }
  } else if (senseToggle == true){
      if ((x > Compare(calibrated_x,0.90,"add")) || (y > Compare(calibrated_y,0.90,"add")) || (x < Compare(calibrated_x,0.90,"subtract")) || (y < Compare(calibrated_y,0.90,"subtract"))){
        Serial.println("High Sensitivity jar hit!"); //*************************
        jarDetected(peripheral); 
        } 
        else if ((x > Compare(calibrated_x,0.80,"add")) || (y > Compare(calibrated_y,0.80,"add")) || (x < Compare(calibrated_x,0.80,"subtract")) || (y < Compare(calibrated_y,0.80,"subtract"))){
          Serial.println("High Sensitivity reset hit!"); //*************************
          resetWarning(peripheral);  
        }
    }
      delay(100);
      BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
  } else {
    //Serial.println("Were Not Connected to the peripheral");
    delay(100);
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
    timerCheck(); // ******************************************
  }
}

//-------------------------------------------
//-----------JAR DETECTED--------------------
//-------------------------------------------

void jarDetected(BLEDevice peripheral){

  if (peripheral.connect()) {
  } else {
    return;
  }

  // Discover peripheral attributes
  if (peripheral.discoverAttributes()) {
  } else {
    digitalWrite(LED,LOW);
    peripheral.disconnect();
    return;
  }

  // Retrieve the IMU characteristic, this characteristic ID is set in the peripheral BLEByteCharacteristic()
  BLECharacteristic jarCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");

 
  if (!jarCharacteristic) {
      peripheral.disconnect();
      return;
      } else if (!jarCharacteristic.canWrite()) {
          digitalWrite(LED,LOW);
          peripheral.disconnect();
          return;
          }
  
  while (peripheral.connected()){ 
    // while the peripheral is connected
    Serial.println("JAR Detected, send 1 to peripheral"); //*************************
    jarCharacteristic.writeValue((byte)1);
    digitalWrite(LED,LOW);
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
    digitalWrite(LED,LOW);
    peripheral.disconnect();
    return;
  }

  // Retrieve the IMU characteristic, this characteristic ID is set in the peripheral BLEByteCharacteristic()
  BLECharacteristic ResetCharacteristic = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
 
  if (!ResetCharacteristic) {
      digitalWrite(LED,LOW);
      peripheral.disconnect();
      return;
      } else if (!ResetCharacteristic.canWrite()) {
          digitalWrite(LED,LOW);
          peripheral.disconnect();
          return;
          }
  while (peripheral.connected()){ 
    // while the peripheral is connected
    Serial.println("Reset Warning, send 1 to peripheral"); //*************************
    ResetCharacteristic.writeValue((byte)1);
    digitalWrite(LED,LOW);
    peripheral.disconnect();
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
    value = val + val * perc;
  }
  else if (addOrSub == "subtract"){
     value = val - val * perc;
  }
  return value;
 }
}

// ******************************************
void resetCurrentTime(){
  timeInactive = millis() - timer;
  return;
}
// ******************************************
void timerCheck(){
  long currentTime = millis() - timer; // set current time
  //Serial.println("current Time " + String(currentTime));
  //Serial.println("timeInactive " + String(timeInactive));
    if(currentTime - timeInactive >= 25000){// if timeInactive > 60 second
      // place in sleep mode
    Serial.println("Going to Sleep for 1 minute");
    esp_sleep_enable_timer_wakeup(60 * 1000000);
    delay(1000);
    Serial.flush(); 
    esp_deep_sleep_start();
  } else{
    // scan for uuid
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
    return;
  }
}