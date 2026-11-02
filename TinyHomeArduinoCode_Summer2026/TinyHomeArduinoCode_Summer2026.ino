#include <OneWire.h>              
#include <DallasTemperature.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

OneWire oneWire(2); 
DallasTemperature sensor2(&oneWire); 

// State variables
int CHECK = 0; // PROBLEM: CHECK variable is set to 0 (UP) at the start, meaning you have to press DOWN to go UP. Will have to chnge in the future.
int CHECK_DB = 0; 
int CHECK_W = 0; 
int CHECK_KB = 0;
int CHECK_F = 0;

int MODE = 1; 
int SYSTEM = 1; 

// Track previous button states for edge detection
int lastSysState = LOW;
int lastSelState = LOW;

// Motor/fan pins
const int DB1 = 3; const int DB2 = 4;
const int W1 = 5;  const int W2 = 6;
const int KB1 = 9; const int KB2 = 10;
const int F1 = 7;  const int F2 = 8;

// Button pins
const int BU = 11;  const int BD = 13;  
const int sel = 12; const int sys = 14; 

// Non-blocking Timer Variables
unsigned long motorStartTime = 0;
bool motorsRunning = false;
int activeMotor1 = -1;
int activeMotor2 = -1;

// LCD Update Timer (prevents reading sensor too rapidly)
unsigned long lastLcdUpdate = 0;

void setup() {
  Serial.begin(9600); // Delete after for space
  sensor2.begin(); 

  pinMode(KB1, OUTPUT); pinMode(KB2, OUTPUT);
  pinMode(DB1, OUTPUT); pinMode(DB2, OUTPUT);
  pinMode(W1, OUTPUT);  pinMode(W2, OUTPUT);
  pinMode(F1, OUTPUT);  pinMode(F2, OUTPUT);

  // Initial off states
  allMotorsOff();

  pinMode(BU, INPUT); pinMode(BD, INPUT);
  pinMode(sel, INPUT); pinMode(sys, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); 
  lcd.print("Manual Mode");
}

// Helper function to stop all movement quickly
void allMotorsOff() {
  digitalWrite(F1, LOW);  digitalWrite(F2, HIGH);
  digitalWrite(DB1, LOW); digitalWrite(DB2, LOW);
  digitalWrite(W1, LOW);  digitalWrite(W2, LOW);
  digitalWrite(KB1, LOW); digitalWrite(KB2, LOW);
}

// Variable to hold how long the current motor task should run
unsigned long motorRunDuration = 10000; 

// Updated Helper function with a custom duration parameter
void runMotorTimed(int pin1, int pin2, int state1, int state2, unsigned long duration) {
  allMotorsOff(); // Safety stop
  digitalWrite(pin1, state1);
  digitalWrite(pin2, state2);
  activeMotor1 = pin1;
  activeMotor2 = pin2;
  motorStartTime = millis();
  motorRunDuration = duration; // Save the custom duration for this specific movement
  motorsRunning = true;
}

