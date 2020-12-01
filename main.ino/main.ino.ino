#define rotary_dt 2
#define rotary_clk 3
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

void setup() {
  Serial.begin(9600);

  pinMode(rotary_clk, INPUT);
  pinMode(rotary_dt, INPUT);
  pinMode(rotary_switch, INPUT_PULLUP);
  pinMode(relay, OUTPUT);
  pinMode(13, OUTPUT);

  Serial.println("==============");
  printModeTemps();
  Serial.println("");
  Serial.println("==============");

  Serial.print("mode: ");
  Serial.println(modes[mode]);

  attachInterrupt(digitalPinToInterrupt(rotary_switch), cycleMode, RISING);
  attachInterrupt(digitalPinToInterrupt(rotary_clk), setCurrentModeTemp, CHANGE);
  
}

void printModeTemps() {
  for (int i=0; i<mode_size; i=i+1) {
      Serial.print(modes[i]);
      Serial.print(": ");
      Serial.print(temps[i]);
      Serial.print(" ");
  }
}

void cycleMode() {
  mode++;
  // Avoid over settings the mode value
  if (mode>=mode_size) {
    mode = 0;
  }
  
  Serial.print("mode: ");
  Serial.print(modes[mode]);
  Serial.print(" Target: ");
  Serial.print(temps[mode]);
  Serial.println("C");
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
    Serial.println("");
  }
  rotaryLastStateCLK = rotaryCurrentStateCLK;
}

void loop() {
  // Keep in mind the pull-up means the pushbutton's logic is inverted. It goes
  // HIGH when it's open, and LOW when it's pressed. Turn on pin 13 when the
  // button's pressed, and off when it's not:

  digitalWrite(13, HIGH);
  delay(200);
  digitalWrite(13, LOW);
  delay(200);  
}
