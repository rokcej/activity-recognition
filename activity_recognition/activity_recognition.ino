#include <Wire.h>
#include <Ticker.h>

// WiFi
#define NETWORK_SSID "Zoxx"
#define NETWORK_PASSWORD "09889576"
#define NETWORK_MODE 1 // Access point = 0, Station = 1

// MPU-9250
#define I2C_ADD_PCF 0x20 // PCF8574AN I2C Address
#define I2C_ADD_MPU 0x68 // MPU-9250 I2C Address
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43

// Constants
const float G = 9.80665f;

const int FREQUENCY = 100;
const float TIME_CONSTANT = 1.f;

const float DT = 1.f / FREQUENCY;
const float ALPHA = TIME_CONSTANT / (TIME_CONSTANT + DT);


// Global variables
Ticker tickerMain;
Ticker tickerLed;
float rot[2] = { 0.f };
float acc_err[3] = { 0.f };
float gyro_err[3] = { 0.f };
int led_count = 0;



// Function declarations
void setupServer();
void loopServer();

float rad2deg(float angle) {
    return angle * 180.f / M_PI;                                
}

void writeLed(int count) {
    uint8_t val = 0;
    for (int i = 0; i < count; ++i)
        val |= 1 << i;
    Wire.beginTransmission (I2C_ADD_PCF);
    Wire.write(~val);
    Wire.endTransmission();
}

void calibrate() {
  for (int i = 0; i < 3; ++i) {
    acc_err[i] = 0.f;
    gyro_err[i] = 0.f;
  }

  Serial.print("Calibrating, please don't move the board for a few seconds... ");
  int samples = 300;
  int frequency = 100;
  for (int i = 0; i < samples; ++i) {
    
    float acc[3], ang_vel[3];
    readAcc(acc);
    readGyro(ang_vel);

    for (int i = 0; i < 3; ++i) {
      acc_err[i] -= acc[i];
      gyro_err[i] -= ang_vel[i];
    }

    delay(1000 / frequency);
  }
  Serial.println("Done!");

  for (int i = 0; i < 3; ++i) {
    acc_err[i] /= (float)samples;
    gyro_err[i] /= (float)samples;
  }

  acc_err[2] += G;
}

void readAcc(float acc[3]) {
  Wire.beginTransmission(I2C_ADD_MPU);
  Wire.write(ACCEL_XOUT_H);
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADD_MPU, 6);
  for (int i = 0; i < 3; ++i) {
    int16_t acc_out = (Wire.read() << 8) | Wire.read();
    acc[i] = (float)acc_out * (2.f * G) / (1 << 15);
  }
}

void readGyro(float ang_vel[3]) {
  Wire.beginTransmission(I2C_ADD_MPU);
  Wire.write(GYRO_XOUT_H);
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADD_MPU, 6);
  for (int i = 0; i < 3; ++i) {
    int16_t ang_vel_out = (Wire.read() << 8) | Wire.read();
    ang_vel[i] = (float)ang_vel_out / 131.f; // GYRO_SENSITIVITY = 131 s/deg = 
  }
}

void updateLed() {
    led_count = led_count % 4 + 1;
    writeLed(led_count);
}

void updateMain() {
  float acc[3], ang_vel[3];
  readAcc(acc);
  readGyro(ang_vel);
  for (int i = 0; i < 3; ++i) {
    acc[i] += acc_err[i];
    ang_vel[i] += gyro_err[i];
  }

  float rot_acc[2] = {
    rad2deg(atan2f(acc[1], acc[2])), // Roll
    rad2deg(atan2f(-acc[0], sqrtf(acc[1] * acc[1] + acc[2] * acc[2]))) // Pitch
  };

  for (int i = 0; i < 2; ++i)
    rot[i] = ALPHA * (rot[i] + ang_vel[i] * DT) + (1.f - ALPHA) * (rot_acc[i]);
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  delay(500);

  // Initialize I2C
  Wire.begin(12, 14); // SDA = 12, SCL = 14
  Wire.setClock(100000); // 100kHz

  tickerLed.attach_ms(500, updateLed); // Start loading animation

  // Init server
  setupServer();

  // Run activity recognition
  calibrate();
  tickerMain.attach_ms(1000 / FREQUENCY, updateMain);
  
  tickerLed.detach(); // Stop loading animation
  writeLed(0);
}

void loop() {
  loopServer();
}
