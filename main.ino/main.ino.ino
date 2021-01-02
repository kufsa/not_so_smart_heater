#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#define ROTARY_CLK 2
#define ROTARY_DT 3
#define ROTARY_SW 4
#define DHTPIN 5
#define RELAY 6
#define DHTTYPE DHT11


// Configuration variables
String modes[] = {"off", "min", "max"};    // Different modes
int temps[]    = { 0,     18,    22};      // Default Temps for each mode
int min_temp   = 5;                        // Hardcode a min temp
int max_temp   = 30;                       // Hardcode a max temp
int mode       = 0;                        // Start in mode 0: off
int screen_auto_off = 5 * 1000;            // Screen auto
int temp_sensor_correction = -3;           // Modify the temp read by this value - Heat in the case.
unsigned long auto_mode_change = 3600000;  // after one hour on max swich back to min
unsigned long relay_toggle_timeout = 605000; // Threshold to avoid rapird relay toggling
unsigned long check_interval = 10000;      // Check temp every 10 seconds


// Internal vairbales
int relay_status = 0;                        // 0: off 1: on
int display_mode = 1;                        // 0: off 1: on
unsigned long last_relay_toggle = millis();  // Timestamp of last action ie: relay togged
unsigned long last_temp_read = millis();     // Last time temp was read
unsigned long last_switch_throw = millis();  // debounce switch
unsigned long last_interval_run = millis();  // last time the check was performed
float temp_history[10];                      // Keep history of temp reads to calculate average


String rotary_rotation;
int push_switch_last_value;
int rotaryCurrentStateCLK;
int rotaryLastStateCLK;
int mode_size  = 3;

// LCD & Temp sensor lib init
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);
DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial.begin(9600);
  dht.begin();

  // Setting pin modes
  pinMode(ROTARY_CLK, INPUT);
  pinMode(ROTARY_DT, INPUT);
  pinMode(ROTARY_SW, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  pinMode(13, OUTPUT);

  // Rotary encoder interrupts
  attachInterrupt(digitalPinToInterrupt(ROTARY_SW), switchHandler, RISING);
  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK), setCurrentModeTemp, CHANGE);

  // LCD setup
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_crox1c_tf);
  u8g2.drawStr(0, 12, "Starting");
  u8g2.sendBuffer();

  printStatus();

  // Startup "animation"
  for (int i=65; i<81; i+=4) {
    delay(250);
    u8g2.drawStr(i, 12, ".");
    u8g2.sendBuffer();
  }
}


void printTempHistory() {
  for (byte i=0; i<10; i++) {
    Serial.print(temp_history[i]);
    Serial.print(", ");
  }
  Serial.println();
}


void printStatus() {
  for (int i=0; i<mode_size; i++) {
      Serial.print(modes[i]);
      Serial.print(": ");
      Serial.print(temps[i]);
      Serial.print(" ");
  }
  Serial.print("Mode: ");
  Serial.print(modes[mode]);
  Serial.println();
  Serial.println("=========");
}


void switchHandler() {
  // Handle interrupt call from encoder switch
  unsigned long now = millis();

  if (display_mode != 0 && now - last_switch_throw > 10) {
    // only cycle when screen is on and input is debounced
    cycleMode();
    last_switch_throw = now;
  }
  logLastAction();
}


void logLastAction(){
  last_action = millis();
}

void cycleMode() {
  // Cycle trough the modes - either from encoder switch press or
  // mode timeout
  mode++;
  // Avoid over settings the mode value
  if (mode>=mode_size) {
    mode = 0;
  }
  printStatus();
}


void drawDisplay() {
  u8g2.clearBuffer();
  u8g2.drawStr(0,12,"Status: ");
  u8g2.setCursor(60,12);
  u8g2.print(modes[mode]);
  
  if (mode != 0) {
    u8g2.setCursor(0,25);
    u8g2.print(getCurrentModeTemp());
    u8g2.setCursor(19,25);
    u8g2.print("C |");
  }
  u8g2.setCursor(50,25);
  u8g2.print(getTemp());
  u8g2.setCursor(70,25);
  u8g2.print("C");
  u8g2.sendBuffer();
}


int getCurrentModeTemp() {
  return temps[mode];
}


int getTemp() {
  unsigned long now = millis();
  if (now > last_temp_read + screen_auto_off or last_temp_read == 0.0) {
    last_temp = int(dht.readTemperature());
    last_temp_read = now;
  }
  return last_temp;
}


void setCurrentModeTemp() {
  rotaryCurrentStateCLK = digitalRead(ROTARY_CLK);
  int dt = digitalRead(ROTARY_DT);
  int _temp = temps[mode];
  if (rotaryCurrentStateCLK != rotaryLastStateCLK && rotaryCurrentStateCLK == 1 && mode != 0 && display_mode == 1) {  
    if (rotaryCurrentStateCLK != dt && _temp < max_temp) {
      _temp++;
    } else if (rotaryCurrentStateCLK == dt && _temp > min_temp) {
      _temp--;
    }
    temps[mode] = _temp;
  }
  rotaryLastStateCLK = rotaryCurrentStateCLK;
  logLastAction();
}


void driveHeaterRelay() {
  unsigned long now = millis();
  if (now > last_relay_toggle + screen_auto_off) {
    if (last_temp < temps[mode] && relay_status == 0) {
      digitalWrite(RELAY, HIGH);
      relay_status = 1;
      last_action = millis();
    } else if (last_temp >= temps[mode] && relay_status == 1) {
      digitalWrite(RELAY, LOW);
      relay_status = 0;
    }
    if (now > last_action + auto_mode_change && mode == 2 && relay_status == 1) {
      // Override mode to reset to min after auto_mode_change expires
      mode = 1;
    }
    last_relay_toggle = now;
  }
}


void loop() { 
  unsigned long now = millis();
  if (now > last_action + screen_auto_off) {
    // Turn off screen after inactivity
    display_mode = 0;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  } else {
    display_mode = 1;
    drawDisplay();
  }
  getTemp();
  driveHeaterRelay();
}
