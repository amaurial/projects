  /*
  LiquidCrystal Library:
 
 The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 

  Pins not in use: 0, 1, 4, A0 
 */

// include the library code:
#include <LiquidCrystal.h>
#include <Servo.h>
#include <avr/wdt.h>     //Watchdog Timer als Zeitgeber
#include <EEPROM.h>
#include <SPI.h>

const int AkkuVoltPin = A4;    //Messfühler Volt
const int AkkuStromPin = A5;   //Messfühler Strom

const int buttonUp = HIGH;    //Nicht gedrückt
const int buttonDown = LOW;    //gedrückt

const int downPin = 6;      //Button unten
const int upPin = 7;      //Button oben

const int FertigPin = A1;    //HIGH wenn die Ladung abgeschlossen ist

const int slaveSelectPin = 2;  //for SPI Device (Pin 11 = SDI und Pin 13 = Clk)

const int servoOnPin = 3;      //for Servo Power
const int servoPin = 5;    //Servo für Ladebox
Servo servo;              //create servo object to control a servo
const int servoclose = 180;    //Position für Ladung
const int servoopen = 19;     //Position kein Auto

// initialize the library with the numbers of the interface pins
//                RS, E, D4, D5, D6, D7
LiquidCrystal lcd(10, 12, A3, 8, A2, 9);

byte Akku0[8] = {
  B01110,
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111
};
byte Akku1[8] = {
  B01110,
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111
};
byte Akku2[8] = {
  B01110,
  B11111,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111
};

byte Akku3[8] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

int Modus = 0;  //Aktuelle Modus des Laders:
/*
  0: kein Akku, keine Ladung, einstellen des Displaykontrast
  1: kein Akku, keine Ladung, einstellen Ladestrom (mA) möglich (6 Stufen: Normal, C/8, C/4, C/2, C, 2C) - Einstellung...
  1: kein Akku, keine Ladung - Ladebereit...
  2: Servo geschlossen ->
  3: Akku, keine Ladung, einstellen Akkustrom anpassen (mAh) möglich - Akkuanpassung...
  4: Akku, Ladung, Abbruch möglich - Ladung...
  5: Servo öffnen ->
  6: Akku, Ladung Ende/Abbruch - Fertig...
  7 == 1
*/

boolean buttonstateEnter = buttonUp;

int maxstrom = 0;  //Strom der von der Kapazität festgelegt wird.

int ladetimesec = 0;   //Ladezeit Sekunden
int ladetime = 0;      //Ladezeit in Minuten

float ladevolt = 0.00;
int ladestrom = 0;
float lademaxvolt = 0;      //maximale Ladespannung

int voltout = 0;    //Spannung die am Ausgang liegt 0-255
float akkuvolt = 0.00;      //Akkuspannung
int akkukap = 0;    //Kapazität
float akkumaxvolt = 0;      //maximale Akkuspannung

//Einstellungen
int lcdkontrast = 245;  //Kontrast des LCD
const int EEPROMkont = 11;    //Register für Kontrast
int einstmodus = 0;  //Modus in dem sich die Einstellungen befinden;
const int EEPROMeinst = 10;    //Register für Einstellungsmodus

//****************************************************************  
void setup() {
  
  pinMode (A2, OUTPUT);
  pinMode (A3, OUTPUT);
  
  // initialize SPI:
  pinMode (slaveSelectPin, OUTPUT);  // set the slaveSelectPin as an output:

  setbinaer(0);    //Ausgangsspannung für Ladung abschalten.
  
  setup_watchdog(6);   // set a timer of length 1 Sekunde
  
  analogReference(INTERNAL);    //Refernzspannung auf 1,1 Volt stellen
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.createChar(1, Akku0);
  lcd.createChar(2, Akku1);
  lcd.createChar(3, Akku2);
  lcd.createChar(4, Akku3);
  lcd.home(); 

  Serial.begin(9600);
  Serial.println("Akkuladegeraet");
  
  servo.attach(servoPin);
  servo.write(servoopen);
  pinMode(servoOnPin, OUTPUT);
  
  pinMode(FertigPin, OUTPUT);
  
  int value = EEPROM.read(EEPROMeinst);
  if (value < 6)
    einstmodus = value;
  value = EEPROM.read(EEPROMkont);
  lcdkontrast = value;
}

