#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <math.h>

// Pin configuration
#define ONE_WIRE_BUS 2
#define FAN1_PIN 10
#define FAN2_PIN 11

// EEPROM addresses for minTemp and maxTemp
#define EEPROM_MIN_TEMP_ADDRESS 0
#define EEPROM_MAX_TEMP_ADDRESS 4

// Temperature range (initialized from EEPROM)
float minTemp;
float maxTemp;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// Initialize the LCD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Button values from the LCD Keypad Shield
#define BTN_RIGHT 0
#define BTN_UP 1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_SELECT 4
#define BTN_NONE 5

// Function declarations
int read_LCD_buttons();
void showTemperature();
void adjustMinTemp();
void adjustMaxTemp();
void saveToEEPROM(int address, float value);
float readFromEEPROM(int address);

void setup() {
  // Start serial communication
  Serial.begin(9600);

  // Start up the library
  sensors.begin();

  // Set up LCD columns and rows
  lcd.begin(16, 2);

  // Set fan pins as output
  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);

  // Read stored temperature values from EEPROM
  minTemp = readFromEEPROM(EEPROM_MIN_TEMP_ADDRESS);
  maxTemp = readFromEEPROM(EEPROM_MAX_TEMP_ADDRESS);

  // If EEPROM contains uninitialized values (such as after a reset), set defaults
  if (isnan(minTemp) || minTemp < 0 || minTemp > 50) minTemp = 24.5; // Replace with your desired default
  if (isnan(maxTemp) || maxTemp < 0 || maxTemp > 50) maxTemp = 25.0; // Replace with your desired default

  // Log the initialization
  Serial.print("Initialized minTemp: ");
  Serial.println(minTemp);
  Serial.print("Initialized maxTemp: ");
  Serial.println(maxTemp);

/*
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AM Lesjak");
  delay(5000);
  */
}

void loop() {
  int button = read_LCD_buttons();

  switch (button) {
    case BTN_RIGHT:
      adjustMaxTemp();
      break;
    case BTN_LEFT:
      adjustMinTemp();
      break;
    case BTN_SELECT:
      // Additional functionality can be added here for future expansion
      break;
    case BTN_NONE:
      showTemperature();
      break;
  }

  // Wait a short period to debounce the button press
  delay(100);
}

void showTemperature() {
  // Request temperature from DS18B20
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // Log the current temperature reading
  Serial.print("Current temperature: ");
  Serial.print(temperature);
  Serial.println("C");

  // Display temperature on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print((char)223); // Degree symbol
  lcd.print("C");

  // Control fan speed based on temperature
  if (temperature > maxTemp) {
    analogWrite(FAN1_PIN, 255); // Full speed
    analogWrite(FAN2_PIN, 255); // Full speed
    Serial.println("Temperature above maxTemp. Fans running at full speed.");
  } else if (temperature < minTemp) {
    analogWrite(FAN1_PIN, 0);
    analogWrite(FAN2_PIN, 0);
    Serial.println("Temperature below minTemp. Fans stopped.");
  } else {
    int fanSpeed = map(temperature, minTemp, maxTemp, 0, 255);
    analogWrite(FAN1_PIN, fanSpeed);
    analogWrite(FAN2_PIN, fanSpeed);
    Serial.print("Temperature within range. Fan speed set to: ");
    Serial.println(fanSpeed);
  }

  // Display the set temperatures
  lcd.setCursor(0, 1);
  lcd.print("");
  lcd.print(minTemp);
  lcd.print((char)223);
  lcd.print("C ");
  lcd.print(maxTemp);
  lcd.print((char)223);
  lcd.print("C");
}

void adjustMinTemp() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Min Temp:");
  lcd.setCursor(0, 1);
  lcd.print(minTemp);
  lcd.print((char)223);
  lcd.print("C");

  int button = BTN_NONE;
  while (button != BTN_SELECT) {
    button = read_LCD_buttons();
    if (button == BTN_UP) {
      minTemp += 0.1;
      Serial.print("Increasing minTemp to: ");
      Serial.println(minTemp);
    } else if (button == BTN_DOWN) {
      minTemp -= 0.1;
      Serial.print("Decreasing minTemp to: ");
      Serial.println(minTemp);
    }

    lcd.setCursor(0, 1);
    lcd.print(minTemp, 1);
    lcd.print((char)223);
    lcd.print("C  ");
    delay(200);
  }

  // Save the new minTemp value to EEPROM
  saveToEEPROM(EEPROM_MIN_TEMP_ADDRESS, minTemp);
  Serial.print("minTemp saved to EEPROM: ");
  Serial.println(minTemp);
}

void adjustMaxTemp() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Max Temp:");
  lcd.setCursor(0, 1);
  lcd.print(maxTemp);
  lcd.print((char)223);
  lcd.print("C");

  int button = BTN_NONE;
  while (button != BTN_SELECT) {
    button = read_LCD_buttons();
    if (button == BTN_UP) {
      maxTemp += 0.1;
      Serial.print("Increasing maxTemp to: ");
      Serial.println(maxTemp);
    } else if (button == BTN_DOWN) {
      maxTemp -= 0.1;
      Serial.print("Decreasing maxTemp to: ");
      Serial.println(maxTemp);
    }

    lcd.setCursor(0, 1);
    lcd.print(maxTemp, 1);
    lcd.print((char)223);
    lcd.print("C  ");
    delay(200);
  }

  // Save the new maxTemp value to EEPROM
  saveToEEPROM(EEPROM_MAX_TEMP_ADDRESS, maxTemp);
  Serial.print("maxTemp saved to EEPROM: ");
  Serial.println(maxTemp);
}

void saveToEEPROM(int address, float value) {
  // Write the float value to EEPROM as 4 bytes
  byte* dataPointer = (byte*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address + i, dataPointer[i]);
  }
}

float readFromEEPROM(int address) {
  // Read 4 bytes from EEPROM and convert them to a float
  float value;
  byte* dataPointer = (byte*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    dataPointer[i] = EEPROM.read(address + i);
  }
  return value;
}

int read_LCD_buttons() {
  int adc_key_in = analogRead(0); // read the value from the sensor

  if (adc_key_in > 1000) return BTN_NONE;
  if (adc_key_in < 50)   return BTN_RIGHT;
  if (adc_key_in < 195)  return BTN_UP;
  if (adc_key_in < 380)  return BTN_DOWN;
  if (adc_key_in < 555)  return BTN_LEFT;
  if (adc_key_in < 790)  return BTN_SELECT;

  return BTN_NONE;
}