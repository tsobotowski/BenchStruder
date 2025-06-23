#include <LiquidCrystal.h>
#include <NEXNTC.h>

#define TEMP_MIN 200
#define TEMP_MAX 280
#define HOLD_TIME 1000  // Length of button press in ms to be read as a hold

NTCThermistor thermistor(
  A0,       // Analog pin
  1000.0,   // Series resistor (Ohms)
  10000.0,  // Thermistor nominal resistance at 25°C
  25.0,     // Nominal temperature (°C)
  3950.0    // Beta coefficient
);

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2, heater = A1, extruder = A2, up_heat = A3, down_extrude = A4;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
float temp;
int extrusion_temp = 0;

enum modes {
  IDLE,
  SET,  //Allow user to set temperature
  HEAT,
  EXTRUDING,
  HOT,
  RUNAWAY,  //Thermal runaway
  INITERR   //Initialization error
};

//global variables for button activity
bool up_press = false, dn_press = false, up_hold = false, dn_hold = false;

modes current_mode;

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

  pinMode(heater, OUTPUT);
  pinMode(extruder, OUTPUT);

  delay(1000);

  lcd.clear();

  if ((temp = thermistor.readTemperatureC()) <= 0) {  //Catch disconnected thermistor
    return false;
  }

  return true;
}

void idle() {  //Idle state, heating is off
  static int temp_cursor = 200;
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
    delay(200);
  }
  if (dn_press && temp_cursor > TEMP_MIN) {
    temp_cursor -= 5;
    delay(200);
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

  if (up_press) {
    extrusion_temp = 0;
    current_mode = IDLE;
  }
}

void extrude() {
  if (up_press) {
    extrusion_temp = 0;
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
  lcd.setCursor(0, 0);
  lcd.print("Startup OK");
  delay(1000);
  lcd.clear();
  current_mode = IDLE;
}

void loop() {
  temp = thermistor.readTemperatureC();
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
    case HOT:
      hot();
      break;
    case INITERR:
      init_err();
      break;
    case RUNAWAY:
      runaway_err();  //Thermal runaway error, stops operation until restart
      break;
  }
}