//****************************************************************  
void lcdladezeit (int zeichen, int zeile, int minuten) {
  if (Modus >= 4) {      //Ab Ladung einblenden
    int h = minuten / 60;    
    int m = minuten % 60;    //Rest
    lcd.setCursor(zeichen, zeile);
    lcd.leftToRight();
    if (h < 10)
      lcd.write("0");
    lcd.print(h);
    if (m < 10)
      lcd.write("0");
    lcd.print(m);  
  }  
}

//****************************************************************  
//Anzeige von Volt und Strom
void lcddaten (int zeichen, int zeile, float volt, int strom) {
  lcd.setCursor(zeichen, zeile);
  lcd.leftToRight();
  lcd.print(volt);
  lcd.write("V ");
  lcd.setCursor(zeichen + 6, zeile);
  lcd.write("   ");    //Zahlen löschen
  lcd.setCursor(zeichen + 6, zeile);
  if (strom < 100) 
   lcd.setCursor(zeichen + 7, zeile);
  if (strom < 10)
   lcd.setCursor(zeichen + 8, zeile); 
  lcd.print(strom);
  lcd.write("mA");
} 

//****************************************************************  
void lcdladedaten (int zeichen, int zeile, float volt, int strom) {
  lcddaten(zeichen, zeile, volt, strom);
}

//****************************************************************  
void lcdakkudaten (int zeichen, int zeile, float volt, int kap) {
  if (Modus >= 2) {
    lcddaten(zeichen, zeile, volt, kap);  //Volt und mA ausgeben
    lcd.write("h");  //Kapazität (mAh)
  }
  else {
    lcd.setCursor(zeichen, zeile);
    lcd.leftToRight();
    lcd.write("kein Akku");
  }
}

//****************************************************************  
//Akkuvisualisierung, mit volt = 0 wird ein Laufbalken dargestellt mit ladetimesec
void lcdakkuanzeige (int zeichen, int zeile, float volt = 0) {
  lcd.setCursor(zeichen, zeile);
  lcd.leftToRight();
  //Akkuladung, Laufbalken anzeigen:
  if (volt < 2.49) {    //min Volt?
    lcd.write((ladetimesec % 4) + 1);  //Ladebalken
    return;
  }
  if (volt < 3.63) {
    lcd.write(1);  //leer
    return;
  }
  if (volt < 3.8) { 
    lcd.write(2);  //halb
    return;
  }
  if (volt < 3.95) {
    lcd.write(3);  //mittel  
    return;
  }
  if (volt < 4.3) {
    lcd.write(4);  //voll
    return;
  }
}

//****************************************************************  
void lcdsetkontrast() {
  lcd.clear();
  lcd.home();
  lcd.leftToRight();
  lcd.write("Einstellungen");
  lcd.setCursor(0,1);
  lcd.write("Kontrast: ");
  lcd.print(lcdkontrast);
  digitalPotWrite(4, 255-lcdkontrast);    //Kontrast des LCD festlegen
}

//****************************************************************  
void lcdeinstellungen(int modus) {
  lcd.clear();
  lcd.home();
  lcd.leftToRight();
  lcd.write("Einstellungen");
  lcd.setCursor(0,1);
  lcd.write("laden: ");
  switch (modus) {   //(6 Stufen: Normal, C/8, C/4, C/2, C, 2C)
    case 0: lcd.write("normal");
            break;
    case 1: lcd.write("C/8");
            break;        
    case 2: lcd.write("C/4");
            break;       
    case 3: lcd.write("C/2");
            break;      
    case 4: lcd.write("1C");
            break;        
    case 5: lcd.write("2C");
            break;        
  }
}

//****************************************************************  
void lcdakkufertig (int zeichen, int zeile) {
  lcd.setCursor(zeichen, zeile);
  lcd.leftToRight();
  lcd.write("Ende. Dauer:");
}

