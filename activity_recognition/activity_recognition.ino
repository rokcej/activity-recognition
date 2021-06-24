#include <Wire.h>
#include <Ticker.h>
#include <math.h>

// WiFi
#define NETWORK_SSID "Zoxx"
#define NETWORK_PASSWORD "09889576"
#define NETWORK_MODE 1 // Access point = 0, Station = 1

// MPU-9250
#define I2C_ADD_PCF 0x20 // PCF8574AN I2C Address
#define I2C_ADD_MPU 0x68 // MPU-9250 I2C Address
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43

#define HISTORY 40

// Constants
const float G = 9.80665f;

const int FREQUENCY = 20;
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

float acc_hist[6][HISTORY] = { 0.f };
float acc_hist_dev[6][HISTORY] = { 0.f };
int   acc_i = 0;

int activity = 0; // 0 = None, 1 = Walking, 2 = Jumping

unsigned int current_step = 0; // Counter for history sync

// Function declarations
void setupServer();
void loopServer();

float rad2deg(float angle) {
    return angle * (180.f / M_PI);
}
float deg2rad(float angle) {
    return angle * (M_PI / 180.f);  
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
  // Read IMUs
  float acc[3], ang_vel[3];
  readAcc(acc);
  readGyro(ang_vel);
  for (int i = 0; i < 3; ++i) {
    acc[i] += acc_err[i];
    ang_vel[i] += gyro_err[i];
  }

  // Get orientation
  float rot_acc[2] = {
    rad2deg(atan2f(acc[1], acc[2])), // Roll
    rad2deg(atan2f(-acc[0], sqrtf(acc[1] * acc[1] + acc[2] * acc[2]))) // Pitch
  };
  for (int i = 0; i < 2; ++i)
    rot[i] = ALPHA * (rot[i] + ang_vel[i] * DT) + (1.f - ALPHA) * (rot_acc[i]);


//    // Correct acc data (remove G component)
//    float roll = deg2rad(rot[0]);
//    float pitch = deg2rad(rot[1]);
//    float cos_pitch = cosf(pitch);
//    float g_rel[3] = {
//        -sinf(pitch),
//        sinf(roll) * cos_pitch,
//        cosf(roll) * cos_pitch
//    };

    // Get absolute acc (absolute directions, not absolute value)
    float roll = deg2rad(rot[0]);
    float pitch = deg2rad(rot[1]);
    float sin_roll = sinf(roll);
    float cos_roll = cosf(roll);
    float sin_pitch = sinf(pitch);
    float cos_pitch = cosf(pitch);
    float acc_abs[3] = {
         acc[0] * cos_pitch + acc[1] * sin_pitch * cos_roll + acc[2] * sin_pitch * cos_roll,
                              acc[1] * cos_roll             - acc[2] * sin_roll,
        -acc[0] * sin_pitch + acc[1] * cos_pitch * sin_roll + acc[2] * cos_pitch * cos_roll
    };

    // Save acc data
    for (int i = 0; i < 3; ++i) {
        acc_hist[i][acc_i] = acc[i];
        acc_hist[i+3][acc_i] = acc_abs[i];
    }

    // Compute mean
    float acc_mean[6] = { 0.f };
    for (int i = 0; i < 6; ++i) {
        acc_mean[i] = 0.0;
        for (int j = 0; j < HISTORY; ++j)
            acc_mean[i] += acc_hist[i][(acc_i + j) % HISTORY];
        acc_mean[i] = acc_mean[i] / (float)HISTORY;
    }
    // Compute std dev
    float acc_dev[6]  = { 0.f };
    for (int i = 0; i < 6; ++i) {
        acc_dev[i] = 0.0;
        for (int j = 0; j < HISTORY; ++j) {
            float err = acc_hist[i][(acc_i + j) % HISTORY] - acc_mean[i];
            acc_dev[i] += err * err;
        }
        acc_dev[i] = sqrtf(acc_dev[i] / (float)HISTORY);
    }

    // Save dev data
    for (int i = 0; i < 6; ++i)
        acc_hist_dev[i][acc_i] = acc_dev[i];

    // Recognize activity
    float threshold_jump = 4.0;
    float threshold_walk = 2.0;

    float dev_forward = acc_dev[3];
    float dev_right = acc_dev[4];
    float dev_up = acc_dev[5];

    if (dev_up >= threshold_jump) {
        if (dev_up >= dev_forward && dev_up >= dev_right)
            activity = 2; // Jump
        else
            activity = 1; // Walk
    } else if (dev_forward >= threshold_walk || dev_right >= threshold_walk || dev_up >= threshold_walk) {
        activity = 1; // Walk
    } else {
        activity = 0; // None
    }
    

    acc_i = (acc_i + 1) % HISTORY; // Update history head index
    ++current_step;
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
