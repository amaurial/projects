/*
  6-fach Servo Dekoder
  Kann Servos zwischen zwei Positionen hin und her bewegen.
  Jede Position kann dabei verschiedene Adressen besitzen.
  Servos können nur mit Zubehör-Adressen und jeweiligem Port geschaltet werden.
  
  aktuelle Version 0.3e
  28.12.2011
  
  Versionshierachie:
  v0.3e 28.12.11 Andreaskreuz Licht für Servo 1 auf vier Ausgänge
  v0.2e 28.12.11 Programmierung mit drei Taster implementiert. (ohne EEPROM)
  v0.1e 26.12.11 Steuerung von vier Servos mittels konstanten Adressen und Positionen

  Author: Philipp Gahtow
*/

// include the library code:
//#include <TimerOne.h>
#include <Servo.h> 

//--------------------------------------------------------------
//Pinbelegungen am Dekoder:
  //Eingänge:
const int dccPin = 2;      //nur auf Pin 2 geht die attachInterrupt Funktion!
    //Taster:
const int UpTasterPin = 0;    //Down (-) Taster
const int DownTasterPin = 1;  //Up (+) Taster
const int PTasterPin = 3;    //Programm Taster

  //Ausgänge:
const int A1Pin = 16;    //Ausgang 1  
const int A2Pin = 17;    //Ausgang 2
const int A3Pin = 18;    //Ausgang 3
const int A4Pin = 19;    //Ausgang 4

//Servo-PIN:
const int s1Pin = 6;
const int s2Pin = 9;
const int s3Pin = 10;
const int s4Pin = 11;
const int s5Pin = 4;
const int s6Pin = 7;

//GND-PIN für Servos:
const int s1EPin = 5;   //Servo1 GND Enable
const int s2EPin = 8;   //Servo2 GND Enable
const int s3EPin = 13;  //Servo3 GND Enable
const int s4EPin = 12;  //Servo4 GND Enable
const int s5EPin = 14;  //Servo5 GND Enable
const int s6EPin = 15;  //Servo6 GND Enable

//--------------------------------------------------------------
//Variablen für Programmierung und Positionsspeicherung
boolean ProgMode = false;  //Programmierung
int ProgPos = 0;      //Position A, B oder C des Servos der Programmiert wird.
int ProgServo = 0;    //Servo der Programmiert wird
int ProgWinkel = 0;    //Drehposition des Servos.
//Adressenverwaltung für Servos beim Programmieren
boolean ProgAdrNeu = false;  //Neue Adresse wurde gesendet
int ProgAdr = 0;      //neue Adresse
int ProgPort = 0;     //neuer Port 
int ProgAktiv = 0;     //neuer Zustand   

//--------------------------------------------------------------
//Variablen für DCC Erkennung
boolean getdcc = false;
int countone = 0;    //Zähle die high so das man eine Präambel erkennen kann (min. 10 high)
boolean getdata = false;     //Es werden DCC Daten gelesen und im Array data abgespeichert
int data[45];                //eingelesene Bit-Werte nach einer Präambel
int datalength = 0;  //Position in data wo gerade der nächste Wert hinkommt
int countbit = 0;    //Zähle Bits, nach 8 Bit wurde ein Byte gelesen und setzte dann die Variable auf 0

//--------------------------------------------------------------
//Globale Servo Variablen:
Servo servo1;  // create servo object to control a servo1 
Servo servo2;  // create servo object to control a servo2 
Servo servo3;  // create servo object to control a servo3 
Servo servo4;  // create servo object to control a servo4 
Servo servo5;  // create servo object to control a servo5 
Servo servo6;  // create servo object to control a servo6 
                // a maximum of eight servo objects can be created

//Position: 0 = undefiniert, 1 = eine Endlage, 2 = andere Endlage
int s1 = 0;    //Aktuelle Position Servo1
int s2 = 0;    //Aktuelle Position Servo2
int s3 = 0;    //Aktuelle Position Servo3
int s4 = 0;    //Aktuelle Position Servo4
int s5 = 0;    //Aktuelle Position Servo5
int s6 = 0;    //Aktuelle Position Servo6