void loop() { 
  unsigned long currentMillis = millis();

  // 1. Manage Background Motor Timers (Dynamic duration)
  if (motorsRunning && (currentMillis - motorStartTime >= motorRunDuration)) {
    digitalWrite(activeMotor1, LOW);
    digitalWrite(activeMotor2, LOW);
    motorsRunning = false;
    activeMotor1 = -1;
    activeMotor2 = -1;
  }

  // 2. Read temperature and update LCD smoothly every 10 seconds
  if (currentMillis - lastLcdUpdate >= 10000) { 
    sensor2.requestTemperatures(); // Getting temperature from sensor

    float rawTemp = sensor2.getTempCByIndex(0);

    // Check for standard error values (85°C is 185°F, -127°C means disconnected)
    if (rawTemp != 85.00 && rawTemp != -127.00) {
      float temp = rawTemp * 9 / 5 + 32; // Convert to Fahrenheit
      
      lcd.begin(16, 2); // Reset LCD to prevent further error during button presses

      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temp, 1); // Prints temp with 1 decimal place (e.g., 72.4)
      lcd.print(" F   ");
    } 
    else {
      Serial.println("Sensor read error caught! Retrying next cycle...");
    }
    
    lastLcdUpdate = currentMillis;

  }

  // 3. System Button Press (Debounced, Updated to allow 4 Systems)
    int currentSysState = digitalRead(sys);
    if (currentSysState == HIGH && lastSysState == LOW) {
      SYSTEM++;
      if (SYSTEM == 5) SYSTEM = 1; // FIXED: Changed from 4 to 5 to include our 4th mode
      
      allMotorsOff();
      delay(50); 
    }
    lastSysState = currentSysState;

  // 4. Mode Selection Button Press
  int currentSelState = digitalRead(sel);
  if (currentSelState == HIGH && lastSelState == LOW) {
    MODE++;
    if (MODE == 5) MODE = 1; 
    delay(50); 
  }
  lastSelState = currentSelState;

  // 5. Update Static Text Layouts
  if (SYSTEM == 1) {
    lcd.setCursor(0, 0);
    if (MODE == 1) lcd.print("Fan             ");
    if (MODE == 2) lcd.print("Door Blinds     ");
    if (MODE == 3) lcd.print("Windows         ");
    if (MODE == 4) lcd.print("Kit/Bat Blinds  ");
  } else if (SYSTEM == 2) {
    lcd.setCursor(0, 0); 
    lcd.print("Automation Mode ");
  } else if (SYSTEM == 3) {
    lcd.setCursor(0, 0); 
    lcd.print("Sync Mode       ");
  } else if (SYSTEM == 4) {
    lcd.setCursor(0, 0); 
    lcd.print("Hold Mode: ");
    // Appends the name of the currently selected component right next to it
    if (MODE == 1) lcd.print("Fan   ");
    if (MODE == 2) lcd.print("DB    ");
    if (MODE == 3) lcd.print("Wndw  ");
    if (MODE == 4) lcd.print("KB    ");
  }

  // =================================================================
  // 6. MANUAL MODE ACTIONS (SYSTEM == 1)
  // =================================================================
  if (SYSTEM == 1 && !motorsRunning) {
    
    // UP BUTTON PRESSED
    if ((digitalRead(BU) == HIGH) && (CHECK == 0)) {
      if (MODE == 1 && CHECK_F == 0)   { digitalWrite(F1, LOW); digitalWrite(F2, LOW); CHECK_F = 1; }
      
      // Changed to 15000 (15 seconds) to give it more time to climb up!
      if (MODE == 2 && CHECK_DB == 0)  { runMotorTimed(DB1, DB2, LOW, HIGH, 5000); CHECK_DB = 1; } // Measured 65 seconds
      if (MODE == 3 && CHECK_W == 0)   { runMotorTimed(W1, W2, LOW, HIGH, 15000);  CHECK_W = 1; }
      if (MODE == 4 && CHECK_KB == 0)  { runMotorTimed(KB1, KB2, HIGH, LOW, 5000); CHECK_KB = 1; } // Measured 25 seconds
      
      CHECK = 1;
    } 
    
    // DOWN BUTTON PRESSED
    if ((digitalRead(BD) == HIGH) && (CHECK == 1)) {
      if (MODE == 1 && CHECK_F == 1)   { digitalWrite(F1, LOW); digitalWrite(F2, HIGH); CHECK_F = 0; }
      
      // Left at 10000 (10 seconds) since dropping down is faster
      if (MODE == 2 && CHECK_DB == 1)  { runMotorTimed(DB1, DB2, HIGH, LOW, 5000); CHECK_DB = 0; } // Measured 65 seconds
      if (MODE == 3 && CHECK_W == 1)   { runMotorTimed(W1, W2, HIGH, LOW, 10000);  CHECK_W = 0; }
      if (MODE == 4 && CHECK_KB == 1)  { runMotorTimed(KB1, KB2, LOW, HIGH, 5000); CHECK_KB = 0; } // Measured 25 seconds
      
      CHECK = 0;
    } 
  }

  // =================================================================
  // 7. AUTOMATION MODE OPERATIONS (SYSTEM == 2)
  // =================================================================
  if (SYSTEM == 2 && !motorsRunning) {
    sensor2.requestTemperatures(); 
    float temp = sensor2.getTempCByIndex(0) * 9 / 5 + 32;

    if ((temp < 68) && (CHECK == 0)) { // If it's cold indoors
      CHECK = 1; CHECK_F = 1; CHECK_DB = 1; CHECK_W = 1; CHECK_KB = 1;
      digitalWrite(F1, LOW); digitalWrite(F2, LOW); 
      runMotorTimed(DB1, DB2, HIGH, LOW, 15000); // 15 seconds for automation UP. TODO: Change this value
    } 
    if ((temp > 72) && (CHECK == 1)) { // If it's hot indoors
      CHECK = 0; CHECK_F = 0; CHECK_DB = 0; CHECK_W = 0; CHECK_KB = 0;
      digitalWrite(F1, LOW); digitalWrite(F2, HIGH); 
      runMotorTimed(DB1, DB2, LOW, HIGH, 10000); // 10 seconds for automation DOWN. TODO: Change this value
    } 
  } 

   // =================================================================
  // 7.5 SYNC MODE OPERATIONS (SYSTEM == 3)
  // =================================================================

  // Currently empty because I don't know what it's for...?
  // It is apparently for smaller (more precise) movements than the Manual Mode. Debating whether to repurpose it or not...
  // =================================================================
  // 8. HOLD MODE OPERATIONS (SYSTEM == 4)
  // =================================================================
  if (SYSTEM == 4) {
    
    // --- IF UP BUTTON IS HELD DOWN ---
    if (digitalRead(BU) == HIGH) {
      if (MODE == 1) { digitalWrite(F1, LOW);  digitalWrite(F2, LOW); CHECK_F = 0; }  // Fan Off
      if (MODE == 2) { digitalWrite(DB1, LOW); digitalWrite(DB2, HIGH); CHECK_DB = 0; } // Door Blind DOWN (swapped due to blind arrangement)
      if (MODE == 3) { digitalWrite(W1, LOW);   digitalWrite(W2, HIGH); CHECK_W = 1; } // Windows UP (Close)
      if (MODE == 4) { digitalWrite(KB1, HIGH); digitalWrite(KB2, LOW); CHECK_KB = 1; } // Kit/Bat Blind UP
    } 
    
    // --- IF DOWN BUTTON IS HELD DOWN ---
    else if (digitalRead(BD) == HIGH) {
      if (MODE == 1) { digitalWrite(F1, LOW);  digitalWrite(F2, HIGH); CHECK_F = 1; } // Fan On
      if (MODE == 2) { digitalWrite(DB1, HIGH);  digitalWrite(DB2, LOW); CHECK_DB = 1; } // Door Blind UP (swapped due to blind arrangement)
      if (MODE == 3) { digitalWrite(W1, HIGH); digitalWrite(W2, LOW); CHECK_W = 0; }  // Wind ows DOWN (Open)
      if (MODE == 4) { digitalWrite(KB1, LOW);  digitalWrite(KB2, HIGH); CHECK_KB = 0; } //Kit/Bat Blind DOWN
    } 
    
    // --- IF NEITHER BUTTON IS HELD ---
    else {
      // Only turn off the specific component we are currently selecting.
      // This prevents slamming other relays around unnecessarily.
      if (MODE == 1) { digitalWrite(F1, LOW);  digitalWrite(F2, HIGH); }
      if (MODE == 2) { digitalWrite(DB1, LOW);  digitalWrite(DB2, LOW); }
      if (MODE == 3) { digitalWrite(W1, LOW);   digitalWrite(W2, LOW); }
      if (MODE == 4) { digitalWrite(KB1, LOW);  digitalWrite(KB2, LOW); }
    }
  }
}
