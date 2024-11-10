#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <SimpleKalmanFilter.h>
#include "WiFi.h"
#include "ESPAsyncWebSrv.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#define OUTPUT_READABLE_REALACCEL

MPU6050 mpu;
int16_t vibrationValue = 0;
// Replace with your network credentials
const char* ssid = "Anhson";
const char* password = "0978915672";
int const INTERRUPT_PIN = 2;  // Define the interruption #0 pin
SimpleKalmanFilter simpleKalmanFilter(2, 2, 0.01);

AsyncWebServer server(80);
/*---MPU6050 Control/Status Variables---*/
bool DMPReady = false;  // Set true if DMP init was successful
uint8_t MPUIntStatus;   // Holds actual interrupt status byte from MPU
uint8_t devStatus;      // Return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // Expected DMP packet size (default is 42 bytes)
uint8_t FIFOBuffer[64]; // FIFO storage buffer

/*---Orientation/Motion Variables---*/ 
Quaternion q;           // [w, x, y, z]         Quaternion container
VectorInt16 aa;         // [x, y, z]            Accel sensor measurements
VectorInt16 gy;         // [x, y, z]            Gyro sensor measurements
VectorInt16 aaReal;     // [x, y, z]            Gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            World-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            Gravity vector

const int numSamples = 10;
int16_t axSamples[numSamples];
int16_t aySamples[numSamples];
int16_t azSamples[numSamples];

int counting = 0;
/*------Interrupt detection routine------*/
volatile bool MPUInterrupt = false;     // Indicates whether MPU6050 interrupt pin has gone high
void DMPDataReady() {
  MPUInterrupt = true;
}

void setup() {
  Wire.begin();
    Wire.setClock(400000);
  Serial.begin(115200); //115200 is required for Teapot Demo output
  while (!Serial);

  /*Initialize device*/
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  /*Verify connection*/
  Serial.println(F("Testing MPU6050 connection..."));
  if(mpu.testConnection() == false){
    Serial.println("MPU6050 connection failed");
    while(true);
  }
  else {
    Serial.println("MPU6050 connection successful");
  }

  /* Initialize and configure the DMP*/
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  /* Supply your gyro offsets here, scaled for min sensitivity */
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(-7);
  mpu.setZGyroOffset(35);
  mpu.setXAccelOffset(14);
  mpu.setYAccelOffset(-1329);
  mpu.setZAccelOffset(1096);

  /* Making sure it worked (returns 0 if so) */ 
  if (devStatus == 0) {
    mpu.CalibrateAccel(6);  // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateGyro(6);
    Serial.println("These are the Active offsets: ");
    mpu.PrintActiveOffsets();
    Serial.println(F("Enabling DMP..."));   //Turning ON DMP
    mpu.setDMPEnabled(true);

    /*Enable Arduino interrupt detection*/
    Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
    Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
    Serial.println(F(")..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), DMPDataReady, RISING);
    MPUIntStatus = mpu.getIntStatus();

    /* Set the DMP Ready flag so the main loop() function knows it is okay to use it */
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    DMPReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize(); //Get expected DMP packet size for later comparison
  } 
  else {
    Serial.print(F("DMP Initialization failed (code ")); //Print the error code
    Serial.print(devStatus);
    Serial.println(F(")"));
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
  }
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Route to load style.css file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/styles.css", "text/css");
    Serial.println("CSS file requested");
  });

  // Route to load JavaScript file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "application/javascript");
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204); // No Content response
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument jsonDoc(200);
    jsonDoc["value"] = vibrationValue; // Replace with real sensor value
    jsonDoc["pump2"] = vibrationValue*0.1; 
    jsonDoc["pump3"] = vibrationValue*10; 

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

float calculateRMS(int16_t values[]) {
    float sumSquares = 0.0;
    for (int i = 0; i < numSamples; i++) {
        sumSquares += values[i] * values[i];
    }
    return sqrt(sumSquares / numSamples);
}

void loop() {
  if (!DMPReady) return; // Stop the program if DMP programming fails.
    
  /* Read a packet from FIFO */
  if (mpu.dmpGetCurrentFIFOPacket(FIFOBuffer)) { // Get the Latest packet 
   if(counting < 10){
      mpu.dmpGetQuaternion(&q, FIFOBuffer);
      mpu.dmpGetAccel(&aa, FIFOBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
      axSamples[counting] = aaReal.x;
      aySamples[counting] = aaReal.y;
      azSamples[counting] = aaReal.z;
      counting++;
      if(counting == 10){
        float rmsX = calculateRMS(axSamples);
        float rmsY = calculateRMS(aySamples);
        float rmsZ = calculateRMS(azSamples);

        // Calculate overall RMS vibration
        float rmsVibration = sqrt(rmsX * rmsX + rmsY * rmsY + rmsZ * rmsZ);
        float kalRead = simpleKalmanFilter.updateEstimate(rmsVibration);
        vibrationValue = kalRead;
        // Output RMS vibration values
        //Serial.print("RMS Vibration: ");
        //Serial.println(kalRead);
        counting = 0;
      }
   }
  }
}