//Ablaufzeit für aktiven GND aktiv:                
unsigned long time1 = 0;
unsigned long time2 = 0;
unsigned long time3 = 0;
unsigned long time4 = 0;

//Enable Time:
int sEnableTime = 50;

//--------------------------------------------------------------
//Servo1:
int s1AdrA = 1;    //Zubehöradresse
int s1PortA = 3;   //Port 
int s1AktivA = 1;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s1posA = 90;  //PositionA 
int s1AdrB = 1;    //Zubehöradresse
int s1PortB = 3;   //Port 
int s1AktivB = 0;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s1posB = 64;  //PositionB

//Servo2:
int s2AdrA = 1;    //Zubehöradresse
int s2PortA = 1;   //Port 
int s2AktivA = 1;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s2posA = 82;  //PositionA
int s2AdrB = 1;    //Zubehöradresse
int s2PortB = 1;   //Port 
int s2AktivB = 0;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s2posB = 59;  //PositionB

//Servo3:
int s3AdrA = 1;    //Zubehöradresse
int s3PortA = 2;   //Port 
int s3AktivA = 1;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s3posA = 76;  //PositionA
int s3AdrB = 1;    //Zubehöradresse
int s3PortB = 2;   //Port 
int s3AktivB = 0;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s3posB = 92;  //PositionB

//Servo4:
int s4AdrA = 1;    //Zubehöradresse
int s4PortA = 4;   //Port 
int s4AktivA = 1;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s4posA = 76;  //PositionA
int s4AdrB = 1;    //Zubehöradresse
int s4PortB = 4;   //Port 
int s4AktivB = 0;   //auf Adresse als Aktiv = 1, oder Inaktiv = 0 reagieren
int s4posB = 92;  //PositionB

//--------------------------------------------------------------
unsigned long count = 0;        //Zeit für Verzögerung die ca. alle 10ms um 1 erhöht wird.
unsigned long fastcount = 0;    //Verzögerung für etwa 10ms

boolean AndreasOn = false;    //Blinken Andreaskreuz

//-------------------------------------------------------------- 
void setup()
{
  pinMode(dccPin, INPUT);    //Dateneingang
  attachInterrupt(0, dccdata, RISING);  //ISR für den Dateneingang
  
  pinMode(DownTasterPin, INPUT);    //Down-Taster
  pinMode(UpTasterPin, INPUT);    //Up-Taster
  pinMode(PTasterPin, INPUT);    //Programm Taster
  attachInterrupt(1, tasterdown, FALLING);  //ISR für alle Taster! (PTasterPin)
  
  pinMode(A1Pin, OUTPUT);      //Ausgang 1
  pinMode(A2Pin, OUTPUT);      //Ausgang 2
  pinMode(A3Pin, OUTPUT);      //Ausgang 3
  pinMode(A4Pin, OUTPUT);      //Ausgang 4
  
  digitalWrite(s1EPin, LOW);
  pinMode(s1EPin, OUTPUT);
  digitalWrite(s2EPin, LOW);
  pinMode(s2EPin, OUTPUT);
  digitalWrite(s3EPin, LOW);
  pinMode(s3EPin, OUTPUT);
  digitalWrite(s4EPin, LOW);
  pinMode(s4EPin, OUTPUT);
  
  //Timer1 zum bestimmen der Impulslängen des DCC Signals
//  Timer1.initialize(70);   // set a timer of length 70 microseconds
//  Timer1.attachInterrupt(dcctime); // attach the service routine here
//  Timer1.stop();              //Timer anhalten
  
  Serial.begin(115200); //9600);
  Serial.println("Servodekoder");
  
  //Servos den Pins zuweisen
  servo1.attach(s1Pin);  // attaches the servo1 on pin 6 to the servo object 
  servo2.attach(s2Pin);  // attaches the servo2 on pin 9 to the servo object 
  servo3.attach(s3Pin);  // attaches the servo3 on pin 10 to the servo object 
  servo4.attach(s4Pin);  // attaches the servo4 on pin 11 to the servo object 
}