//****************************************************************  
void lcdakkukapeinst (int zeichen, int zeile) {
  lcd.setCursor(zeichen, zeile);
  lcd.leftToRight();
  lcd.write("Kapazitaet?");
}

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9=8 sec
void setup_watchdog(int ii) {
  byte bb;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}

//****************************************************************  
// Watchdog Interrupt Service / is executed when watchdog timed out
// Timeout muss hier abgefangen werden, sonst wird ein Reset ausgelöst!
ISR(WDT_vect) {
  ladeintervall();
}

//****************************************************************  
void ladeintervall() {
  if (Modus == 4) {    //Akku wird geladen...
    if (ladetimesec == 0 && ladetime == 0) {   //Start der Ladung
      Serial.print("Akku: ");
      Serial.print(akkuvolt);
      Serial.print("V, ");
      Serial.print(akkukap);
      Serial.println("mAh");
      Serial.print("Laden mit maximal: ");
      Serial.print(maxstrom);      //maximalen Ladestrom ausgeben
      Serial.println("mA");
    }
    ladetimesec++;    //Sekunden hochzählen
    if (ladetimesec >= 60) {  //
      ladetimesec = 0;
      ladetime++;      //Minuten hochzählen
      if (ladevolt > lademaxvolt && ladetime > 25)
        lademaxvolt = ladevolt;      //maximale Ladespannung sichern

      Serial.print(ladetime);
      Serial.print("min, Akku: ");
      Serial.print(akkuvolt);
      Serial.print("V, mit ");
      Serial.print(ladevolt);
      Serial.print("V ");
      Serial.print(ladestrom);
      Serial.print("mA - max:");
      Serial.print(lademaxvolt);
      Serial.print("V - ");
      Serial.println(voltout);
    }
  }
}

//****************************************************************  
//Ladeservo ansteuern
void setServo(boolean pos) {
  if (pos == true)
    servo.write(servoclose);  //Ladebacken schließen
  else servo.write(servoopen); //Ladebacken öffnen
  digitalWrite(servoOnPin, HIGH);      //Servo aktivieren
  delay(1000);    //warten bis Servo Position erreicht hat
  digitalWrite(servoOnPin, LOW);      //Servo deaktivieren
}

//****************************************************************  
//Ausgangsspannung für Ladung ausgeben/einstellen
void setbinaer(int wert) {   
  if (wert > 255)    //maximal 8 Bit zulassen
    wert = 255;
  if (wert < 0)
    wert = 0;  
  digitalPotWrite(5, wert);     //Ausgangsspannung für Ladung
}

//****************************************************************  
float getVolt() {
  float volt = analogRead(AkkuVoltPin);
  volt = volt * 0.575;      //Skala anpassen
  volt = volt / 100;       //Kommastelle einrücken
  return volt;
}

//****************************************************************  
int getStrom() {
  float strom = analogRead(AkkuStromPin);
  strom = strom * 2.0;      //Anpassen an analogRead -> Volt
  strom = strom / 1.8;      //Anpassen an analogRead -> Volt
  strom = strom / 1.5;      //Teilen durch den Widerstandswert
  strom = strom - 4.0;      //LED Strom abziehen
  if (strom < 0)        //Wenn nur LED an ist, nicht mit anzeigen!
    strom = 0;
  return strom;
}

//****************************************************************  
boolean getAkkuVolt() {
  setbinaer(0);             //Ladespannung abschalten;
  delay(60);
  float Volt = getVolt();    //Wert einlesen
//  Volt = Volt + 0.04;  //Ausgleichswert zum Anpassen!!!
  Volt = Volt + 0.07;  //Ausgleichswert zum Anpassen!!!
  setbinaer(voltout);    //aktuelle Ladespannung einschalten
  if (Volt < 2.90) 
    return false;
  if (Modus == 4) {  //bei Ladung  
    if (Volt > akkuvolt)  //nur Spannungsanstieg setzten
      akkuvolt = Volt;   //Akkuspannung setzten
  }
  else akkuvolt = Volt;    //Akkuspannung setzten
  if (akkumaxvolt < akkuvolt)
    akkumaxvolt = akkuvolt;
  return true;  
}

