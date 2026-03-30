/*
ESP32-CAM Remote Control 
*/

// ========== WiFi Configuration ==========
#include "wifi_creds.h"  // keeps secrets out of git

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

#include "esp_wifi.h"
#include "esp_camera.h"
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_M5STACK_PSRAM
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    15
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     25
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM        5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       32
#define VSYNC_GPIO_NUM    22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#else
#error "Camera model not selected"
#endif

void startCameraServer();

// ========== Motor Control Configuration (DRV8833) ==========
// DRV8833 Dual Motor Driver - Each motor uses 2 pins:
// Motor A (LEFT track):  IN1=PWM speed, IN2=direction
// Motor B (RIGHT track): IN3=PWM speed, IN4=direction
//
// Direction logic:
// IN_x LOW,  IN_y LOW   = Coast (freewheel)
// IN_x LOW,  IN_y HIGH  = Reverse
// IN_x HIGH, IN_y LOW   = Forward
// IN_x HIGH, IN_y HIGH  = Brake
//
// Recommended GPIO connections:
const int MOTOR_A_PWM = 12;      // Left track speed (PWM)
const int MOTOR_A_DIR = 13;      // Left track direction
const int MOTOR_B_PWM = 14;      // Right track speed (PWM)
const int MOTOR_B_DIR = 15;      // Right track direction

#define PWM_FREQ 5000    // 5 kHz - good frequency for DRV8833
#define PWM_CHANNEL_A 3
#define PWM_CHANNEL_B 4
#define PWM_CHANNEL_DIR_A 5
#define PWM_CHANNEL_DIR_B 6

void initMotors() 
{
  // Setup PWM for motor speeds (channels 3 & 4)
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, 8);     // 8-bit resolution (0-255)
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, 8);
  ledcAttachPin(MOTOR_A_PWM, PWM_CHANNEL_A);
  ledcAttachPin(MOTOR_B_PWM, PWM_CHANNEL_B);
  
  // Setup PWM for direction control (channels 5 & 6)
  // Can be digital (HIGH/LOW) or PWM for variable braking
  ledcSetup(PWM_CHANNEL_DIR_A, PWM_FREQ, 8);
  ledcSetup(PWM_CHANNEL_DIR_B, PWM_FREQ, 8);
  ledcAttachPin(MOTOR_A_DIR, PWM_CHANNEL_DIR_A);
  ledcAttachPin(MOTOR_B_DIR, PWM_CHANNEL_DIR_B);
}

const int ServoPin = 2;  
void initServo() 
{
  ledcSetup(8, 50, 16); // 50 hz PWM, 16-bit resolution, range from 3250 to 6500.
  ledcAttachPin(ServoPin, 8); 
}

void setup() 
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // prevent brownouts by silencing them
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  // Remote Control Car
  initMotors();
  initServo();
  
  ledcSetup(7, 5000, 8);
  ledcAttachPin(4, 7);  //pin4 is LED
  
  Serial.println("ssid: " + (String)ssid);
  Serial.println("password: " + (String)password);
  
  WiFi.begin(ssid, password);
  delay(500);

  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      if ((StartTime+10000) < millis()) break;
  } 

  /*
  int8_t power;
  esp_wifi_set_max_tx_power(20);
  esp_wifi_get_max_tx_power(&power);
  Serial.printf("wifi power: %d \n",power); 
  */
  
  startCameraServer();

  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("");
    Serial.println("WiFi connected");    
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
  } else {
    Serial.println("");
    Serial.println("WiFi disconnected");      
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("' to connect");
    char* apssid = "ESP32-CAM";
    char* appassword = "12345678";         //AP password require at least 8 characters.
    WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);    
  }

  for (int i=0;i<5;i++) 
  {
    ledcWrite(7,10);  // flash led
    delay(200);
    ledcWrite(7,0);
    delay(200);    
  }       
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  Serial.printf("RSSi: %ld dBm\n",WiFi.RSSI()); 
}