//-------------------------------------------------------------- 
void loop()
{ 
  fastcount++;
  
  if (getdcc == true) {      //Wenn eine Steigene Flanke erkannt wurde
    delayMicroseconds(50);    //Abwarten bis ca. 70µs vergangen sind
    dcctime();                //Status des Eingangs auswerten
    getdcc = false;         //dcc erkennung zurücksetzten, warten auf nächtes Bit 
    fastcount = fastcount + 3200;    //vertrödelte Zeit ausgleichen
  }  
  
  if (fastcount > 80000) {    //immer im Intervall von ca. 10 ms Durchlaufen
    count++;              //Servo Zeit erhöhen.
    fastcount = 0;           //Intervallzeit rücksetzten
    if (count - time1 > sEnableTime)    //Prüfen ob die Zeit fürs Stellen abgelaufen ist
      digitalWrite(s1EPin, LOW);      //Servo1 GND abschalten
    if (count - time2 > sEnableTime)     //Prüfen ob die Zeit fürs Stellen abgelaufen ist
      digitalWrite(s2EPin, LOW);     //Servo2 GND abschalten
    if (count - time3 > sEnableTime)   //Prüfen ob die Zeit fürs Stellen abgelaufen ist
      digitalWrite(s3EPin, LOW);   //Servo3 GND abschalten
    if (count - time4 > sEnableTime)   //Prüfen ob die Zeit fürs Stellen abgelaufen ist
      digitalWrite(s4EPin, LOW);   //Servo4 GND abschalten

    //Andreas Kreuz Blinken
    if (s1 == 1) {      //Servo 1 auf Position A
      if (count % 1 == 0) {    //Blinkintervall  
        if (AndreasOn == true)
          AndreasOn = false;
        else AndreasOn = true;
        digitalWrite(A1Pin, AndreasOn);   //Ausgang 1 
        digitalWrite(A2Pin, !AndreasOn);   //Ausgang 2
        digitalWrite(A3Pin, AndreasOn);   //Ausgang 3 
        digitalWrite(A4Pin, !AndreasOn);   //Ausgang 4 
      }  
    }
    else {    //Blinken abschalten
      AndreasOn = false;
      digitalWrite(A1Pin, AndreasOn);   //Ausgang 1 
      digitalWrite(A2Pin, AndreasOn);   //Ausgang 2
      digitalWrite(A3Pin, AndreasOn);   //Ausgang 3 
      digitalWrite(A4Pin, AndreasOn);   //Ausgang 4 
    }
  }  
}

//****************************************************************  
//Interrupt funktion für alle Taster
void tasterdown() {
  //1. Tasterstatus einlesen:
  int UpbuttonState = digitalRead(UpTasterPin);
  int DownbuttonState = digitalRead(DownTasterPin);
  //2. Taster auswerten:
  //Prüfen ob Up-Taster gedrückt wurde?
  if (UpbuttonState == LOW && DownbuttonState == HIGH && ProgMode == true) {
    if (ProgWinkel < 120) {          //Prüfen ob max Winkel erreicht ist?
      ProgWinkel = ProgWinkel + 1;
      posServo(ProgServo, 1, ProgWinkel, 0);    //Servo positionieren
    }
    return;  //Routine Verlassen
  }
  //Prüfen ob Down-Taster gedrückt wurde?
  if (DownbuttonState == LOW && UpbuttonState == HIGH && ProgMode == true) {
    if (ProgWinkel > 1) {      //Prüfen on min Winkel erreicht ist?
      ProgWinkel = ProgWinkel - 1;
      posServo(ProgServo, 1, ProgWinkel, 0);    //Servo positionieren
    }  
    return;  //Routine Verlassen
  }
  
  //Prüfen ob Progtaster gedrück wurde und ProgMode aktiv ist?
  if (DownbuttonState == HIGH && UpbuttonState == HIGH && ProgMode == true) {
    if (ProgPos < 2) {   //Position != B ?
      ProgSave();              //Speichern der neuen Werte
      ProgPos = ProgPos + 1;    //wechseln zur nächsten Position
      switch (ProgServo) {
        case 1 : ProgWinkel = s1posB;  
                 break;
        case 2 : ProgWinkel = s2posB;  
                 break;
        case 3 : ProgWinkel = s3posB;  
                 break;
        case 4 : ProgWinkel = s4posB;  
                 break;         
      }
    }
    else {    //ProgPos == 2, zum nächsten Servo?
      if (ProgServo < 4) {    //Prüfen ob noch Servos vorhanden sind?
        ProgSave();              //Speichern der neuen Werte
        ProgServo = ProgServo + 1;      //Sprung zum nächsten Servo
        switch (ProgServo) {
          case 1 : ProgWinkel = s1posA;  
                   break;
          case 2 : ProgWinkel = s2posA;  
                   break;
          case 3 : ProgWinkel = s3posA;  
                   break;
          case 4 : ProgWinkel = s4posA;  
                   break;         
        }
      }
      else {  //kein Servo mehr vorhanden - ENDE
        ProgSave();              //Speichern der neuen Werte
        ProgMode = false;        //Werte alle zurücksetzten - Programmierung ENDE
        ProgPos = 0;    //Position A, B oder C des Servos der Programmiert wird.
        ProgServo = 0;  //Servo der Programmiert wird
        ProgWinkel = 0;   //Drehposition des Servos.
      }
    }
    posServo(ProgServo, 1, ProgWinkel, 0);    //Servo positionieren
    return;  //Routine Verlassen
  }
  //Prüfen ob Progtaster gedrück wurde und ProgMode inaktiv ist?
  if (DownbuttonState == HIGH && UpbuttonState == HIGH && ProgMode == false) {
    ProgMode = true;    //Programmiermodus EIN
    ProgPos = 1;   //Position A
    ProgServo = 1;    //Servo mit dem die Programmierung begonnen wird.
    ProgWinkel = s1posA;  //Position auf A stellen.
    posServo(ProgServo, 1, ProgWinkel, 0);    //Servo positionieren
  }
}

