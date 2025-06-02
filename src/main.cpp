#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD address and size
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int billAcceptorPin = 2;       // Bill acceptor signal pin
const int relayCoinHopperPin = 3;    // Single relay channel for coin hopper
const int buzzerPin = 4;             // Buzzer pin
const int buttonPin = 5;             // Button pin

// Variables
volatile int credits = 0; // Each credit represents ₱5
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Double-press tracking
unsigned long lastButtonPressTime = 0;
bool waitingForSecondPress = false;
const unsigned long doublePressInterval = 1500; // 1.5 seconds

// Activate buzzer
void buzz(int duration) {
  digitalWrite(buzzerPin, HIGH);
  delay(duration);
  digitalWrite(buzzerPin, LOW);
}

// Interrupt Service Routine
void billInserted() {
  credits++; // Each bill gives 1 credit = ₱5
  buzz(100);
}

// Dispense ₱5 coins per credit using single hopper
void dispenseCoins(int numCoins) {
  for (int i = 0; i < numCoins; i++) {
    digitalWrite(relayCoinHopperPin, LOW);   // Relay ON (active LOW)
    delay(300);                              // Coin dispensing time
    digitalWrite(relayCoinHopperPin, HIGH);  // Relay OFF
    buzz(50);
    delay(200);
  }
}

void setup() {
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Insert Bill      ");
  lcd.setCursor(0, 1);
  lcd.print("Credits: 0       ");

  // Setup pins
  pinMode(billAcceptorPin, INPUT_PULLUP);
  pinMode(relayCoinHopperPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // Turn off devices initially
  digitalWrite(relayCoinHopperPin, HIGH);  // Relay OFF (active LOW)
  digitalWrite(buzzerPin, LOW);

  // Attach interrupt for bill acceptor
  attachInterrupt(digitalPinToInterrupt(billAcceptorPin), billInserted, FALLING);
}

void loop() {
  // Read and debounce button
  bool buttonState = digitalRead(buttonPin);

  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == LOW && lastButtonState == HIGH) {
      unsigned long now = millis();

      if (waitingForSecondPress && (now - lastButtonPressTime <= doublePressInterval)) {
        // Double press detected — restart
        credits = 0;
        lcd.setCursor(0, 0);
        lcd.print("System Restarted ");
        lcd.setCursor(0, 1);
        lcd.print("Credits: 0       ");
        buzz(200);
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("Insert Bill      ");
        lcd.setCursor(0, 1);
        lcd.print("Credits: 0       ");
        waitingForSecondPress = false;
      } else {
        // First press detected
        waitingForSecondPress = true;
        lastButtonPressTime = now;

        if (credits > 0) {
          dispenseCoins(credits);  // Each credit = ₱5 coin
          credits = 0;
          lcd.setCursor(0, 1);
          lcd.print("Credits: 0       ");
        } else {
          lcd.setCursor(0, 0);
          lcd.print("No credits       ");
          buzz(200);
          delay(1000);
          lcd.setCursor(0, 0);
          lcd.print("Insert Bill      ");
          lcd.setCursor(0, 1);
          lcd.print("Credits: 0       ");
        }
      }
    }
  }

  lastButtonState = buttonState;

  // Cancel double-press if time exceeded
  if (waitingForSecondPress && (millis() - lastButtonPressTime > doublePressInterval)) {
    waitingForSecondPress = false;
  }

  // Update LCD every loop
  lcd.setCursor(0, 1);
  lcd.print("Credits: ");
  lcd.print(credits);
  lcd.print("       ");  // Clear any leftover digits

  delay(50);
}
