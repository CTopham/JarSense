
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

//------ JARS -----
float jar_x_high_positive = 3.00;
float jar_y_high_positive = 4.00;
float jar_x_high_negative = -3.00;
float jar_y_high_negative = -4.00;
//float jar_z_high_positive = 0.18;
//float jar_z_high_negative = -0.18;

float jar_x_low_positive = 5.00;
float jar_y_low_positive = 6.00;
float jar_x_low_negative = -5.00;
float jar_y_low_negative = -6.00;
//float jar_z_low_positive = 0.23;
//float jar_z_low_negative = -0.23;

//------ RESETS -----

float reset_x_high_positive = 2.00;
float reset_y_high_positive = 3.00;
float reset_x_high_negative = -2.00;
float reset_y_high_negative = -3.00;
//float reset_z_high_positive = 0.12;
//float reset_z_high_negative = -0.12;

float reset_x_low_positive = 3.00;
float reset_y_low_positive = 4.00;
float reset_x_low_negative = -3.00;
float reset_y_low_negative = -4.00;
//float reset_z_low_positive = 0.12;
//float reset_z_low_negative = -0.12;

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
  Serial.print("Filter bandwidth set to: ");
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

  Serial.println("");
  delay(1000);


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

  // Discovered a peripheral, print out address, local name, and advertised service
  if (peripheral) {
// This is the peripherals local name it sets
    if (peripheral.localName() != "IMU_Data") { 
        return;
    }
// Stop scanning
    BLE.stopScan();


//-----------------------------------
//---------------IMU READING---------
//-----------------------------------
  sensitivityToggle();
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x = a.acceleration.x;
  float y = a.acceleration.y;
  //float z = a.acceleration.z;

  String concatIMUString;

  concatIMUString = String(a.acceleration.x) + "," + String(a.acceleration.y) + "," + String(a.acceleration.z);
  //Serial.println(concatIMUString);
  keepAlive();
  if (senseToggle == false){
    if ((x > jar_x_low_positive) || (y > jar_y_low_positive) || (x < jar_x_low_negative) || (y < jar_y_low_negative)){
      Serial.println("Low Sensitivity Jar hit!"); //*************************
      jarDetected(peripheral); 
      } else if ((x > reset_x_low_positive) || (y > reset_y_low_positive) || (x < reset_x_low_negative) || (y < reset_y_low_negative)){
        Serial.println("Low Sensitivity reset hit!"); //*************************
        resetWarning(peripheral); 
        }
  } else if (senseToggle == true){
      if ((x > jar_x_high_positive) || (y > jar_y_high_positive) || (x < jar_x_high_negative) || (y < jar_y_high_negative)){
        Serial.println("High Sensitivity jar hit!"); //*************************
        jarDetected(peripheral); 
        } 
        else if ((x > reset_x_high_positive) || (y > reset_y_high_positive) || (x < reset_x_high_negative) || (y < reset_y_high_negative)){
          Serial.println("High Sensitivity reset hit!"); //*************************
          resetWarning(peripheral);  
        }
    }
      delay(500);
      BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
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
    peripheral.disconnect();
    return;
  }

  // Retrieve the IMU characteristic, this characteristic ID is set in the peripheral BLEByteCharacteristic()
  BLECharacteristic jarCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");

 
  if (!jarCharacteristic) {
      peripheral.disconnect();
      return;
      } else if (!jarCharacteristic.canWrite()) {
          peripheral.disconnect();
          return;
          }
  
  while (peripheral.connected()){ 
    // while the peripheral is connected
    Serial.println("JAR Detected, send 1 to peripheral"); //*************************
    jarCharacteristic.writeValue((byte)1);
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
  BLECharacteristic ResetCharacteristic = peripheral.characteristic("19b10002-e8f2-537e-4f6c-d104768a1214");
 
  if (!ResetCharacteristic) {
      peripheral.disconnect();
      return;
      } else if (!ResetCharacteristic.canWrite()) {
          peripheral.disconnect();
          return;
          }
  while (peripheral.connected()){ 
    // while the peripheral is connected
    Serial.println("Reset Warning, send 1 to peripheral"); //*************************
    ResetCharacteristic.writeValue((byte)1);
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

void keepAlive(){
  delay(200);
  digitalWrite(LED,HIGH);
  delay(200);
  digitalWrite(LED,LOW);
  delay(200);
  digitalWrite(LED,HIGH);
  return;
}