//****************************************************************  
//- Speichern der neuen Position in Variable und EEPROM
//- Speichern der letzten Zubehör-Adresse die gesendet wurde zu der Servoposition
void ProgSave() {
  switch (ProgServo) {
    case 1: switch(ProgPos) {      //Servo 1
              case 1: s1posA = ProgWinkel;      //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {    //Prüfen ob neue Adresse gesendet wurde?
                        s1AdrA = ProgAdr;    //Adresse übernehmen
                        s1PortA = ProgPort;    //Port übernehmen
                        s1AktivA = ProgAktiv;  //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
              case 2: s1posB = ProgWinkel;   //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {  //Prüfen ob neue Adresse gesendet wurde?
                        s1AdrB = ProgAdr;    //Adresse übernehmen
                        s1PortB = ProgPort;    //Port übernehmen
                        s1AktivB = ProgAktiv;    //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
            }
            break;
    case 2: switch(ProgPos) {      //Servo 2
              case 1: s2posA = ProgWinkel;      //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {    //Prüfen ob neue Adresse gesendet wurde?
                        s2AdrA = ProgAdr;    //Adresse übernehmen
                        s2PortA = ProgPort;    //Port übernehmen
                        s2AktivA = ProgAktiv;  //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
              case 2: s2posB = ProgWinkel;   //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {  //Prüfen ob neue Adresse gesendet wurde?
                        s2AdrB = ProgAdr;    //Adresse übernehmen
                        s2PortB = ProgPort;    //Port übernehmen
                        s2AktivB = ProgAktiv;    //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
            }
            break;        
    case 3: switch(ProgPos) {      //Servo 3
              case 1: s3posA = ProgWinkel;      //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {    //Prüfen ob neue Adresse gesendet wurde?
                        s3AdrA = ProgAdr;    //Adresse übernehmen
                        s3PortA = ProgPort;    //Port übernehmen
                        s3AktivA = ProgAktiv;  //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
              case 2: s3posB = ProgWinkel;   //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {  //Prüfen ob neue Adresse gesendet wurde?
                        s3AdrB = ProgAdr;    //Adresse übernehmen
                        s3PortB = ProgPort;    //Port übernehmen
                        s3AktivB = ProgAktiv;    //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
            }
            break;
    case 4: switch(ProgPos) {      //Servo 4
              case 1: s4posA = ProgWinkel;      //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {    //Prüfen ob neue Adresse gesendet wurde?
                        s4AdrA = ProgAdr;    //Adresse übernehmen
                        s4PortA = ProgPort;    //Port übernehmen
                        s4AktivA = ProgAktiv;  //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
              case 2: s4posB = ProgWinkel;   //neuen Winkel übernehmen
                      if (ProgAdrNeu == true) {  //Prüfen ob neue Adresse gesendet wurde?
                        s4AdrB = ProgAdr;    //Adresse übernehmen
                        s4PortB = ProgPort;    //Port übernehmen
                        s4AktivB = ProgAktiv;    //Zustand übernehmen
                        ProgAdrNeu = false;    //Adresse als abgespeichert markieren
                      }
                      break;
            }
            break;        
  }
}