//****************************************************************  
boolean getAkkuDaten() {      //Ermitteln der Akkuspannung und Strom für Ladung
  if (getAkkuVolt() == false)  //Akkuspannung bestimmen
    return false;    //kein Akku vorhanden
   //Kapazität versuchen zu bestimmen:
   
   setbinaer(110);   //Ladespannung einschalten auf ca. 4,25Volt
   delay(300);      //Kapazität des Akkus bestimmen.
   float Strom1 = getStrom();
   delay(50);
   float Strom2 = getStrom();
   delay(50);
   float Strom3 = getStrom();
   float Strom = (Strom1 + Strom2 + Strom3) / 3;
   akkukap = 0;          
   if (akkuvolt < 3.80) {
     if (Strom > 15) 
       akkukap = 80;
     if (Strom > 40)
       akkukap = 160;
     if (Strom > 140)
       akkukap = 300;
   }
   if (akkukap == 0)      //Falls kein aussagekräftiger Wert bestimmt wurde
     akkukap = Strom * 2;  
 
   setbinaer(0);    //Ladespannung abschalten
   return true;  //Akku vorhanden, Daten bestimmt!
}

//**************************************************************** 
//maximalen Ladestrom bestimmen
void getmaxstrom() {
  maxstrom = akkukap;
  switch (einstmodus) {
    case 0: maxstrom = maxstrom / 10;  //C/10 - normal
            break;
    case 1: maxstrom = maxstrom / 8;  //C/8
            break;   
    case 2: maxstrom = maxstrom / 4;  //C/4
            break;        
    case 3: maxstrom = maxstrom / 2;  //C/2
            break;     
    case 4: maxstrom = maxstrom;  //1C
            break;        
    case 5: maxstrom = maxstrom * 2;  //2C
            break;        
  }
}

//**************************************************************** 
void ladungsteuern() {
  //Ladespannung setzten
  setbinaer(voltout);
  delay(100);
  ladestrom = getStrom();
  ladevolt = getVolt(); 
  ladevolt = ladevolt - 0.06;  //Ausgleichswert
 
  //Stromregelung der Ladung
  if ((ladestrom - 3) < (maxstrom - 3)) {    //Spannung für Strom anpassen
    voltout++;                    //Ladespannung erhöhen
  }
/*  if ((ladestrom) > maxstrom) {
    voltout--;
    if (voltout < 0)
      voltout = 0;
  }
  */
  
  //Abschaltung der Ladung
  if ((einstmodus == 0 && ladetime >= 800) || (akkuvolt > 4.74)) { //Ladung beenden, nach 13,x Stunden - Normalladung
    Serial.println("Normalladung beendet!");
    voltout = 0;
//    setbinaer(0); //Ladespannung setzten
    moduswechsel(5);
  }
  if (ladetime >= 30 && einstmodus != 0) {    //Schnellladung abschaltung, keine Normalladung!
    if (410 / einstmodus < ladetime) {    //Ladezeit überschritten
      Serial.println("Ladung Ende! Zeit abgelaufen."); 
      voltout = 0;
      moduswechsel(5);
    } 
    if ((lademaxvolt-0.02) >= ladevolt) {  //Akkuvolt voll
      Serial.println("Ladung beendet!");    
      voltout = 0;
//      setbinaer(0); //Ladespannung setzten
      moduswechsel(5);
    }
  } 
}

//**************************************************************** 
//Bestätigung prüfen
boolean checkEnterButton() {
  int stateUp = digitalRead(upPin);      //Einlesen Up Button
  int stateDown = digitalRead(downPin);  //Einlesen Down Button
  if (stateUp == buttonDown && stateDown == buttonDown && buttonstateEnter == buttonUp) {  //Setzten
    buttonstateEnter = buttonDown;
    return true;
  }
  if (stateUp == buttonUp && stateDown == buttonUp && buttonstateEnter == buttonDown)   //Rücksetzten
    buttonstateEnter = buttonUp;
  
  //Serial Prüfen?
  if (Serial.available()) {
    int inbyte = Serial.read();
    if (inbyte = '\n')    //Enter per Serial?
      return true;
  }
    
  return false;
}

