//include libraries
#include <Wire.h>
//DHT11
#include <DHT.h>
//LCD_12C
#include <LiquidCrystal_I2C.h>
//RTC
#include <RTClib.h>


//DHT11
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);  // Initializes DHT11 on pin 2

//LCD
#define I2C_ADDR 0x3c
#define BRIGHT 127
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initializes LCD at I2C 0x27 with 16(column)x2(row) characters

//RTC
RTC_DS3231 RealTimeClock;
#define rtc RealTimeClock

//LDR
#define LDR_PIN A1     // LDR analog pin
#define LDR_DIGITAL 4  // LDR digital pin

//Music
#define BUZZER_PIN 6  // Connect buzzer  to pin 6

void playMelody() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(BUZZER_PIN, LOW);   // Turn buzzer ON
    delay(200);                      // Wait
    digitalWrite(BUZZER_PIN, HIGH);  // Turn buzzer OFF
    delay(100);                      // Pause between beeps
  }
}


//Toggle
unsigned long latestToggleTime = 0;  // Initiate timer to track last toggle
bool ShowMoisture = false;         // indicate toggle flag false = light, true = moisture


//Timer
int counter = 0;  // counter for a loop

//LED
int led = 7;

//Ultrasonic  Sensor
int Triggerpin = 8;
int Echopin = 9;
int Duration;
float Distance;


//moisture sensor
int water_count = 0;  // Counter dry soil detections

//Relay
int Relay = 13;  // Relay for water pump

void setup() {

  //DHT11
  Serial.begin(9600);  // Start Serial for debugging
  Wire.begin();        // Start I2C

  //Buzzer
  pinMode(BUZZER_PIN, OUTPUT);     // Set buzzer pin
  digitalWrite(BUZZER_PIN, HIGH);  // Ensure buzzer is OFF
  dht.begin();                     // Start DHT sensor
  delay(100);

  //LCD
  lcd.init();       // Start LCD
  lcd.backlight();  // Turn on backlight
  lcd.clear();
  lcd.setCursor(0, 0); // Display 1st row
  lcd.print("Wait a minutes.");

  //RTC
  if (!rtc.begin()) { //Start RTC
    Serial.println("RTC not found!");
  }
  DateTime now = rtc.now(); // Read current time
  Serial.println(now.timestamp());

  //LDR
  pinMode(LDR_DIGITAL, INPUT);


  //Ultrasonic Sensor
  pinMode(Triggerpin, OUTPUT);
  pinMode(Echopin, INPUT);

  //LED
  pinMode(led, OUTPUT);

  //Relay
  pinMode(Relay, OUTPUT);
  digitalWrite(Relay, HIGH); // Off initially
}

void loop() {

  //Check temperature, humidity, and soil humidity once a minute(60 sec).

  if (counter == 1) {
    checkDHT11();
    checkWater();
    checkMoisture();
  }

  delay(10000);  //10sec
  counter++;

  if (counter >= 6) {
    counter = 0;   // Reset every 60 seconds
  }

  checkWater();
}

//DHT11
void checkDHT11() {
  float t = dht.readTemperature();  // Temperature
  float h = dht.readHumidity(); // Humidity
  int lighting = analogRead(LDR_PIN);  // Light
  DateTime now = rtc.now(); // Time
  int moisture = analogRead(A0);  // Soil moisture

  if (isnan(t) || isnan(h)) { // Check if not a number
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");

  Serial.print("Time: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.println(now.minute());

  Serial.print("Light: ");
  Serial.println(lighting);

  lcd.setCursor(0, 0); // Set row 1
  if (now.hour() < 10) lcd.print("0");
  lcd.print(now.hour()); // hour
  lcd.print(":");
  if (now.minute() < 10) lcd.print("0");
  lcd.print(now.minute()); //mins
  lcd.print(" ");
  lcd.print(String(t) + "C  ");  // temp

  if (millis() - latestToggleTime >= 5000) { // Millies when Arduino started running and lasttoggle value
    ShowMoisture = !ShowMoisture; // Toggle
    latestToggleTime = millis(); // Update lasttoggle value
  }

  lcd.setCursor(0, 1); // Set row 2
  lcd.print("H:");
  lcd.print((int)h);
  lcd.print("% ");
  if (!ShowMoisture) {
    lcd.print("L: ");
    lcd.print(lighting); // light
  } else {
    lcd.print("M:");
    lcd.print(moisture); //moisture
  }
  lcd.print("     ");
}


//Ultrasonic Sensor
void checkWater() {

  digitalWrite(Triggerpin, LOW);
  delayMicroseconds(1);
  digitalWrite(Triggerpin, HIGH);
  delayMicroseconds(11);  // Pulse to measurement
  digitalWrite(Triggerpin, LOW);
  Duration = pulseIn(Echopin, HIGH); //

  if (Duration > 0) {
    Distance = Duration / 2; // one way travel
    Distance = Distance * 0.034;  // ultrasonic  speed is 340m/s = 34000cm/s = 0.034cm/us
    Serial.print(Distance);
    Serial.println("  cm");

    if (Distance > 12) {
      lcd.setCursor(0, 0);
      lcd.print("Refill Water!   ");
      digitalWrite(led, HIGH); // Water low
    } else {
      digitalWrite(led, LOW);
    }
  }
}

//moisture sensor
void checkMoisture() {

  //Measure soil humidity
  int moisture = analogRead(A0); // 0-1023(0-5V)
  Serial.print("Moisture Sensor Value:"); // Serial Monitor to identify the data
  Serial.println(moisture);

  if (moisture <= 300) {
    water_count++;
    Serial.print("Water Count: ");
    Serial.println(water_count);
    if (water_count == 5) { // watering if reach 5
      watering();
      water_count = 0;
    }
  }
}


//Watering
void watering() {
  Serial.println("Relay ON");   // Pump ON
  digitalWrite(Relay, LOW);
  delay(10000);
  Serial.println("Relay OFF"); // Pump OFF
  digitalWrite(Relay, HIGH);
  delay(10000);
  completeWatering(); // Call completeWatering
  counter++;
}

//Music
void completeWatering() {

  lcd.setCursor(0, 0);
  lcd.print("   Thank  you!   ");
  lcd.setCursor(0, 1);
  lcd.print("   (^ - ^)/     ");

  playMelody();
  delay(4000);

  counter++;
  lcd.setCursor(0, 0);
  lcd.print("                  ");
  lcd.setCursor(0, 1);
  lcd.print("                  ");
  checkDHT11();
  int moisture = analogRead(A0);
  lcd.setCursor(0, 1);
  lcd.print("Moisture:  " + String(moisture) + "   ");  // Refresh LCD after watering
}
