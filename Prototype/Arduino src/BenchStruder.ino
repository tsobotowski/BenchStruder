#include <LiquidCrystal.h>
#include <NEXNTC.h>

//Beginning of global variables
#define TEMP_MIN 100 //Minimum heater tempreture
#define TEMP_MAX 280 //Maximum heater tempreture
#define HOLD_TIME 1000  // Length of button press in ms to be read as a hold

NTCThermistor thermistor(
  A0,       // Analog pin
  4700.0,   // Series resistor (Ohms) Default: 1000
  100000.0,  // Thermistor nominal resistance at 25°C Default: 10000
  25.0,     // Nominal temperature (°C) Default: 25
  4267.0    // Beta coefficient Default: 3950
);

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2, heater = A1, extruder = A2, up_heat = A4, down_extrude = A3;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
float temp;
int extrusion_temp = 0; //Initialize extrusion temp as 0

enum modes {
  IDLE,
  HEAT,
  EXTRUDING,
  RUNAWAY,  //Thermal runaway
  INITERR   //Initialization error
};

//global variables for button activity
bool up_press = false, dn_press = false, up_hold = false, dn_hold = false;

modes current_mode; //Current mode of operation

//End of global variables

void read_buttons() {  // TODO: Make non-blocking
  unsigned long start_time;
  up_press = false, dn_press = false, up_hold = false, dn_hold = false;  // Reset button states

  if (digitalRead(up_heat) == LOW) {  // If up button is pressed
    start_time = millis();            // Record start time
    while (true) {                    // Wait until it is released TODO: add millis() - start_time > 1000, need flag to note when a hold is already recorded so button being high in previous cycle can be ignored
      if (digitalRead(up_heat) == HIGH) {
        break;
      }
    }
    if (millis() - start_time > HOLD_TIME) {  //If button was held for more than a second, record a hold
      up_hold = true;
    } else {  //If button was held for less than a second, record a press
      up_press = true;
    }
  }

  if (digitalRead(down_extrude) == LOW) {
    start_time = millis();
    while (true) {
      if (digitalRead(down_extrude) == HIGH) {
        break;
      }
    }
    if (millis() - start_time > HOLD_TIME) {
      dn_hold = true;
    } else {
      dn_press = true;
    }
  }
}

bool start() {       //Set up
  pinMode(heater, OUTPUT);
  pinMode(extruder, OUTPUT);

  //Make sure relays are off
  digitalWrite(heater, HIGH);
  digitalWrite(extruder, HIGH);

  lcd.begin(16, 2);  //Add handling
  lcd.setCursor(0, 0);
  lcd.print("Starting");

  // Set up thermistor
  thermistor.begin();
  thermistor.setADCFiltering(true, 500);  // Enable moving average filter
  thermistor.setADCOversampling(2);       // Optional: enable oversampling
  thermistor.setAsyncInterval(500);       // Update every 500ms

  pinMode(up_heat, INPUT);
  pinMode(down_extrude, INPUT);

  delay(1000); //Wait for a thermistor reading

  lcd.clear();

  Serial.begin(9600);

  if ((temp = thermistor.readTemperatureC()) <= 0) {  //Catch disconnected thermistor
    return false;
  }

  return true;
}

void idle() {  //Idle state, heating is off
  digitalWrite(heater, HIGH);
  digitalWrite(extruder, HIGH);

  static int temp_cursor = TEMP_MIN; //User tempreture selection, starts at minumum
  lcd.setCursor(0, 0);
  lcd.print("Idle  Temp: ");
  lcd.print((int)temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("       Set: ");
  lcd.print(temp_cursor);
  lcd.print("C");


  if (up_press && temp_cursor < TEMP_MAX) {
    temp_cursor += 5;
  }
  if (dn_press && temp_cursor > TEMP_MIN) {
    temp_cursor -= 5;
  }

  if (up_hold) {
    extrusion_temp = temp_cursor; //Set tempreture selection
    current_mode = HEAT;
  }
}

void heat() {  //Heating state
  lcd.setCursor(0, 0);
  lcd.print("Heat  Temp: ");
  lcd.print((int)temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("       Set: ");
  lcd.print(extrusion_temp);
  lcd.print("C");

  //Simple bang-bang control
  if(temp < (extrusion_temp - 10)){
    digitalWrite(heater, LOW);
  }
  else if(temp > extrusion_temp){
    digitalWrite(heater, HIGH);
  }

  if (up_press) {
    extrusion_temp = 0;
    current_mode = IDLE;
  }
}

void extrude() {
  if (up_press) {
    current_mode = IDLE;
  }
}

void hot() {
}

void runaway_err() {
  return;
}

void init_err() {  //Prevents operation if thermistor is not connected or reading irregular value
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Error: Temp     ");
    lcd.setCursor(0, 1);
    lcd.print("[Heat] to bypass");

    read_buttons();
    if (up_press || dn_press) {
      current_mode = IDLE;
      return;
    }
  }
}

void setup() {
  if (!start()) {
    current_mode = INITERR;
    return;
  }
  //Serial connection to monitor tempreture control

  lcd.setCursor(0, 0);
  lcd.print("Startup OK");
  delay(1000);
  lcd.clear();
  current_mode = IDLE;
}

void loop() {
  temp = thermistor.readTemperatureC();

  //Report temp
  Serial.print(temp);

  read_buttons();

  switch (current_mode) {
    case IDLE:
      idle();
      break;
    case HEAT:
      heat();
      break;
    case EXTRUDING:
      extrude();
      break;
    case INITERR:
      init_err();
      break;
    case RUNAWAY:
      runaway_err();  //Thermal runaway error, stops operation until restart
      break;
  }
}