//**************************************************************** 
//Bestätigung Up prüfen
boolean checkUpButton() {
  int stateUp = digitalRead(upPin);      //Einlesen Up Button
  int stateDown = digitalRead(downPin);  //Einlesen Down Button
  if (stateUp == buttonDown && stateDown == buttonUp) {  //nach oben 
    return true;
  }
  return false;  
}

//**************************************************************** 
//Bestätigung Down prüfen
boolean checkDownButton() {
  int stateUp = digitalRead(upPin);      //Einlesen Up Button
  int stateDown = digitalRead(downPin);  //Einlesen Down Button
  if (stateUp == buttonUp && stateDown == buttonDown) { //nach unten
    return true;
  }
  return false;  
}

//****************************************************************  
void kontrastbutton() {
  if (checkDownButton() == true) {  //nach unten
    lcdkontrast--;
    if (lcdkontrast < 0)
      lcdkontrast = 0;
  }
  if (checkUpButton() == true) {  //nach oben 
    lcdkontrast++;
    if (lcdkontrast > 255)
      lcdkontrast = 255;
  }
  if (checkEnterButton() == true) {
    int value = EEPROM.read(EEPROMkont);
    if (value != lcdkontrast)
      EEPROM.write(EEPROMkont, lcdkontrast);
    moduswechsel(1);
  }
}

//****************************************************************  
//Buttonsteuerung unter Menü Einstellungen
void einstellungenbutton() {
  if (checkDownButton() == true) {  //nach unten laufen
    einstmodus++;
    if (einstmodus > 5)
      einstmodus = 0;
  }
  if (checkUpButton() == true) {  //nach oben laufen
    einstmodus--;
    if (einstmodus < 0)
      einstmodus = 5;
  }
  if (checkEnterButton() == true) {  //Akku suchen?
    int value = EEPROM.read(EEPROMeinst);
    if (value != einstmodus)
      EEPROM.write(EEPROMeinst, einstmodus);
    moduswechsel(2);
  }
}

//****************************************************************  
//Kapazität des Akkus ändern:
void akkukapbutton() {
  if (checkDownButton() == true) {  //nach unten laufen
    akkukap -= 10;
    if (akkukap < 10)
      akkukap = 10;
  }
  if (checkUpButton() == true) {  //nach oben laufen
    akkukap += 10;
    if (akkukap > 990)
      akkukap = 990;
  }
  //akkukap runden
  int neu = akkukap % 10;      
  if (neu != 0) {
    if (neu > 5) 
      akkukap = akkukap + (10 - neu);
    else akkukap = akkukap - neu;  
  }
  if (checkEnterButton() == true) {  //Ladung starten..
    getmaxstrom();
    moduswechsel(4); 
  }
}

//****************************************************************  
void ladebutton() {
  if (checkEnterButton() == true) {  //Ladung Abbruch
    moduswechsel(5); 
    voltout = 0; 
  }
}

//****************************************************************  
//LAdungsdaten zurücksetzten nach Abschluss der Ladung
void ladungreset() {
  ladetimesec = 0;
  ladetime = 0;
  akkumaxvolt = 0;
}

//****************************************************************  
void moduswechsel (int toModus) {
  if (toModus != 2 && toModus != 6)  //nicht bei Servosteuerung
    lcd.clear();
  Modus = toModus;
  Serial.print("Modus ");
  Serial.print(toModus);
  switch(toModus) {
    case 0: Serial.println(" Einstellungen");
            break;
    case 1: Serial.println(" Ladebereit");
            break;
    case 2: Serial.println(" Servo: close");
            break;        
    case 3: Serial.println(" Akkuanpassung");
            break;
    case 4: Serial.println(" Ladung");
            break;
    case 5: Serial.println(" Servo: open");
            break;
    case 6: Serial.println(" Fertig");
            break;
  }
}

