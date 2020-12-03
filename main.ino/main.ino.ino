#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#define rotary_dt 3
#define rotary_clk 2
#define rotary_switch 4
#define relay 5


String rotary_rotation;
int push_switch_last_value;

int rotaryCurrentStateCLK;
int rotaryLastStateCLK;

String modes[] = {"off", "min", "max"};
int temps[] = {0,     15,    21};
int min_temp = 10;
int max_temp = 30;
int mode = 0;
int mode_size = 3;
int display_mode = 1;  // 0: off 1: on


U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

void setup() {
  Serial.begin(9600);

  pinMode(rotary_clk, INPUT);
  pinMode(rotary_dt, INPUT);
  pinMode(rotary_switch, INPUT_PULLUP);
  pinMode(relay, OUTPUT);
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
  cycleMode();
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
  if (mode != 0) {
    u8g2.drawStr(0,12,"Status: ");
    u8g2.setCursor(60,12);
    u8g2.print(modes[mode]);
    u8g2.drawStr(0,24,"Target: ");
    u8g2.setCursor(60,24);
    u8g2.print(getCurrentModeTemp());
  }
  u8g2.sendBuffer();
}

int getCurrentModeTemp() {
  return temps[mode];
}

void setCurrentModeTemp() {
  rotaryCurrentStateCLK = digitalRead(rotary_clk);
  int dt = digitalRead(rotary_dt);
  int _temp = temps[mode];
  if (rotaryCurrentStateCLK != rotaryLastStateCLK && rotaryCurrentStateCLK == 1 && mode != 0) {
      
    if (rotaryCurrentStateCLK != dt && _temp < max_temp) {
      _temp++;
    }
    else if (rotaryCurrentStateCLK == dt && _temp > min_temp) {
      _temp--;
    }
    temps[mode] = _temp;
    printModeTemps();
  }
  rotaryLastStateCLK = rotaryCurrentStateCLK;
}

void loop() {

  printDisplay();
  
  digitalWrite(13, HIGH);
  delay(200);

  printDisplay();
  
  digitalWrite(13, LOW);
  delay(200);  
  
}
