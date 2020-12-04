#include <DHT.h>
#include <DHT_U.h>

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#define rotary_dt 3
#define rotary_clk 2
#define rotary_switch 4
#define DHTPIN 5
#define relay 6
#define DHTTYPE DHT11

String rotary_rotation;
int push_switch_last_value;

int rotaryCurrentStateCLK;
int rotaryLastStateCLK;

String modes[] = {"off", "min", "max"};
int temps[] = {0,     15,    21};
int min_temp = 5;
int max_temp = 30;
int mode = 0;
int mode_size = 3;
int display_mode = 1;  // 0: off 1: on
int screen_auto_off = 5 * 1000;
int auto_mode_change = 1000 * 3600;  // One hour on mode max. then swich back to min
unsigned long last_action = millis();
float last_temp = 0.0;
unsigned long last_temp_read = millis();

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(rotary_clk, INPUT);
  pinMode(rotary_dt, INPUT);
  pinMode(rotary_switch, INPUT_PULLUP);
  //pinMode(relay, OUTPUT);
  pinMode(13, OUTPUT);
  
  printModeTemps();

  attachInterrupt(digitalPinToInterrupt(rotary_switch), switchHandler, RISING);
  attachInterrupt(digitalPinToInterrupt(rotary_clk), setCurrentModeTemp, CHANGE);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_crox1c_tf);
  u8g2.drawStr(0, 12, "Starting");
  u8g2.sendBuffer();
  
  for (int i=65; i<81; i+=4) {
    delay(250);
    u8g2.drawStr(i, 12, ".");
    u8g2.sendBuffer();
  }
}

void printModeTemps() {
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
  if (display_mode != 0) {
    // only cycle when screen is on
    cycleMode();
  }
  logLastAction();
}

void logLastAction(){
  last_action = millis();
}

void cycleMode() {
  mode++;
  // Avoid over settings the mode value
  if (mode>=mode_size) {
    mode = 0;
  }
  printModeTemps();
}

void printDisplay() {
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
  u8g2.print(int(getTemp()));
  u8g2.setCursor(70,25);
  u8g2.print("C");
  u8g2.sendBuffer();
}

int getCurrentModeTemp() {
  return temps[mode];
}

float getTemp() {
  unsigned long now = millis();
  if (now > last_temp_read + screen_auto_off or last_temp_read == 0.0) {
    last_temp = dht.readTemperature();
    Serial.print("TEMP: ");
    Serial.println(last_temp);
    last_temp_read = now;
  }
  return last_temp;
}

void setCurrentModeTemp() {
  rotaryCurrentStateCLK = digitalRead(rotary_clk);
  int dt = digitalRead(rotary_dt);
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


void loop() { 
  unsigned long now = millis();
  if (now > last_action + screen_auto_off) {
    // Turn off screen after inactivity
    display_mode = 0;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  } else {
    display_mode = 1;
    printDisplay();
  }
}
