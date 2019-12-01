#define rotary_1 2
#define rotary_2 3
#define rotary_switch 4
#define relay 5


String rotary_rotation;
int push_switch_last_value;

String menu[] = {"Item 1", "Item 2", "Item 3", "Item 4"};
int menu_pos = 0;
int menu_size = 4;

void setup() {
  Serial.begin(9600);

  pinMode(rotary_1, INPUT_PULLUP);
  pinMode(rotary_2, INPUT_PULLUP);
  pinMode(rotary_switch, INPUT_PULLUP);
  pinMode(relay, OUTPUT);
  pinMode(13, OUTPUT);

  Serial.println("");
  for (int i=0; i<menu_size; i=i+1) {
      Serial.print(menu[i]);
      Serial.print(" ");
  }
  Serial.println("");

  getMenuItem(menu_pos);
}

void getMenuItem(int _pos) {
  if (_pos < 0) {
    // Negative numbers produce errors
    menu_pos = 0;
    _pos = 0;
  }
  else if (_pos > menu_size-1) {
    // Don't jump up the menu at the end
    menu_pos = menu_size - 1;
    _pos = menu_size - 1;
  }
  
  int pos = _pos % menu_size;
  Serial.println(menu[pos]);
  delay(70);

}

void loop() {
  //read the pushbutton value into a variable
  int push_switch = digitalRead(rotary_switch);
  int r_value_1 = digitalRead(rotary_1);
  int r_value_2 = digitalRead(rotary_2);
  
  if (r_value_1>r_value_2) {
    menu_pos++;
    Serial.println("-- Clockwise rotation");
    getMenuItem(menu_pos);
  }
  else if(r_value_1<r_value_2) {
    menu_pos--;
    Serial.println("-- Counter-clockwise rotation"); 
    getMenuItem(menu_pos);
  }

  // Keep in mind the pull-up means the pushbutton's logic is inverted. It goes
  // HIGH when it's open, and LOW when it's pressed. Turn on pin 13 when the
  // button's pressed, and off when it's not:
  if (push_switch == HIGH) {
    digitalWrite(13, LOW);
  } else {
    digitalWrite(13, HIGH);
    Serial.println("ENTER");
    digitalWrite(relay, HIGH);
    delay(1000);
    digitalWrite(relay, LOW);
    delay(300);
  }
  
}
