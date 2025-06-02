#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD address and size
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int billAcceptorPin = 2;
const int relayCoinHopperPin = 8;
const int buzzerPin = 4;
const int buttonPin = 6;

bool isButtonPressed = false;

// Variables
volatile int credits = 0; // Each credit = ₱5
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 1000;

// Buzzer utility
void buzz(int duration) {
  digitalWrite(buzzerPin, HIGH);
  delay(duration);
  digitalWrite(buzzerPin, LOW);
}

// Interrupt Service Routine for bill acceptor
void billInserted() {
  credits++;
  buzz(100);
  Serial.print("Bill inserted. Credits: ");
  Serial.println(credits);
}

// Convert bill value to number of ₱5 coins, keep remainder as credit
int billToCoin() {
  int _coinToDispense = credits / 5;
  credits = credits % 5;
  return _coinToDispense;
}

bool isCreditValid (){
  return credits > 5;
}

// Dispense ₱5 coins
void dispenseCoins(int numCoins) {
  Serial.print("Starting to dispense ");
  Serial.print(numCoins);
  Serial.println(" peso coin(s).");

  for (int i = 0; i < numCoins; i++) {
    Serial.print("Dispensing coin ");
    Serial.println(i + 1);

    digitalWrite(relayCoinHopperPin, LOW);   // Relay ON (active LOW)
    delay(3000);                              // ON for 1 second

    digitalWrite(relayCoinHopperPin, HIGH);  // Relay OFF
    delay(1000);                              // OFF for 1 second
  }

  Serial.println("Dispensing complete.");
}



void handleValidCredit(){
  Serial.print("Credit to dispense: ");
  Serial.println(credits);
  dispenseCoins(billToCoin());

  Serial.print("Remaining credit: ");
  Serial.println(credits);

}

void handleInvalidCredit(){
  Serial.println("Please insert bill");
}

void setup() {
  Serial.begin(9600);
  delay(2000);  // Allow time for Serial monitor

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Insert Bill      ");
  lcd.setCursor(0, 1);
  lcd.print("Credits: 0       ");

  pinMode(billAcceptorPin, INPUT_PULLUP);
  pinMode(relayCoinHopperPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  digitalWrite(relayCoinHopperPin, HIGH);  // Relay OFF
  digitalWrite(buzzerPin, LOW);

  attachInterrupt(digitalPinToInterrupt(billAcceptorPin), billInserted, FALLING);

  Serial.println("System initialized. Waiting for bill or button press.");

  //digitalWrite(relayCoinHopperPin, LOW);
}

void loop() {
  bool buttonState = digitalRead(buttonPin);
 isButtonPressed = digitalRead(buttonPin);

 if (isButtonPressed){
    Serial.println("Button PRESSED");
    isButtonPressed = false;
    delay(1000);
    if (isCreditValid()){
      handleValidCredit();
    } else {
      handleInvalidCredit();
    }
 }

  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == LOW && lastButtonState == HIGH) {
      Serial.println("Button pressed.");

      if (credits > 0) {
        Serial.print("Credits available: ");
        Serial.println(credits);
        dispenseCoins(credits);
        credits = 0;
        lcd.setCursor(0, 1);
        lcd.print("Credits: 0       ");
        Serial.println("Credits reset to 0.");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("No credits       ");
        buzz(200);
        Serial.println("Button pressed, but no credits to dispense.");
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("Insert Bill      ");
      }
    }
  }

  lastButtonState = buttonState;

  // Update LCD display
  lcd.setCursor(0, 1);
  lcd.print("Credits: ");
  lcd.print(credits);
  lcd.print("       ");

  delay(50);

  

  
}
