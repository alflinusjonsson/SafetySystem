#include <LiquidCrystal.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TimeLib.h>

/* LCD Circuit
 * D7 -> PIN2
 * D6 -> PIN3
 * D5 -> PIN4
 * D4 -> PIN5
 * E  -> PIN6
 * RS -> PIN7
 */
 
#define SDA 10
#define SCK 13
#define MOSI 11
#define MISO 12
#define RST 9
#define SAFE 1
#define UNSAFE 0
#define IN 1
#define OUT 0
#define DEADorALIVE 0
#define REACTORROOM 1.6

MFRC522 mfrc522(SDA, RST);
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
const int buttonPin = 8;
const int buzzerPin = A1;

int radiationInput = A0;
int radiationLevel = 0;
int radiationLevelMin = 0;
int radiationLevelMax = 1023;
int previousRad = 0;

int reading = 0;

int stamp = OUT;

long RPS_UNIT = 0;
char PPE = 'C';
int buttonState = 0; 

int state = SAFE;      // the current state of the output pin

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16,2);

  lcd.setCursor(15,0);
  lcd.print(PPE);
 
  // set up rfid
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.begin(9600);

  // set buzzer as output and turn it off
  pinMode(buzzerPin, OUTPUT);
  analogWrite(buzzerPin, 255);
 
  // set button as an input
  pinMode(buttonPin, INPUT);

  // set timer to 0 as default
  setTime(0,0,0,0,0,0000);
}

void timers(){
 
  lcd.setCursor(0,2);
  lcd.print(minute());
  lcd.print(":");
  lcd.print(second());
  lcd.print(" ");
  
  lcd.setCursor(1,2);
  lcd.print("Max: ");
  lcd.print(calculateRPS());

  if (calculateRPS() >= minute())
  {
    lcd.clear();
    lcd.print("WARNING! CHECK OUT!");
  }
}

void sendBluetooth(){
  
  Serial.println();
  Serial.print(stamp);
  Serial.print(",");
  Serial.print(minute());
  Serial.print(",");
  Serial.print(second());
  Serial.print(",");
  Serial.print(PPE);
  Serial.print(",");
  Serial.print(radiationLevel);
  Serial.print(",");
  Serial.print(DEADorALIVE);
}

int calculateRPS(){
  
  if (PPE == 'C'){
    RPS_UNIT = (REACTORROOM * radiationLevel)/1;
  }

  if (PPE == 'S'){
    RPS_UNIT = (REACTORROOM * radiationLevel)/5;
  }

  long maxAllowedDailyRad = 500000/RPS_UNIT;
  
  return maxAllowedDailyRad/60;
}

void loop() {

  // send information over bluetooth
  sendBluetooth();

  // read the state of radiation
  radiationLevel = analogRead(radiationInput);
  radiationLevel = map(radiationLevel, radiationLevelMin, radiationLevelMax, 1, 100);
  lcd.setCursor(14,2);
  if (radiationLevel != previousRad){
    lcd.print(radiationLevel);
  }
  previousRad = radiationLevel;
 
  reading = digitalRead(buttonPin);
  
  if (reading == HIGH) {
    delay(20);
    if (state == SAFE && reading == HIGH) {
      state = UNSAFE;
      lcd.setCursor(15,0);
      lcd.print("S");
      PPE = 'S';
      lcd.setCursor(14,2);
      lcd.print(radiationLevel);
      }
    else if (state == UNSAFE && reading == HIGH) {
      state = SAFE;
      lcd.setCursor(15,0);
      lcd.print("C");
      PPE = 'C';
      lcd.setCursor(14,2);
      lcd.print(radiationLevel);
    }   
  }

  if (stamp == IN){
    timers();
  }

  //look for card
  if ( !mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  //select one of the cards 
  if ( !mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  // show UID on monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;

  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();

  if (content.substring(1) == "96 FE 0A F8" && stamp == OUT)
  {
    analogWrite(buzzerPin, 0);
    delay(50);
    analogWrite(buzzerPin, 255);
    lcd.home();
    lcd.print("IN ");
    stamp = IN; 
    delay(1000); 
    
    setTime(0, 0, 0, 0, 0, 0000); 
  }

  else if (content.substring(1) == "96 FE 0A F8" && stamp == IN)
  {
    analogWrite(buzzerPin, 0);
    delay(50);
    analogWrite(buzzerPin, 255);
    lcd.home();
    lcd.print("OUT"); 
    stamp = OUT;
    delay(1000);
    
    setTime(0, 0, 0, 0, 0, 0000);
    lcd.setCursor(0,2);
    lcd.print("0");
    lcd.print(":");
    lcd.print("00");
   
  }
}
