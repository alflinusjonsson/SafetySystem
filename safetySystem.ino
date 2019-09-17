
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
#define REACTOR_ROOM 1
#define CONTROL_ROOM 2
#define BREAK_ROOM 3

MFRC522 mfrc522(SDA, RST);
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

const int clothButton = 8;
const int roomButton = A2;
const int buzzerPin = A1;
const int radiationInput = A0;

int radiationLevel = 0;
int radiationLevelMin = 0;
int radiationLevelMax = 1023;
int previousRad = 0;
int clothButtonV = 0;
int roomButtonV = 0;

int stamp = OUT;
long E = 0;
float rc = 1.6; // reactor room default
char pcV = 'C'; // normal cloth default
char room = 'R';

int pc = SAFE;      // default state protective cloth
int state = REACTOR_ROOM; // default room

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16,2);
 
  // set up rfid
  SPI.begin();
  mfrc522.PCD_Init();
  
  Serial.begin(9600);

  // set buzzer as output and turn it off
  pinMode(buzzerPin, OUTPUT);
  analogWrite(buzzerPin, 255);

  // set roomPin button as input
  pinMode(roomButton, INPUT);
 
  // set clothPin button as an input
  pinMode(clothButton, INPUT);

  // set timer to 0 as default
  setTime(0,0,0,0,0,0000);

  lcd.print("Please clock in");
}

void state_machine_run() 
{
  switch(state)
  {
   case REACTOR_ROOM:
      rc = 1.6;
      state = CONTROL_ROOM;
      room = 'R';
      lcd.setCursor(7,2);
      lcd.print("ROOM:");
      lcd.print(room);
   break;

   case CONTROL_ROOM:
      rc = 0.5;
      state = BREAK_ROOM;
      room = 'C';
      lcd.setCursor(7,2);
      lcd.print("ROOM:");
      lcd.print(room);
   break;

   case BREAK_ROOM:
      rc = 0.1;
      state = REACTOR_ROOM;
      room = 'B';
      lcd.setCursor(7,2);
      lcd.print("ROOM:");
      lcd.print(room);
   break;
  }
}

void sendBluetooth(){

  Serial.println();
  Serial.print("!");
  Serial.print(stamp);
  Serial.print(",");
  Serial.print(minute());
  Serial.print(",");
  Serial.print(second());
  Serial.print(",");
  Serial.print(pcV);
  Serial.print(",");
  Serial.print(radiationLevel);
  Serial.print(",");
  Serial.print(room);
  Serial.print(";");

  delay(200);
}

void loop() {

  // read the state of radiation
  radiationLevel = analogRead(radiationInput);
  radiationLevel = map(radiationLevel, radiationLevelMin, radiationLevelMax, 1, 100);
  if (stamp == IN){
    if (radiationLevel != previousRad){
        lcd.setCursor(10,0);
        lcd.print(radiationLevel);
        lcd.print("   ");
    }

    sendBluetooth();
  }
  previousRad = radiationLevel;
 
  clothButtonV = digitalRead(clothButton);
  roomButtonV = analogRead(roomButton);
  
  if (roomButtonV > 1000){
    delay(50);
    if (roomButtonV > 1000){
      state_machine_run();
    }
  }
  
  if (clothButtonV == HIGH) {
    delay(20);
    if (pc == SAFE && clothButtonV == HIGH) {
      pc = UNSAFE;
      lcd.setCursor(5,2);
      lcd.print("S");
      pcV = 'S';
      }
    else if (pc == UNSAFE && clothButtonV == HIGH) {
      pc = SAFE;
      lcd.setCursor(5,2);
      lcd.print("C");
      pcV = 'C';
    }   
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
  Serial.print("Message : Tech1");
  
  content.toUpperCase();

  if (content.substring(1) == "96 FE 0A F8" && stamp == OUT)
  {
    analogWrite(buzzerPin, 0);
    delay(50);
    analogWrite(buzzerPin, 255);
    lcd.clear();
    lcd.home();
    lcd.print("IN ");
    stamp = IN;
    Serial.print("IN");
    delay(2000);
    lcd.home();
    lcd.print("Select gear&room");
    delay(2000);
    lcd.clear();
    lcd.print("Radiation:");
    lcd.setCursor(0,2);
    lcd.print("GEAR:");
    lcd.setCursor(5,2);
    lcd.print(pcV);
    lcd.setCursor(7,2);
    lcd.print("ROOM:");
    lcd.print(room);
    delay(1000); 
    
    setTime(0, 0, 0, 0, 0, 0000); 
  }

  else if (content.substring(1) == "96 FE 0A F8" && stamp == IN)
  {
    analogWrite(buzzerPin, 0);
    delay(50);
    analogWrite(buzzerPin, 255);
    lcd.clear();
    lcd.home();
    lcd.print("OUT"); 
    stamp = OUT;
    Serial.print("OUT");
    sendBluetooth();
    delay(2000);
    lcd.clear();
    lcd.print("Please clock in");
    setTime(0, 0, 0, 0, 0, 0000);
   
  }
}