//****************************************************************  
//Interrupt funktion auf dccPin
void dccdata() {
//  Timer1.start();  //Timer starten
  getdcc = true;
}
 
//****************************************************************   
//Timer nach Intterrupt auf DCC-Pin
void dcctime() {
//  Timer1.stop();  //Timer anhalten
  int State = digitalRead(dccPin);
  if (getdata == true) {
    datalength += 1;  //Bitweise im Array weitergehen.
    countbit += 1;    //Abzählen der Bytes, also jeweils 8 Bit
  }
  if (State == LOW) {      //1-Bit gelesen
    countone += 1;            //Zählen der 1-Bit für Präambel erforderlich
    if (getdata == true) {    //eingelesenen Bitwert im Array Speichern
      data[datalength] = 1;    //Speichert das ein 1-Bit gelesen wurde
    }
    if (countbit > 8) {        //End-Bit gelesen.
      countbit = 0;            
      getdata = false;        //Stop des Einlesen der Daten
      dccauswertung();         //eingelesene Daten auswerten.
    }
  }  //Ende 1-Bit
  else {                  //0-Bit gelesen
    if (getdata == true) {    //eingelesenen Bitwert im Array Speichern
      data[datalength] = 0;    //Speichert das ein 0-Bit gelesen wurde
    }
    if (countone > 10) {   //Präambel erkannt ab hier Daten lesen. (Es wurden mind. 10 HIGH erkannt)
      getdata = true;     //Einlesen der Bitwerte in Array aktivieren.
      datalength = -1;    //Position im Array an der die Bitwerte gespeichert werden.
      countbit = 0;       //beginne Bits zu zählen. Auswertung von 1 Byte Blöcken ermöglichen nach 8 Bit.
    }
    if (countbit > 8) {    //Null-Bit gelesen. (Immer nach 1 Byte)
      countbit = 0;
    }
    countone = 0;    //Es wurde ein 0-Bit gelesen, lösche Anzahl gelesener 1-Bit
  }  //Ende 0-Bit
}

//****************************************************************  
//XOR Berechnung 
int xorcheck(int x, int y) {
  /* Wertetabelle XOR:
    x  y  |  z
   ------------
    0  0  |  0
    0  1  |  1
    1  0  |  1
    1  1  |  0
  */
  if (x == 0 && y == 0) return 0;
  if (x == 0 && y == 1) return 1;
  if (x == 1 && y == 0) return 1;
  if (x == 1 && y == 1) return 0;
    
  return 99;  //keine Übereinstimmung - Fehler Ausgeben
} //Ende XOR

