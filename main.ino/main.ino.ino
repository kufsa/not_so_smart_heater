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
unsigned long auto_mode_change = 3600000;  // after one hour on max switch back to min
unsigned long relay_toggle_timeout = 605000; // Threshold to avoid rapid relay toggling
unsigned long check_interval = 10000;      // Check temp every 60 seconds


// Internal variables
int relay_status = 0;                        // 0: off 1: on
int display_mode = 1;                        // 0: off 1: on
unsigned long last_relay_toggle = millis();  // Timestamp of last action ie: relay togged
unsigned long last_temp_read = millis();     // Last time temp was read
unsigned long last_switch_throw = millis();  // Last time switch was pressed. ->debounce switch
unsigned long last_interval_run = millis();  // last time the check was performed
float temp_history[10];                      // Keep history of temp reads to calculate average
int rotaryCurrentStateCLK;
int rotaryLastStateCLK;
int mode_size  = 3;

// LCD & Temp sensor lib init
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);
DHT dht(DHTPIN, DHTTYPE);


/*
 * Print current state to serial - useful for debugging
 */
void printStatus() {
  
    Serial.println(">>>>>>>>>>>>>> Debug");

    Serial.print("Current Temp: ");
    Serial.println(getTemp());

    Serial.print("Mode settings: ");
    for (int i=0; i<mode_size; i++) {
        Serial.print(modes[i]);
        Serial.print(": ");
        Serial.print(temps[i]);
        Serial.print(" ");
    }
    Serial.println();
    
    Serial.print("Mode: ");
    Serial.println(modes[mode]);
    Serial.print("Relay status: ");
    Serial.println(relay_status);
    Serial.print("Now: ");
    Serial.println(millis());
    Serial.print("Display Mode: ");
    Serial.println(display_mode);
    Serial.print("Last relay toggle: ");
    Serial.println(last_relay_toggle);
    Serial.print("Last temp read: ");
    Serial.println(last_temp_read);
    Serial.print("Last switch throw: ");
    Serial.println(last_switch_throw);
    Serial.print("last interval run: ");
    Serial.println(last_interval_run);
    Serial.print("Rotary current state clk: ");
    Serial.println(rotaryCurrentStateCLK);
    Serial.print("Rotary last state clk: ");
    Serial.println(rotaryLastStateCLK);
    Serial.print("Check Interval: ");
    Serial.println(check_interval);

    Serial.println("<<<<<<<<<<<<<< Debug");

}


int getCurrentModeTemp() {
    return temps[mode];
}

void switchHandler() {
  Serial.println("~~~~~~~~~~~~~~~ Switch pressed");
  // Handle interrupt call from encoder switch
  unsigned long now = millis();
  if (display_mode != 0 && now - last_switch_throw > 100) {
    // only cycle when screen is on and input is debounced
      last_switch_throw = now;
      cycleMode();
  }

  logLastAction();
  driveHeaterRelay();
}


void logLastAction(){
  last_relay_toggle = millis();
}

/*
 * Cycle trough the modes - either from encoder switch press or mode timeout
 */
void cycleMode() {
  mode++;
  // Avoid over settings the mode value
  if (mode>=mode_size) {
    mode = 0;
  }
  printStatus();
}


/*
 * Update screen to reflect the current state
 */
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


/*
 * Read temperature from sensor. Write it to the temp history list and return the current value.
 */
float readTemp() {
    unsigned long now = millis();
    float current_temp = dht.readTemperature();

    // Move all history values one to the right to make room for the latest
    for (byte i = 9; i > 0; i--) {
        temp_history[i] = temp_history[i-1];
    }

    temp_history[0] = current_temp;
    last_temp_read = now;

    return current_temp;
}


/*
 * Return the averaged temperature - Does not read data from sensor!
 */
int getTemp() {

  unsigned long now = millis();
  float temp_sum = 0.0;
  int valid_values = 0;

  for (byte i = 0; i < 10; i++) {
    if (temp_history[i] != 0) {
      // sum of all valid values in array for average
      temp_sum = temp_sum + temp_history[i];
      valid_values++;
    }
  }
  int average_temp = temp_sum / valid_values + temp_sensor_correction;
  return average_temp;
}


/*
 * Handler for rotary encoder input
 */
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
  driveHeaterRelay();
  logLastAction();
}

/*
 * Check current temp average and enable/disable relay accordingly
 */
void driveHeaterRelay() {
  unsigned long now = millis();
  if (getTemp() < temps[mode] && relay_status == 0) {
    digitalWrite(RELAY, HIGH);
    relay_status = 1;
    last_relay_toggle = millis();
    last_relay_toggle = now;

  } else if (getTemp() > temps[mode] && relay_status == 1) {
    digitalWrite(RELAY, LOW);
    relay_status = 0;
    last_relay_toggle = now;

  }
}

/*
 * Switch from max mode back to min mode after a set time of inactivity
 */
void autoModeFallback() {
  unsigned long now = millis();
  if (mode == 2
      && relay_status == 1
      && now > last_relay_toggle + auto_mode_change
      && now > last_relay_toggle + relay_toggle_timeout)
  {
    // Override mode to reset to min after auto_mode_change expires
    mode = 1;
    Serial.println("Auto fallback to min status");
  }
}




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

  readTemp();

}



/*
 * Main arduino loop
 */
void loop() { 
  unsigned long now = millis();

  if (now > last_relay_toggle + screen_auto_off) {
    // Turn off screen after inactivity
    display_mode = 0;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  } else {
    display_mode = 1;
    drawDisplay();
  }

  if (now > check_interval + last_interval_run) {
    printStatus();

    last_interval_run = now;
    readTemp();
    autoModeFallback();
    driveHeaterRelay();
  }
}