//****************************************************************  
void loop() {
  if (Modus > 1) {  //Normalbetrieb:
    lcdakkudaten(0,0, akkuvolt, akkukap);
    delay(200);
  }
  else {  //Einstellungen:
    delay(100);
  }
  
  //LCD Kontrast einstellen
  if (Modus == 0) {
    lcdsetkontrast();
    kontrastbutton();
  }
  
  //kein Akku, keine Ladung, einstellen Ladestrom (mA) möglich (6 Stufen: Normal, C/8, C/4, C/2, C, 2C) - Einstellung...
  if (Modus == 1) {
    lcdeinstellungen(einstmodus);
    einstellungenbutton();
  }
  
  //kein Akku, keine Ladung - Ladebereit...
  if ((Modus < 2) && (getAkkuDaten() == true)) {    //Bei Akku in Modus 2 wechseln!
    int value = EEPROM.read(EEPROMeinst);      //Akkuladungsgeschwindigkeit:
    if (value != einstmodus)
      EEPROM.write(EEPROMeinst, einstmodus);
    value = EEPROM.read(EEPROMkont);           //LCD Kontrasteinstellung
    if (value != lcdkontrast)
      EEPROM.write(EEPROMkont, lcdkontrast);
    moduswechsel(2);
  }
  
  if (Modus == 2) {  //Servo geschlossen ->
    digitalWrite(FertigPin, LOW);
    setServo(true);
    delay(800);
    if (getAkkuVolt() == true) { //Akku vorhanden 
      moduswechsel(3);
    }
    else moduswechsel(5);  
  }
  if (Modus == 3) {  //Akku, keine Ladung, einstellen Akkustrom anpassen (mAh) möglich - Akkuanpassung...
    lcdakkuanzeige(15,0, akkuvolt);      //Ladezustand Akku
    lcdakkukapeinst(0,1);      //Kapazität nachfragen?
    akkukapbutton();
    if (getAkkuVolt() == false) {  //Akkuspannung bestimmen
      moduswechsel(5);
    }
  }
  if (Modus == 4) {  //Akku, Ladung, Abbruch möglich - Ladung...
    lcdladedaten(0,1, ladevolt, ladestrom);
    lcdladezeit(12,1, ladetime);
    lcdakkuanzeige(15,0);          //Ladezustand Akku ladend
    ladungsteuern();            //Ladespannung bestimmen!
    ladebutton();        //Abbruch?
    if (getAkkuVolt() == false || akkukap < 50) {  //Akkuspannung prüfen, Akkukap prüfen
      moduswechsel(5);
      voltout = 0;
    }

  }
  if (Modus == 5) {  //Servo öffnen ->
    setServo(false);
    lcdakkuanzeige(15,0, akkuvolt);      //Ladezustand Akku
    lcdakkufertig(0,1);    //Ladung beendet einblenden
    lcdladezeit(12,1, ladetime);    //Ladedauer
    
    if (getAkkuVolt() == false) {   //Akkuspannung bestimmen
      if (ladetime != 0 || ladetimesec != 0) {
        lcdakkudaten(0,0, akkuvolt, akkukap);    //werden sonst nicht mit dargestellt bei Abbruch
        delay(3000);      //Etwas warten wenn Akku Abgezogen wurde...Daten bleiben auf LCD!
      }
      moduswechsel(1);    //kein Akku
    }
    else moduswechsel(6);  //Achtung kein LCD löschen.
    ladungreset();    //Ladedaten zurücksetzten
  } 
  if (Modus == 6) {  //Akku, Ladung Ende/Abbruch - Fertig...
    digitalWrite(FertigPin, HIGH);
    if (getAkkuVolt() == false) { //Akkuspannung bestimmen
      moduswechsel(1);
    }
  }
}

//****************************************************************  
int digitalPotWrite(int address, int value) {
  SPI.begin(); 
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin,LOW);
  //  send in the address and value via SPI:
  SPI.transfer(address);
  SPI.transfer(value);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin,HIGH); 
  SPI.end(); 
}