//****************************************************************  
//Auswerten der eingelesenen Bitwerte.
void dccauswertung() {
  /*
    Beschreibung der Einträge im data Array:
      Bit 0-7   : 1. Byte nach der Präambel (Adresse)
      Bit 8     : Null-Bit
      Bit 9-16  : 2. Byte (long Adresse oder Daten)
      Bit 17    : Null-Bit
      Bit 18-25 : 3. Byte (bei datalength == 26 => CRC)
      Bit 26    : Null-Bit (bei datalength == 26 => End-Bit)
      Bit 27-34 : 4. Byte (wenn vorhanden, long Adr. oder long Data)
      Bit 35    : End-Bit (wenn vorhanden, long Adr. oder long Data)
      Bit 36-43 : 5. Byte (wenn vorhanden, long Adr. und long Data)
      Bit 44    : End-Bit (wenn vorhanden, long Adr. und long Data)
  */
  
  //1.1 Prüfen der CRC Summe
  boolean check = false;    //CRC Summe nicht erfolgreich - Befehl verwerfen
  //1.2 Prüfen für 3 Byte Befehl
  if (datalength == 26 && data[8] == 0 && data[17] == 0 && data[26] == 1) {  //Prüfen für max. 3 Byte
    int bit0 = xorcheck(data[0], data[9]);    //Bestimme XOR von 1. Byte und 2. Byte
    int bit1 = xorcheck(data[1], data[10]); 
    int bit2 = xorcheck(data[2], data[11]); 
    int bit3 = xorcheck(data[3], data[12]); 
    int bit4 = xorcheck(data[4], data[13]); 
    int bit5 = xorcheck(data[5], data[14]); 
    int bit6 = xorcheck(data[6], data[15]); 
    int bit7 = xorcheck(data[7], data[16]); 
    //Prüfen von XOR und 3. Byte
    if (bit0 == data[18] && bit1 == data[19] && bit2 == data[20] && bit3 == data[21] && bit4 == data[22] && bit5 == data[23] && bit6 == data[24] && bit7 == data[25]) { 
      check = true;    //Befehl annehmen
    }
  }
  //1.3 Prüfen für 4 Byte oder 5 Byte Befehl
  if ((datalength == 35 && data[8] == 0 && data[17] == 0 && data[26] == 0 && data[35] == 1) || (data[0] == 1 && data[1] == 1 && datalength == 44 && data[17] == 0 && data[26] == 0 && data[35] == 0 && data[44] == 1)) {  
    //Prüfsumme bestimmen für das 1. Byte und 2. Byte mit XOR
    int bit0 = xorcheck(data[0], data[9]);    //1. Byte Bit 7 mit 2. Byte Bit 7
    int bit1 = xorcheck(data[1], data[10]);   //1. Byte Bit 6 mit 2. Byte Bit 6
    int bit2 = xorcheck(data[2], data[11]);   //1. Byte Bit 5 mit 2. Byte Bit 5
    int bit3 = xorcheck(data[3], data[12]);   //1. Byte Bit 4 mit 2. Byte Bit 4
    int bit4 = xorcheck(data[4], data[13]);   //1. Byte Bit 3 mit 2. Byte Bit 3
    int bit5 = xorcheck(data[5], data[14]);   //1. Byte Bit 2 mit 2. Byte Bit 2
    int bit6 = xorcheck(data[6], data[15]);   //1. Byte Bit 1 mit 2. Byte Bit 1
    int bit7 = xorcheck(data[7], data[16]);   //1. Byte Bit 0 mit 2. Byte Bit 0
    bit0 = xorcheck(bit0, data[18]);       //Bestimme XOR Prüfsumme von (1.Byte und 2. Byte) mit 3. Byte
    bit1 = xorcheck(bit1, data[19]); 
    bit2 = xorcheck(bit2, data[20]); 
    bit3 = xorcheck(bit3, data[21]); 
    bit4 = xorcheck(bit4, data[22]); 
    bit5 = xorcheck(bit5, data[23]); 
    bit6 = xorcheck(bit6, data[24]); 
    bit7 = xorcheck(bit7, data[25]); 
    //Prüfen von XOR und 4. Byte oder 5. Byte
    int plus = 0;      //Bei einem 4 Byte Befehl kein Verschub
    if (data[0] == 1 && data[1] == 1) 
      int plus = 9;    //Verschub von 9 Bit bei einem 5 Byte Befehl - das 4. Byte hat keine Auswirkung auf die CRC Summe!
    if (bit0 == data[27+plus] && bit1 == data[28+plus] && bit2 == data[29+plus] && bit3 == data[30+plus] && bit4 == data[31+plus] && bit5 == data[32+plus] && bit6 == data[33+plus] && bit7 == data[34+plus]) {    //bestimmen von XOR
      check = true;    //Befehl annehmen
    }
  }
  //1.4 CRC Summe war falsch, Daten nicht verarbeiten!
  if (check == false) { //Prüfen ob CRC nicht erfolgreich war?
    return;  //Abbruch der Auswertung, eingelesene Bit sind fehlerhaft
  }  
  //Ende CRC Check
  
/*  //2.1 Bestimmen kurzen Adresse (bis 127)
  int verschub = 0;    //Bits die durch eine lange Adresse verschoben werden.
  int dccAdr = 0;
  if (data[0] == 0) {
    //Aus 1. Byte von Bit 6 bis Bit 0
    bitWrite(dccAdr,6,data[1]);
    bitWrite(dccAdr,5,data[2]);  //32
    bitWrite(dccAdr,4,data[3]);  //16
    bitWrite(dccAdr,3,data[4]);  //8
    bitWrite(dccAdr,2,data[5]);  //4
    bitWrite(dccAdr,1,data[6]);  //2
    bitWrite(dccAdr,0,data[7]);  //1
  } //Ende kurze Adresse
  //2.2 Bestimmen der langen Adresse mit 14 Bits also 16384
  if (data[0] == 1 && data[1] == 1) {
    verschub = 9;    //alles wird um 9 Bits durch die lange Adresse verschoben!
    //Aus 1. Byte von Bit 5 bis Bit 0
    bitWrite(dccAdr,13,data[2]);  //Bit 13 * 8192
    bitWrite(dccAdr,12,data[3]);  //Bit 12 * 4096
    bitWrite(dccAdr,11,data[4]);  //Bit 11 * 2048
    bitWrite(dccAdr,10,data[5]);  //Bit 10 * 1024
    bitWrite(dccAdr,9,data[6]);  //Bit 9 * 512
    bitWrite(dccAdr,8,data[7]);  //Bit 8 * 256
    //und 2. Byte
    bitWrite(dccAdr,7,data[9]);  //Bit 7 * 128
    bitWrite(dccAdr,6,data[10]);  //Bit 6 * 64
    bitWrite(dccAdr,5,data[11]);  //Bit 5 * 32
    bitWrite(dccAdr,4,data[12]);  //Bit 4 * 16
    bitWrite(dccAdr,3,data[13]);  //Bit 3 * 8
    bitWrite(dccAdr,2,data[14]);  //Bit 2 * 4
    bitWrite(dccAdr,1,data[15]);  //Bit 1 * 2
    bitWrite(dccAdr,0,data[16]);  //Bit 0 (1)
  }  //Ende lange Adresse
  */
  
  //2.3 Bestimmen der Weichenadresse
  int dccZAdr = 0;
  int dccZPort = 0;
  if (data[0] == 1 && data[1] == 0 && data[9] == 1) {
    //1. Adressteil für je 64 Adressen
    bitWrite(dccZAdr,5,data[2]);    //Bit 5 * 32
    bitWrite(dccZAdr,4,data[3]);    //Bit 4 * 16
    bitWrite(dccZAdr,3,data[4]);    //Bit 3 * 8
    bitWrite(dccZAdr,2,data[5]);    //Bit 2 * 4
    bitWrite(dccZAdr,1,data[6]);    //Bit 1 * 2
    bitWrite(dccZAdr,0,data[7]);    //Bit 0 * 1
    //2. Adressteil für bis zu 64 * 8 Adressen
    if (data[12] == 0) dccZAdr = dccZAdr + 64;
    if (data[11] == 0) dccZAdr = dccZAdr + 128;
    if (data[10] == 0) dccZAdr = dccZAdr + 256;
    //Port bestimmen, bis zu 4 möglich
    dccZPort += (data[14] * 2);
    dccZPort += data[15];
    dccZPort = dccZPort + 1;  //Ports von 1 - 4 möglich
  }
  
  //Leerlaufpackete hier filtern!
  if (!(data[0] == 1 && data[1] == 1 && data[2] == 1 && data[3] == 1 && data[4] == 1 && data[5] == 1 && data[6] == 1 && data[7] == 1)) { //(dccAdr == decAdr) {
    //komplette 1. Byte 1 => Leerlaufpacket (wird nicht angezeigt!)
    //_______________________________________________________________________________
    if (ProgMode == true) { //Prüfen ob im ProgMode
      ProgAdrNeu = true;
      ProgAdr = dccZAdr;
      ProgPort = dccZPort;
      ProgAktiv = data[16];
      return;  //Nicht die Servos setzten hier ENDE
    }
    
    //Servo1:________________________________________________________________________
    //Prüfen ob Adresse und Port = A von Servo1: 
    if (dccZAdr == s1AdrA && dccZPort == s1PortA && data[16] == s1AktivA && s1 != 1) {  
      s1 = 1;  //PositionA
      posServo(1, s1, s1posA, s1posB);    //Servo positionieren
    }
    //Prüfen ob Adresse und Port = B von Servo1: 
    if (dccZAdr == s1AdrB && dccZPort == s1PortB && data[16] == s1AktivB && s1 != 2) {     
      s1 = 2;   //PositionB
      posServo(1, s1, s1posA, s1posB);    //Servo positionieren
    }
    //Servo2:________________________________________________________________________
     //Prüfen ob Adresse und Port = A von Servo2:
    if (dccZAdr == s2AdrA && dccZPort == s2PortA && data[16] == s2AktivA && s2 != 1) { 
      s2 = 1;   //PositionA
      posServo(2, s2, s2posA, s2posB);    //Servo positionieren
    }
     //Prüfen ob Adresse und Port = B von Servo2:
    if (dccZAdr == s2AdrB && dccZPort == s2PortB && data[16] == s2AktivB && s2 != 2) {    
      s2 = 2;    //PositionB
      posServo(2, s2, s2posA, s2posB);    //Servo positionieren
    }
    //Servo3:________________________________________________________________________
    //Prüfen ob Adresse und Port = A von Servo3:
    if (dccZAdr == s3AdrA && dccZPort == s3PortA && data[16] == s3AktivA && s3 != 1) {  
      s3 = 1;  //PositionA
      posServo(3, s3, s3posA, s3posB);    //Servo positionieren
    }
    //Prüfen ob Adresse und Port = B von Servo3:    
    if (dccZAdr == s3AdrB && dccZPort == s3PortB && data[16] == s3AktivB && s3 != 2) { 
      s3 = 2;   //PositionB
      posServo(3, s3, s3posA, s3posB);    //Servo positionieren
    }
    //Servo4:________________________________________________________________________
     //Prüfen ob Adresse und Port = A von Servo4:
    if (dccZAdr == s4AdrA && dccZPort == s4PortA && data[16] == s4AktivA && s4 != 1) { 
      s4 = 1;    //PositionA
      posServo(4, s4, s4posA, s4posB);    //Servo positionieren
    }
     //Prüfen ob Adresse und Port = B von Servo4:
    if (dccZAdr == s4AdrB && dccZPort == s4PortB && data[16] == s4AktivB && s4 != 2) {  
      s4 = 2;   //PositionB
      posServo(4, s4, s4posA, s4posB);    //Servo positionieren
    }
  }
}

//****************************************************************  
//Positionieren eines Servos und aktivieren!
void posServo(int servo, int npos, int sposA, int sposB) {
  int pos = 90;     //default Position
  if (npos == 1)           //Position abfragen
    pos = sposA;            //setzten auf PositionA
  else pos = sposB;         //setzten auf PositionB
  switch (servo) {
    case 1: servo1.write(pos);       // tell servo to go to position in variable 'pos' 
            digitalWrite(s1EPin, HIGH);    //GND aktivieren  
            time1 = count;     //Zeit merken für aktiven GND 
            break;  
    case 2: servo2.write(pos);       // tell servo to go to position in variable 'pos'  
            digitalWrite(s2EPin, HIGH);    //GND aktivieren  
            time2 = count;     //Zeit merken für aktiven GND 
            break;  
    case 3: servo3.write(pos);       // tell servo to go to position in variable 'pos'  
            digitalWrite(s3EPin, HIGH);    //GND aktivieren  
            time3 = count;     //Zeit merken für aktiven GND 
            break;
    case 4: servo4.write(pos);       // tell servo to go to position in variable 'pos'  
            digitalWrite(s4EPin, HIGH);    //GND aktivieren  
            time4 = count;     //Zeit merken für aktiven GND 
            break;        
    default: Serial.print("Servo not found! -> ");
  }
  Serial.print("Servo");    
  Serial.print(servo);
  Serial.print(": ");
  Serial.println(pos);  
}
