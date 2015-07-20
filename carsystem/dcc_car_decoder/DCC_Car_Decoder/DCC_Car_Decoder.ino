/*
  DCC-Fahrzeugdecoder
  - decodert DCC-Signale 
  - prüft empfangende Befehle mit CRC
  - wertet die Befehle anhand der gesendeten Adresse aus
  - steuert Motor und Lichteffekte im Modellfahrzeug
  - Registerwerte des Dekoder können über CV geändert werden
  
  Serial:
  - RX-Signal PIN 0
  - TX-Signal PIN 1
  
  Inputs:
  - DCC Signal PIN 2 (Interrupt)
  - Lichtsensor (F3) PIN 18/A4
  - Akkuspannung PIN 19/A5
  
  Outputs:
  - Motor PIN 5
  - Abblendlicht (F0) PIN 6
  - Blinken rechts (F1) PIN 9
  - Blinken links (F2) PIN 10
  - Rücklicht mit Bremslicht PIN 11
  - Rundumlicht PIN 3
  - IR-Empfänger Power PIN 4
  - weitere Ausgänge sind frei Programmierbar und für Einsatzseznarien anwendbar:
    * AUX1/Sirene PIN 12
    * AUX2 PIN 13
    * AUX3 PIN 14/A0
    * AUX4 PIN 15/A1
    * AUX5 PIN 16/A2
    * AUX6 PIN 17/A3
  - PIN 7 & 8 sind FREI, da sie auf dem Platinenlayout aus Platzgründen nicht verdrahtet werden konnten.
  
  Aktuelle Software Version "1.52"  */
const int decversion1 = 1;   //mögliche Werte von 1 bis 4  
const int decversion2 = 5;   //mögliche Werte von 0 bis 8
const int decversion3 = 3;   //mögliche Werte von 0 bis 8
/*Stand: 16.12.2011
  
  Versionshirachie:
  V1.53  11.08.13  Problem mit Motoransteuerung im Sleep-Mode behoben. (Pin wird auf LOW gehalten!)
  V1.52  07.08.12  Schreibfehler (Bitswap) bei CV-Programmierung korregiert
  V1.51  31.01.11  AutoSerial ON beim Starten möglich, keine Zentrale für CV-Programmierung notwendig;
                   Fehler bei der Funktionszuweisung Rundumlicht behoben;
                   Motorzwangsabschaltung unter 3,3 Volt
  V1.50  16.12.11  Verbesserung des Register Zugriff und Programmierung;
                   Optimierung bei der Dekodierung von DCC-Befehlen;
                   Problem mit Rücklichtabschaltung bei DCC-Datenabschaltung (PWM-Timer) behoben;
                   CV-Data.h Datei mit allen Registerzuweisungen und default Werten hinzugefügt
  V1.48  08.12.11  Blinkerausgang invertierbar und Akkuwarnung abschaltbar über CV
  V1.47  06.12.11  Akkurichtwert für Geschwindigkeiten und Programmierbestätigung
  V1.46  24.11.11  Zusätzliche Abhängigkeit der Sirene von einer weiteren Funktion.
  V1.45  22.11.11  Serial LDR auslesen angepasst. LDR muss am Dekoder und an AREF hängen!
  V1.44  17.11.11  Programmierbare Ports CV Konfigurationsvariablen zugewiesen.
  V1.43  Serielle Kommunikation über CV12 aktivierbar, sonst inaktiv. (GET und SET von CV)
  V1.42  Sirene implementiert und Fehler mit time Berechnung behoben in LDR, Notstop & Sleep
  V1.41  Test der Energiesparfunktion
  V1.40  Automatische Energiesparfunktion bei abgeschalteten Daten (99,5% Ersparnis mit lowsleep).
  V1.32  Rundumlicht angepasst
  V1.31  angepasster Serieller Akkutest mit Refernzwert für Warnblinken
  V1.30  serielle Kommunikation aktiviert, auslesen von CV1-8 Werten möglich
  V1.20  programmierbare Einsatzausgänge und Rundumlicht
  V1.10  Decoder CV Programmierung implementiert
  V1.00  erste Entwickerversion, Decodierung von DCC Signalen
  
  Autor: Philipp Gahtow
*/

// include the library code:
#include <TimerOne.h>    //Timer für die DCC Signalerkennung
#include <EEPROM.h>      //Speicher für CV Werte
//#include <SoftPWM.h>     //für PWM auf allen Ausgängen
#include <avr/sleep.h>   //Energiesparen durch Sleep
#include <avr/wdt.h>     //Watchdog Timer für Sleep wakeup

#include "CV_Data.h"     //Enthält Zuweisungen der Register zum Speicherinhalt

//Konfiguration für Power Down Sleep-Mode
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif 

//Pinconfiguration
#define lichtPin 6    //Abblendlicht   (Ausgange mit PWM)
#define F1Pin 10      //blinken links   (Ausgange mit PWM)
#define F2Pin 11      //blinken rechts  (Ausgang mit PWM)
#define rueckPin 9   //Rücklichtlicht + Bremslicht  (Ausgang mit PWM)
#define SpeedPin 5   //Motoranschluss  (Ausgang mit PWM)
#define IRPowerPin 4   //Versorgung für IR-Empfänger

  //Eingänge:
#define dccPin 2      //nur auf Pin 2 geht die attachInterrupt Funktion!
#define ldrPin 18 //A4;     //18 - Helligkeitssensor Eingang
#define akkuPin 19 //A5;    //19 - Akkuspannung wird geprüft, zu niedrig => Warnblinken

//Ausgänge die Programmierbar sind:
#define rundlichtpin 3  //Rundumlicht, hier PWM notwendig
#define aux1 12      //Port1, mit Sirene (ohne Dreifachblitz)
#define aux2 13      //Port2
#define aux3 14 //A0;     //Port3
#define aux4 15 //A1;     //Port4
#define aux5 16 //A2;     //Port5
#define aux6 17 //A3;     //Port6

//Variablen
  //Decoderadresse
int decAdr = CVdefaultdecadr;    //Adresse auf welcher der Fahrzeugdecoder reagieren soll.

  //Status Programmierbarer Ausgänge
int auxtime = 0;          //Übergabeparameter für die Funktion setPort
int rlichtseq = 0;        //Sequenzenzähler für Rundumlicht
int rlichttime = 0;       //Zeiten für Sequenzen
int aux1seq = 0;            //Sequenzenzähler für Einfach, Doppel oder Dreifachblinken
int aux1time = 0;           //Zeiten zwisschen den Blinksequenzen
int aux2seq = 0;            //Sequenzenzähler für Einfach, Doppel oder Dreifachblinken
int aux2time = 0;           //Zeiten zwisschen den Blinksequenzen
int aux3seq = 0;            //Sequenzenzähler für Einfach, Doppel oder Dreifachblinken
int aux3time = 0;           //Zeiten zwisschen den Blinksequenzen
int aux4seq = 0;            //Sequenzenzähler für Einfach, Doppel oder Dreifachblinken
int aux4time = 0;           //Zeiten zwisschen den Blinksequenzen
int aux5seq = 0;            //Sequenzenzähler für Einfach, Doppel oder Dreifachblinken
int aux5time = 0;           //Zeiten zwisschen den Blinksequenzen
int aux6seq = 0;            //Sequenzenzähler für Einfach, Doppel oder Dreifachblinken
int aux6time = 0;           //Zeiten zwisschen den Blinksequenzen

//Rundumlicht
int rlichtfunktion = CVdefaultrlichtfunktion;     //Funktionstaste (bitweise F11 bis F4)
int rlichtZeit = CVdefaultrlichtZeit;        //Geschwindigkeit des Rundumlicht.


#define AUXPINS_SIZE 6
#define CVSIZE 29
aux_pin_struct AUXPINS[AUXPINS_SIZE];
boolean f[CVSIZE];

//Port1:
int aux1funktion = CVdefaultaux1funktion;       //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
int sirenefunktion = CVdefaultsirenefunktion;         //Abhängigkeit von einer 2. Funktion, kann zB. nur mit Rundlicht betrieben werden.
int aux1art = 0;            //Art des Blinken/Leuchten (unten erklärt) (0 - 3), hier mit Sirene.
int sireneArt = CVdefaultsireneArt;          //Art des Klang der Sirene (2 Bit)
int aux1blitz = CVdefaultaux1blitz;          //Länge des Blitz ( *10 ms)
int aux1p1 = CVdefaultaux1p1;             //Pause zwischen Blitzen  ( *10 ms)
int aux1p2 = CVdefaultaux1p2;            //Pause zwischen Zweifachblitz  ( *10 ms)
//Port2:
int aux2funktion = CVdefaultaux2funktion;       //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
int aux2art = 2;            //Art des Blinken/Leuchten (unten erklärt) (0 - 3)
int aux2blitz = CVdefaultaux2blitz;          //Länge des Blitz ( *10 ms)
int aux2p1 = CVdefaultaux2p1;             //Pause zwischen Blitzen  ( *10 ms)
int aux2p2 = CVdefaultaux2p2;            //Pause zwischen Zweifachblitz  ( *10 ms)
//Port3:
int aux3funktion = CVdefaultaux3funktion;       //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
int aux3art = 3;            //Art des Blinken/Leuchten (unten erklärt) (0 - 3)
int aux3blitz = CVdefaultaux3blitz;          //Länge des Blitz ( *10 ms)
int aux3p1 = CVdefaultaux3p1;             //Pause zwischen Blitzen  ( *10 ms)
int aux3p2 = CVdefaultaux3p2;            //Pause zwischen Zweifachblitz  ( *10 ms)
//Port4:
int aux4funktion = CVdefaultaux4funktion;       //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
int aux4art = 1;            //Art des Blinken/Leuchten (unten erklärt) (0 - 3)
int aux4blitz = CVdefaultaux4blitz;          //Länge des Blitz ( *10 ms)
int aux4p1 = CVdefaultaux4p1;             //Pause zwischen Blitzen  ( *10 ms)
int aux4p2 = CVdefaultaux4p2;            //Pause zwischen Zweifachblitz  ( *10 ms)
//Port5:
int aux5funktion = CVdefaultaux5funktion;       //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
int aux5art = 3;            //Art des Blinken/Leuchten (unten erklärt) (0 - 3)
int aux5blitz = CVdefaultaux5blitz;        //Länge des Blitz ( *10 ms)
int aux5p1 = CVdefaultaux5p1;           //Pause zwischen Blitzen  ( *10 ms)
int aux5p2 = CVdefaultaux5p2;             //Pause zwischen Zweifachblitz  ( *10 ms)
//Port6:
int aux6funktion = CVdefaultaux6funktion;       //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
int aux6art = 1;            //Art des Blinken/Leuchten (unten erklärt) (0 - 3)
int aux6blitz = CVdefaultaux6blitz;          //Länge des Blitz ( *10 ms)
int aux6p1 = CVdefaultaux6p1;             //Pause zwischen Blitzen  ( *10 ms)
int aux6p2 = CVdefaultaux6p2;            //Pause zwischen Zweifachblitz  ( *10 ms)

/*
Art:
-1 = Sirene*
0 = kein Blinken
1 = einfaches Blinken
2 = Doppelblitz
3 = Dreifachblitz

zu * beachte nur an Port 1 -> (art -1), kein Dreifachblitz.
*/

  //Status der Ausgänge
int flash = 0;               //globale Blinkvariable für ein, oder mehrere Ausgänge (+1 alle 10ms)
int ldrdelay = 0;            //Berechnung der Verzögerung für an/aus mit LDR
int bremsdelay = 0;          //Berechnung der Verzögerung für die Bremslichter

  //Abblendlicht und Bremslicht
int lichthelligkeit = CVdefaultlichthelligkeit;  //Helligkeit des Abblendlichts
boolean licht = false;      //Abblendlicht EIN/AUS für Bremslichtsteuerung mit PWM
boolean bremslicht = false; //Bremslicht EIN/AUS für Bremslicht mit integriertem Abblendlicht (PWM)

  //Blinker  & Akkuwarnung
int binkhelligkeit = CVdefaultbinkhelligkeit;    //Helligkeit des Blinklichts  
int blinktakt = CVdefaultblinktakt;          //Wert * 10 = ms Blinkzeit für jeweils EIN oder AUS
int blinkAkkuleer = CVdefaultblinkAkkuleer;      //Akkuspannung unter welcher der Decoder die Warnblinkanlage einschaltet. (Wert * 4; bei 1024 = 5 Volt)
boolean akkuwarnunginaktiv = false; //Warnblinker bei unterschreiten der eingestellen Warnspannung
boolean binkzustand = false;  //wenn Blinker aktiv, enthält aktuellen Zustand der Blinkleuchten
boolean blinkerinvertiert = false;  //Invertierung der Blinkeransteuerung

 //Rücklicht
int rueckhelligkeit = CVdefaultrueckhelligkeit;    //Rücklichter leuchten gedimmt mit dieser Stärke wenn sie EIN sind
int bremshelligkeit = CVdefaultbremshelligkeit;   //Helligkeit des Bremslichtes
int bremslichtschwelle = CVdefaultbremslichtschwelle;     //Fahrstufe ab der das Bremslicht sofort EIN geht beim Reduzieren
int bremslichtschnell = CVdefaultbremslichtschnell;       //Bei Reduzierung der Fahrstufe von mehr als (1/bremslichtschnell) = EIN
int bremsnachleuchtstand = CVdefaultbremsnachleuchtstand;  //Zeit die das Bremslicht im Stand anbleibt (Wert * 10 = 2550ms)
int bremsnachleuchtfahrt = CVdefaultbremsnachleuchtfahrt;   //Zeit die das Bremslicht während der Fahrt anbleibt (Wert * 10 = 800ms)

  //LDR schwelle für Abblendlicht  (0 = dunkel bis 1024 = hell)
int ldrlichtein = CVdefaultldrlichtein;      //unter diesem Wert geht das Licht EIN (normal: 470/4)
int ldrdelayein = CVdefaultldrdelayein;      //Wartezeit unter dem Wert bis Licht EIN geht (Wert * 10 = ms)
int ldrlichtaus = CVdefaultldrlichtaus;      //über diesem Wert geht das Licht AUS  (normal: 630/4)
int ldrdelayaus = CVdefaultldrdelayaus;      //Wartezeit über dem Wert bis Licht AUS geht  (Wert * 10 = ms)

  //Automatische STOP Funktion - wenn DCC Daten erkannt wurden, kann das Auto weiterfahren.
int dateninZeit = 0;        //Der Zeitpunkt an dem Daten erkannt wurden.
boolean makeSleep = false;  //Variable die anzeigt ob die CPU im Ruhemodus ist
int dateninPause = CVdefaultdateninPause;      //Zeit die das DCC Signal nicht Empfangen wird bis gestoppt wird. (dateninPause+1 * 10 * 2 = 1001 ms)
int datenSleepPause = CVdefaultdatenSleepPause;   //Sleep spart 90% Akku, wenn keine DCC Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
int SleepLowTime = CVdefaultSleepLowTime;      //Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)

  //Motoransteuerung - Fahrdynamik
int startspannung = CVdefaultstartspannung;      //Spannung, die bei Fahrstufe 1 am Motor anliegt. (0 = 0 Volt und 255 = max. Spannung)
int beschleunigungsrate = CVdefaultbeschleunigungsrate;    //Zeit bis zum Hochschalten zur nächst höheren Fahrstufe  (CV3 * 0,9 sec / Anz. Fahrstufen)
int bremsrate = CVdefaultbremsrate;          //Zeit bis zum Herunterschalten zur nächst niedriegern Fahrstufe (CV3 * 0,9 sec / Anz. Fahrstufen)
int maximalspannung = CVdefaultmaxspannung;    //Spannung die bei höchster Fahrstufe anliegt.  (0 = min. Spannung und 255 = max. Volt)
int speedakkuwert = CVdefaultspeedakkuwert;    //Akkuwert nach dem die CV-Werte 2-5 ausgelegt sind (Akkuspannung = Wert / 50,75)

  //Zustände der Funktionen
boolean f0 = false;        //Status vom Ausgang F0

boolean f1 = false;        //Status vom Ausgang F1
boolean f2 = false;        //Status vom Ausgang F2
boolean f3 = false;        //Status vom Ausgang F3
boolean f4 = false;        //Status vom Ausgang F4

boolean f5 = false;        //Status vom Ausgang F5
boolean f6 = false;        //Status vom Ausgang F6
boolean f7 = false;        //Status vom Ausgang F7
boolean f8 = false;        //Status vom Ausgang F8

boolean f9 = false;         //Status vom Ausgang F9
boolean f10 = false;        //Status vom Ausgang F10
boolean f11 = false;        //Status vom Ausgang F11
boolean f12 = false;        //Status vom Ausgang F12

boolean f13 = false;        //Status vom Ausgang F13
boolean f14 = false;        //Status vom Ausgang F14
boolean f15 = false;        //Status vom Ausgang F15
boolean f16 = false;        //Status vom Ausgang F16
boolean f17 = false;        //Status vom Ausgang F17
boolean f18 = false;        //Status vom Ausgang F18
boolean f19 = false;        //Status vom Ausgang F19
boolean f20 = false;        //Status vom Ausgang F20

boolean f21 = false;        //Status vom Ausgang F21
boolean f22 = false;        //Status vom Ausgang F22
boolean f23 = false;        //Status vom Ausgang F23
boolean f24 = false;        //Status vom Ausgang F24
boolean f25 = false;        //Status vom Ausgang F25
boolean f26 = false;        //Status vom Ausgang F26
boolean f27 = false;        //Status vom Ausgang F27
boolean f28 = false;        //Status vom Ausgang F28

int sollspeed = 0;          //Geschwindigkeit des Motors
int istspeed = 0;           //aktuelle fahrende Geschwindigkeit

//Variablen für DCC Erkennung
int countone = 0;    //Zähle die high so das man eine Präambel erkennen kann (min. 10 high)
boolean getdata = false;     //Es werden DCC Daten gelesen und im Array data abgespeichert
int data[45];                //eingelesene Bit-Werte nach einer Präambel
int datalength = 0;  //Position in data wo gerade der nächste Wert hinkommt
int countbit = 0;    //Zähle Bits, nach 8 Bit wurde ein Byte gelesen und setzte dann die Variable auf 0
boolean getdcc = false;      //DCC Daten vorhanden und werden verarbeitet

//Serielle Kommunikation for Debug CV
boolean serialEnable = false;    //ob die serial Kommunikation aktiv ist.
String sbuffer;        //Eingabespeicher für Zeichen
int szahl = 0;         //Eingabespeicher für Zahlen
float akkuSpannung = 0.0;     //die Akkuspannung, (akkuPin / 203.0) = Volt, ergibt sich aus Spannungsteiler (Plus - 100k -|- 27k - Minus);
int sSetAdr = 0;  //ist > 0 wenn ein Wert über Serial programmiert wird.
int autoSerial = 256;     //Zeit * 10 = ms, die beim starten Serial aktiv ist.


//****************************************************************  
//System Start routine 
void setup()
{
  //1. Setzten der jeweiligen Ausgänge
  setPinMode();    //setzen der Modi für die Pins und IR Einschalten
  
  //2. Laden der nutzerdefinierte Einstellungen:
  getRegister();  //Einlesen der CV Konfigurationswerte
  analogReference(INTERNAL);    //Refernzspannung auf 1,1 Volt stellen

  //3. Timer1 aktivieren zum bestimmen der Impulslängen des DCC Signals
  Timer1.initialize(63);   // set a timer of length 65 microseconds
  Timer1.attachInterrupt(dcctime); // attach the service routine here
//  Timer1.stop();              //Timer anhalten
  
  //4. Serielle Kommunikation aktivieren für Debug
  Serial.begin(9600);	// opens serial port, sets data rate to 9600 bps
  Serial.print("Serial OFF, Dekoderversion ist: ");
  writeSerialDecVersion();
    
  //5. CPU Sleep Mode Konfiguration 
  // SM2 SM1 SM0 Sleep Mode
  // 0    0  0 Idle
  // 0    0  1 ADC Noise Reduction
  // 0    1  0 Power-down
  // 0    1  1 Power-save
  // 1    0  0 Reserved
  // 1    0  1 Reserved
  // 1    1  0 Standby(1)

  cbi( SMCR,SE );      // sleep enable, power down mode
  cbi( SMCR,SM0 );     // power down mode
  sbi( SMCR,SM1 );     // power down mode
  cbi( SMCR,SM2 );     // power down mode
  
  startVariables();
  
}

//****************************************************************  
void startVariables(){
    
    //default ones
    int i=0;
    for (i=0;i<AUXPINS_SIZE;i++){
      AUXPINS[i].time = 0;
      AUXPINS[i].seq = 0;      
      AUXPINS[i].port = 12+i; //ports 12 to 17
    }
  
    AUXPINS[0].function = CVdefaultaux1funktion;
    AUXPINS[0].blitz = CVdefaultaux1blitz;   
    AUXPINS[0].p1 = CVdefaultaux1p1;
    AUXPINS[0].p2 = CVdefaultaux1p2; 
    AUXPINS[0].art = 0; 
  
    AUXPINS[1].function = CVdefaultaux2funktion;
    AUXPINS[1].blitz = CVdefaultaux2blitz;   
    AUXPINS[1].p1 = CVdefaultaux2p1;
    AUXPINS[1].p2 = CVdefaultaux2p2; 
    AUXPINS[1].art = 2;

    AUXPINS[2].function = CVdefaultaux3funktion;
    AUXPINS[2].blitz = CVdefaultaux3blitz;   
    AUXPINS[2].p1 = CVdefaultaux3p1;
    AUXPINS[2].p2 = CVdefaultaux3p2; 
    AUXPINS[2].art = 3;
  
    AUXPINS[3].function = CVdefaultaux4funktion;
    AUXPINS[3].blitz = CVdefaultaux4blitz;   
    AUXPINS[3].p1 = CVdefaultaux4p1;
    AUXPINS[3].p2 = CVdefaultaux4p2;  
    AUXPINS[3].art = 1;
  
    AUXPINS[4].function = CVdefaultaux5funktion;
    AUXPINS[4].blitz = CVdefaultaux5blitz;   
    AUXPINS[4].p1 = CVdefaultaux5p1;
    AUXPINS[4].p2 = CVdefaultaux5p2; 
    AUXPINS[4].art = 3;

    AUXPINS[5].function = CVdefaultaux6funktion;
    AUXPINS[5].blitz = CVdefaultaux6blitz;   
    AUXPINS[5].p1 = CVdefaultaux6p1;
    AUXPINS[5].p2 = CVdefaultaux6p2;     
    AUXPINS[5].art = 1;
    
    for (i=0;i<CVSIZE;i++){
       f[i]=false;
    }
    
}  


//****************************************************************  
//Programmablauf 
void loop()
{ 
  if (serialEnable == false) {
    setausgaenge();   //Steuerung der Ausgänge des kompletten Fahrzeugs und Sleepfunktion
    if (autoSerial > 0) {    //Prüfen ob beim Start Serialdaten verfügbar sind?
      autoSerial--;    
      if (Serial.available() > 0) {    //Serielle Daten verfügbar?
        for (int i = 0; Serial.available() > 0; i++)  //Daten verwerfen
          Serial.read();    //Buffer leeren
        setSerial(true);   //alle normalen Funktionen des Dekoder werden abgeschaltet!;
        autoSerial = 0;    //Auto Serial ON deaktivieren
      }
    }
    delay(10);        //Intervall zum Ansteuern der Ausgänge (alle 10ms)
  }  
  else checkSerial();    //Prüfen ob Serielle Verbindung Aktiv ist?
}

//****************************************************************  
//Interrupt funktion auf dccPin
void dccdata() {
  getdcc = true;
  Timer1.start();  //Timer starten, warten bis zur Mitte zwischen HIGH und LOW ca. 70 Microsekunden
}

//****************************************************************   
//Timer nach Intterrupt auf DCC-Pin
void dcctime()
{
  if (getdcc == true) {
  Timer1.stop();  //Timer anhalten
  int State = digitalRead(dccPin);    //Einlesen des aktuellen Status des DCC-Eingangs
  if (getdata == true) {      //Sollen Daten gelesen werden?
    datalength += 1;  //Bitweise im Array weitergehen.
    countbit += 1;    //Abzählen der Bytes, also jeweils 8 Bit
  }
  
  if (State == LOW) {      //1-Bit gelesen
    countone += 1;            //Zählen der 1-Bit für Präambel erforderlich
    if (getdata == true) {    //eingelesenen Bitwert im Array Speichern
      data[datalength] = 1;    //Speichert das ein 1-Bit gelesen wurde
    }
    if (countbit > 8) {        //End-Bit gelesen.
      countbit = 0;            //setzte Bitzähler zurück (eigentlich nicht notwendig)
      getdata = false;         //Stop des Einlesen der Daten
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
      countbit = 0;      //setzte Bitzähler zurück
    }
    countone = 0;    //Es wurde ein 0-Bit gelesen, lösche Anzahl gelesener 1-Bit
  }  //Ende 0-Bit
  }
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
  
  //Daten wurden erkannt und die CRC Summe stimmt. Wenn nicht dann Not-STOP nach einer eingestellten Zeit. 
  dateninZeit = flash;  //Auto kann ohne bedenken weiterfahren. Zeitpunkt des Dateneingang wird festgehalten.
  makeSleep = false;    //Bei Aktivität von Daten kein Schlafen zulassen.  
  
  //CV Programm Mode writing - Änderung von Standardwerten im Decoder
  //Service Mode Instruction Packets for Direct Mode
    /*Bitconfiguration für das schreiben von CV Werten
      long-preamble (least 20 bits)
      Bit 0-7   : 1. Byte muss Adresse 124 sein -> WRITE CV BYTE (Achtung: letzten 2 bit sind Teil der 10 bit Adresse)
      Bit 9-16  : 2. Byte ist die CV Adresse (10 bit)
      Bit 18-25 : 3. Byte Data für die CV Adresse (8 bit)
      Bit 27-34 : 4. Byte CRC Summe
    */
    
  /*  
    Instructions packets using Direct CV Addressing are 4 byte packets of the format:
    long-preamble 0 0111CCAA 0 AAAAAAAA 0 DDDDDDDD 0 EEEEEEEE 1
    
    The actual Configuration Variable desired is selected via the 10 bit address with the two bit address (AA) in the first 
    data byte being the most significant bits of the CV number. The Configuration variable being addressed is the provided 
    10 bit address plus 1. CV #1 is defined by the address 00 00000000. A Command Station/Programmer must provide full read 
    and write manipulation for all values of 8 bit data.
    
    The defined values for Instruction types (CC) are:
      CC=10 Bit Manipulation
      CC=01 Verify byte
      CC=11 Write byte
    
    Type="01" VERIFY CV BYTE
      The contents of the Configuration Variable as indicated by the 10 bit address are compared with the data byte (DDDDDDDD). 
      If the values are identical the Digital Decoder shall respond with an acknowledgment as defined in Section D.

    Type="11" WRITE CV BYTE
      The contents of the Configuration Variable as indicated by the 10 bit address are replaced by the data byte (DDDDDDDD). 
      Upon completion of all write operations, the Digital Decoder may respond with an acknowledgment as defined in Section D.  
  */  
  if (data[0] == 0 && data[1] == 1 && data[2] == 1 && data[3] == 1 && data[4] == 1 && data[5] == 1 && data[6] == 0 && data[7] == 0 && datalength == 35) {  
  //Prüfen nach CV (Adresse 124 und ein 4 Byte Befehl)
    //1. Ausgänge Abschalten
    sollspeed = 0;
    istspeed = 0;
    setausgaenge();
    //2. CV Adr bestimmen aus 2. Byte
    int cvAdr = 0;
    bitWrite(cvAdr,7,data[9]);  //Bit 7 (128)
    bitWrite(cvAdr,6,data[10]); //Bit 6 (64)
    bitWrite(cvAdr,5,data[11]); //Bit 5 (32)
    bitWrite(cvAdr,4,data[12]); //Bit 4 (16)
    bitWrite(cvAdr,3,data[13]); //Bit 3 (8)
    bitWrite(cvAdr,2,data[14]); //Bit 2 (4)
    bitWrite(cvAdr,1,data[15]); //Bit 1 (2)
    bitWrite(cvAdr,0,data[16]); //Bit 0 (1)
    //3. CV Wert bestimmen aus 3. Byte
    int cvWert = 0;
    bitWrite(cvWert,7,data[18]);  //Bit 7 (128)
    bitWrite(cvWert,6,data[19]);      //Bit 6 (64)
    bitWrite(cvWert,5,data[20]);      //Bit 5  (32)
    bitWrite(cvWert,4,data[21]);      //Bit 4  (16)
    bitWrite(cvWert,3,data[22]);       //Bit 3  (8)
    bitWrite(cvWert,2,data[23]);       //Bit 2  (4)
    bitWrite(cvWert,1,data[24]);       //Bit 1  (2)
    bitWrite(cvWert,0,data[25]);       //Bit 0  (1)
    //4. schreibe Wert in EEPROM und entsprechende Variable
    updateRegister(cvAdr, cvWert);
    //keine weitern Daten vorhanden
    return; 
    //Auswertung Beenden.
  }  //Ende CV Programm Mode
      
  //2.1 Bestimmen kurzen Adresse (bis 127)
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
  int i,j;
  //3.1 Stimmt Adresse mit der des Decoders überein?
  if (dccAdr == decAdr) {
    //4. Datenlänge Prüfen. So kann das Datenformat ermittelt werden.
    if (datalength == 26 || (verschub == 9 && datalength == 35)) {  //1-Byte Adr + 0Bit + 1-Byte Data + 0-Bit + 1-Byte CRC + End-Bit (Basisformat)
       //5.1. Abfragen der Bits, diese Bestimmen welche Daten empfangen wurden.
       if (data[9+verschub] == 1 && data[10+verschub] == 0 && data[11+verschub] == 0) {    //Auswertung der Funktionen F0 bis F4
         
         if (data[12+verschub] == 1)    //F0 aktivieren
           f[0] = true;
         else f[0] = false;
         
         //activate F1 to F4         
         j = 16;
         for (i = 1;i<5;i++){
             if (data[j+verschub] == 1)
               f[i] = true;
             else f[i] = false;  
             j--;           
         }
       /*
         
         if (data[16+verschub] == 1)    //F1 aktivieren
           f[1] = true;
         else f[1] = false;
         if (data[15+verschub] == 1)    //F2 aktivieren
           f[2] = true;
         else f[2] = false;
         if (data[14+verschub] == 1)    //F3 aktivieren
           f[3] = true;
         else f[3] = false;
         if (data[13+verschub] == 1)    //F4 aktivieren
           f[4] = true;
         else f[4] = false;
         */
         //keine weitern Daten vorhanden
         return; 
         //Auswertung Beenden.
       }  //Ende der Auswertung Funktion F0 bis F4
       if (data[9+verschub] == 1 && data[10+verschub] == 0 && data[11+verschub] == 1 && data[12+verschub] == 1) {    //Auswertung der Funktionen F5 bis F8
         //activate F5 to F8 
         j = 16;
         for (i = 5;i<9;i++){
             if (data[j+verschub] == 1)
               f[i] = true;      
             else f[i] = false; 
             j--;      
         }
         /*
         if (data[16+verschub] == 1)    //F5 aktivieren
           f[5] = true;
         else f[5] = false;
         if (data[15+verschub] == 1)    //F6 aktivieren
           f[6] = true;
         else f[6] = false;
         if (data[14+verschub] == 1)    //F7 aktivieren
           f[7] = true;
         else f[7] = false;
         if (data[13+verschub] == 1)    //F8 aktivieren
           f[8] = true;
         else f[8] = false;
         */
         //keine weitern Daten vorhanden
         return; 
         //Auswertung Beenden.
       }  //Ende der Auswertung Funktion F5 bis F8
       if (data[9+verschub] == 1 && data[10+verschub] == 0 && data[11+verschub] == 1 && data[12+verschub] == 0) {    //Auswertung der Funktionen F9 bis F12
         //activate F9 to F12 
         j = 16;
         for (i = 9;i<13;i++){
             if (data[j+verschub] == 1)
               f[i] = true;      
             else f[i] = false;    
             j--;   
         }
         /*
         if (data[16+verschub] == 1)    //F9 aktivieren
           f[9] = true;
         else f[9] = false;
         if (data[15+verschub] == 1)    //F10 aktivieren
           f[10] = true;
         else f[10] = false;
         if (data[14+verschub] == 1)    //F11 aktivieren
           f[11] = true;
         else f[11] = false;
         if (data[13+verschub] == 1)    //F12 aktivieren
           f[12] = true;
         else f[12] = false;
         */
         //keine weitern Daten vorhanden
         return; 
         //Auswertung Beenden.
       }  //Ende der Auswertung Funktion F9 bis F12
       if (data[9+verschub] == 0 && data[10+verschub] == 1) {            //Auswertung der Fahrgeschwindigkeit/Fahrtrichtung 14 oder 28 Stufen
 /*        if (data[11+verschub] == 1)    //Fahrtrichtungsbit
           fahrtrichtung = true;
         else fahrtrichtung = false;  */
         sollspeed = 0;
         bitWrite(sollspeed,3,data[13+verschub]); //Bit 3 * 8
         bitWrite(sollspeed,2,data[14+verschub]); //Bit 2 * 4
         bitWrite(sollspeed,1,data[15+verschub]); //Bit 1 * 2
         bitWrite(sollspeed,0,data[16+verschub]); //Bit 0 * 1
         //bei 14 Fahrstufen nur jede 2. Fahrstufe von 28 nehmen.
         sollspeed = sollspeed * 2;    //Hochrechnen auf 28 Fahrstufen
         //Bei 28 Fahrstufen existiert ein weiteres Bit
         sollspeed += data[12+verschub];   //dazurechnen des zusätzliches Geschwindigkeits Bit nach NRMA Norm falls verfügbar
         //Bestimmen der Notstop-Befehle
         if (sollspeed >= 1 && sollspeed <= 3) { //Notstop
          sollspeed = 0;
          istspeed = 0;    //auch den Motor stoppen - Notstop
         }
         else {
           if (sollspeed != 0) {  //wenn kein normaler Stop dann 3 Notstopfahrstufen abziehen
             sollspeed = sollspeed - 3;    //abziehen der Notstopfahrstufen
           }
         }
         sollspeed = 126 / 28 * sollspeed;    //Hochrechnen auf 128 Fahrstufen.
         //keine weitern Daten vorhanden
         return; 
         //Auswertung Beenden.
       }
    }  //Ende der Datenlänge "26" - Basisformat
    if (datalength == 35 || (verschub == 9 && datalength == 44)) {  //1-Byte Adr + 0Bit + 1-Byte Adr/Data + 0-Bit + 1-Byte Data + 1-Byte CRC + End-Bit
      //5.2. Abfragen der Bits, diese Bestimmen welche Daten empfangen wurden.
        //126 Fahrstufen Auswerten
      if (data[9+verschub] == 0 && data[10+verschub] == 0 && data[11+verschub] == 1 && data[12+verschub] == 1 && data[13+verschub] == 1 && data[14+verschub] == 1 && data[15+verschub] == 1 && data[16+verschub] == 1) {
/*        if (data[18+verschub] == 1)   //Fahrtrichtungsbit
          fahrtrichtung = true;
        else fahrtrichtung = false;  */
        sollspeed = 0;
        bitWrite(sollspeed,6,data[19+verschub]); //Bit 6 * 64
        bitWrite(sollspeed,5,data[20+verschub]); //Bit 5 * 32
        bitWrite(sollspeed,4,data[21+verschub]); //Bit 4 * 16
        bitWrite(sollspeed,3,data[22+verschub]); //Bit 3 * 8
        bitWrite(sollspeed,2,data[23+verschub]); //Bit 2 * 4
        bitWrite(sollspeed,1,data[24+verschub]); //Bit 1 * 2
        bitWrite(sollspeed,0,data[25+verschub]); //Bit 0
        if (sollspeed == 1) { //Notstop
          sollspeed = 0;
          istspeed = 0;    //auch den Motor stoppen
        }
        if (sollspeed > 0)
          sollspeed -= 1;    //Notstop abziehen
        //keine weitern Daten vorhanden
        return; 
        //Auswertung Beenden.
      } //Ende 128 Fahrstufen  
        //Funktion F13 bis F20 im Expansion Byte lesen        
      if (data[9+verschub] == 1 && data[10+verschub] == 1 && data[11+verschub] == 0 && data[12+verschub] == 1 && data[13+verschub] == 1 && data[14+verschub] == 1 && data[15+verschub] == 1 && data[16+verschub] == 0) {
        //activate F13 to F20 
        j = 25;
         for (i = 13;i<21;i++){
             if (data[j+verschub] == 1)
               f[i] = true;      
             else f[i] = false;    
             j--;   
         }
        /*
        if (data[25+verschub] == 1)    //F13 aktivieren
          f[13] = true;
        else f[13] = false;
        if (data[24+verschub] == 1)    //F14 aktivieren
          f[14] = true;
        else f[14] = false;
        if (data[23+verschub] == 1)    //F15 aktivieren
          f[15] = true;
        else f[15] = false;
        if (data[22+verschub] == 1)    //F16 aktivieren
          f[16] = true;
        else f[16] = false;
        if (data[21+verschub] == 1)    //F17 aktivieren
          f[17] = true;
        else f[17] = false;
        if (data[20+verschub] == 1)    //F18 aktivieren
          f[18] = true;
        else f[18] = false;
        if (data[19+verschub] == 1)    //F19 aktivieren
          f[19] = true;
        else f[19] = false;
        if (data[18+verschub] == 1)    //F20 aktivieren
          f[20] = true;
        else f[20] = false;
        */
        //keine weitern Daten vorhanden
        return; 
        //Auswertung Beenden.
      } //Ende F13-F20
        //Funktion F21 bis F28 im Expansion Byte lesen        
      if (data[9+verschub] == 1 && data[10+verschub] == 1 && data[11+verschub] == 0 && data[12+verschub] == 1 && data[13+verschub] == 1 && data[14+verschub] == 1 && data[15+verschub] == 1 && data[16+verschub] == 1) {
        
        //activate F21 to F28 
        j = 25;
         for (i = 21;i<29;i++){
             if (data[j+verschub] == 1)
               f[i] = true;      
             else f[i] = false;    
             j--;   
         }
        /*
        if (data[25+verschub] == 1)    //F21 aktivieren
          f[21] = true;
        else f[21] = false;
        if (data[24+verschub] == 1)    //F22 aktivieren
          f[22] = true;
        else f[22] = false;
        if (data[23+verschub] == 1)    //F23 aktivieren
          f[23] = true;
        else f[23] = false;
        if (data[22+verschub] == 1)    //F24 aktivieren
          f[24] = true;
        else f[24] = false;
        if (data[21+verschub] == 1)    //F25 aktivieren
          f[25] = true;
        else f[25] = false;
        if (data[20+verschub] == 1)    //F26 aktivieren
          f[26] = true;
        else f[26] = false;
        if (data[19+verschub] == 1)    //F27 aktivieren
          f[27] = true;
        else f[27] = false;
        if (data[18+verschub] == 1)    //F28 aktivieren
          f[28] = true;
        else f[28] = false;
        */
        //keine weitern Daten vorhanden
        return; 
        //Auswertung Beenden.
      }  //Ende F21-F28
    }  //Ende der Datenlänge "35"
  }  //Ende der Adressauswertung
  
  //3.2 Bradcast Stop Packet?
/*  if (dccAdr == 0 && data[8] == 0 && data[9] == 0 && data[10] == 1 && data[13] == 0 && data[14] == 0 && data[15] == 0 && data[17] == 0 && data[25] == 1) {
    //Decoder wird in Grundstellung versetzt, Fahrtrichtung- und Richtungsinformationen gelöscht
    sollspeed = 0;
    istspeed = 0;
    f0 = false;
    f1 = false;
    f2 = false;
    f3 = false;
    f4 = false;
    f5 = false;
    f6 = false;
    f7 = false;
    f8 = false;
    f9 = false;
    f10 = false;
    f11 = false;
    f12 = false;
    f13 = false;
    f14 = false;
    f15 = false;
    f16 = false;
    f17 = false;
    f18 = false;
    f19 = false;
    f20 = false;
    f21 = false;
    f22 = false;
    f23 = false;
    f24 = false;
    f25 = false;
    f26 = false;
    f27 = false;
    f28 = false;
    //Setzten der Decoderausgänge
    setausgaenge();
  }  //Ende Bradcast Stop Packet
 */
}

//****************************************************************  
//Bestimmen der Differenz zwischen zwei Werten.
int diff(int x1, int x2) {
  int d = x1 - x2;
  if (d < 0) {    //Prüfen ob, Differenz negativ?
    d = d - (2 * d);  //Zahl positiv machen.
  }
  return d;
}

//****************************************************************  
//Ausgänge anhand der Speicherwerte schalten/setzten
void setausgaenge() {
  flash += 1;      //Taktgeber für das Blinken der Ausgänge und Zeitgeber für Lichtdelay
  int akkuwert = analogRead(akkuPin) / 4;
  //______________________________________________________________________  
  //1. Automatisches Anhalten, wenn keine DCC-Daten für (dateninPause+1 * 10 * 2 ms) erhalten wurden.  
  //Prüfen ob immernoch DCC-Daten gesendet werden?
  if (diff(dateninZeit,flash) > (dateninPause+1)*2) {  //DCC-Daten wurden in (Zeit < dateninpause+1 * 10 * 2 ms) empfangen, wenn nicht dann Not-STOP
    //Zeit in der DCC Signale erfasst werden müssen ist abgelaufen.
    if (sollspeed > 0 || istspeed > 0) {  //Prüfen ob Auto fährt
      //Warnblinkanlage einschalten.
      f[1] = true;
      f[2] = true;
    }
    //Auto anhalten damit kein Schaden entsteht.
    sollspeed = 0;
    istspeed = 0;
  }
  //Wenn keine Daten empfangen werden, Timer1 aktivieren damit Rücklicht und Blinker (PIN 11) anbleibt!
  if (diff(dateninZeit,flash) > 2 && getdcc == true) {  //sobald keine Daten erkannt wurden, einmal aktivieren
    getdcc = false;          //dcc Erkennung deaktivieren.
    Timer1.start();          //Timer1 aktivieren, der sonst nicht läuft. PWM wird somit wieder an PIN 10 & 11 aktiv
  }

  //______________________________________________________________________  
  //2. Einschlafen Prüfen, ist ausreichend Zeit ohne Daten verstichen?
  if (diff(dateninZeit,flash) > (datenSleepPause+1)*100) {   //Prüfen ob die Zeit ohne Daten abgelaufen ist?
    makeSleep = true;    //Prozessor in den Ruhemodus setzten.
    setsleep();          //Prüfen ob Decoder abgeschaltet werden kann
  }
  
  //______________________________________________________________________ 
  //3. Blinker ansteuern
  if (0 == flash % blinktakt) {      //Blinkfequenz für Blinker links und rechts
    binkzustand = !binkzustand;
    if (((f[1] == true) && (f[2] == true)) || ((akkuwert < blinkAkkuleer) && (akkuwarnunginaktiv == false))) {  //Warnblinkanlage ((blinkAkkuleer * 4) / 2.03 = Volt)
      if (binkzustand == true) {
        analogWrite(F1Pin, binkhelligkeit);     //Blinker links setzten
        analogWrite(F2Pin, binkhelligkeit);    //Blinker rechts setzten
      }
      else {
        digitalWrite(F1Pin, LOW);    //Blinker links löschen
        digitalWrite(F2Pin, LOW);    //Blinker rechts löschen
      }
    }
    else {
      if (((f[1] == true) && (blinkerinvertiert == false)) || ((f[2] == true) && (blinkerinvertiert == true))) {  //blinken links oder invertiert
        if (binkzustand == true)
          analogWrite(F1Pin, binkhelligkeit);     //Blinker links setzten
        else digitalWrite(F1Pin, LOW);    //Blinker links löschen  
      }
      else digitalWrite(F1Pin, LOW);    //Blinker links löschen, falls gerade bei HIGHT abgeschaltet
      if (((f[2] == true) && (blinkerinvertiert == false)) || ((f[1] == true) && (blinkerinvertiert == true))) {  //blinken rechts oder invertiert
        if (binkzustand == true)
          analogWrite(F2Pin, binkhelligkeit);     //Blinker rechts setzten
        else digitalWrite(F2Pin, LOW);    //Blinker rechts löschen  
      }
      else digitalWrite(F2Pin, LOW);    //Blinker rechts löschen, falls gerade bei HIGHT abgeschaltet
    }
  }

  //______________________________________________________________________
  //4.Abblendlicht ansteuern
  if (f[0] == true) {  //Abblendlicht ist aktiv.
    if (f[3] == true) {  //Lichtautomatik ist aktiv
      int val = analogRead(ldrPin);    // read the input ldrPin
      if (val > (ldrlichtaus*4)) {    //schwelle für Licht AUS (Standard = 630)
        if (licht == true && (diff(ldrdelay,flash) > ldrdelayaus))  //Warten für 10 * 200 = 2000ms bis Status geändert wird
          licht = false;     ///Meldung für Abblendlicht AUS
        if (licht == false)    //Einstellung der Wartezeit
          ldrdelay = flash;
      }
      if (val < (ldrlichtein*4)) {    //schwelle für Licht EIN  (Standard = 470)
        if (licht == false && (diff(ldrdelay,flash) > ldrdelayein))  //Warten für 10 * 100 = 1000ms bis Status geändert wird
          licht = true;    ///Meldung für Abblendlicht EIN
        if (licht == true)    //Einstellung der Wartezeit
          ldrdelay = flash;
      }
    }
    else   //keine Lichtautomatik
      licht = true;    ///Meldung für Abblendlicht EIN
  }
  else licht = false;    //Meldung für Abblendlicht AUS

  //______________________________________________________________________  
  //5. Geschwindigkeit mit Bremslicht steuern (alles mit 126 Fahrstufen!)
  if (istspeed == 0) {        //Bremslicht im Stand länger nachleuchten
    if ((bremsdelay+bremsnachleuchtstand) < flash)    //Nachleuchten von 10*300 = 3000ms.
      bremslicht = false;    //Bremslicht AUS
  }
  else {                    //Bremslicht nachleuchten wärend der Fahrt
    if ((bremsdelay+bremsnachleuchtfahrt) < flash)    //Bremslicht nach 10*80 = 800ms abschalten
      bremslicht = false;    //Bremslicht AUS
  }
  if (sollspeed < istspeed) {    //Abbremsen des Motors
    bremsdelay = flash;      //Nachleuchtverzögerung setzten
    if ((sollspeed+(sollspeed/bremslichtschnell) < istspeed) && (istspeed > 1)) {  //schnelles Abbremsen (Wert ist Standard 4)
      if (0 == flash % ((bremsrate+1) / 2)) {  //CV Bremsrate, diese darf nicht Null sein
        istspeed -= 2;        //Geschwindigkeit stark reduzieren
        bremslicht = true;    //Bremslicht EIN
      }
    }
    else {              //normales Abbremsen
      if (0 == flash % (bremsrate+1)) {  //CV Bremsrate, diese darf nicht Null sein
        istspeed -= 1;          //Geschwindigkeit leicht reduzieren
        if (sollspeed < bremslichtschwelle)     //Bremslicht nur bei geringer Geschwindigkeit einschalten
          bremslicht = true;   //Bremslicht EIN
      }
    }
  }
  if (sollspeed > istspeed) {    //Beschleunigen des Motors
    if (0 == flash % (beschleunigungsrate+1)) { //CV Beschleunigungsrate, diese darf nicht Null sein.
      istspeed += 1;          //Geschwindigkeit leicht erhöhen
      if (sollspeed > 0)      //Bremslicht erst wirklich beim beschleunigen Abschalten nicht vorher!
        bremslicht = false;    //Bremslicht AUS
    }
  }

  //______________________________________________________________________  
  //6. Bremslicht und Abblendlicht setzen
  if (licht == true) {     //wenn normales Licht EIN 
    analogWrite(lichtPin, lichthelligkeit);    //Abblendlicht EIN
    if (bremslicht == true) {    //wenn Bremslicht EIN
      analogWrite(rueckPin, bremshelligkeit);    //Bremslicht einschalten
//      SoftPWMSet(rueckPin, bremshelligkeit);    //Bremslicht einschalten
    }
    else {
      analogWrite(rueckPin, rueckhelligkeit);    //gedimmtes Rücklicht
//      SoftPWMSet(rueckPin, rueckhelligkeit);    //gedimmtes Rücklicht
    }
  }
  else {    //normales Licht AUS
    digitalWrite(lichtPin, LOW);    //Abblendlicht AUS
    if (bremslicht == true) {    //wenn Bremslicht EIN
      analogWrite(rueckPin, bremshelligkeit);    //Bremslicht einschalten
//      SoftPWMSet(rueckPin, bremshelligkeit);    //Bremslicht einschalten
    }
    else {
      digitalWrite(rueckPin, LOW);  //Bremslicht ausschalten
//      SoftPWMSet(rueckPin, 0);  //Bremslicht ausschalten
    }
  }

  //______________________________________________________________________  
  //7. Motor ansteuern, Berechnung der Motorgeschwindigkeit für 8 Bit (0-255)
  if (istspeed > 0 && akkuwert > 167) {    //wenn Motor nicht steht und Akku mehr als 3,3 Volt hat (Wert * 4 / 203 = Volt)
    int mValue = map(istspeed, 1, 125, startspannung, maximalspannung);    //Geschwindigkeit für Motor berechnen
        //bei istspeed - 1 weil dort die Fahrstufe "Stop" abgezogen werden muss, deshalb auch nur 125!
    int akkuausgleich = (speedakkuwert - akkuwert) / 2; //Akkuspannung mit einrechnen und nicht zu stark ausgleichen!
    if (akkuausgleich > 0)          //Prüfen ob Akkuspannung zu niedrig ist?
      mValue = mValue + akkuausgleich;   //geringe Akkuspannung mit höherer Motorspannungs ausgleichen 
    analogWrite(SpeedPin, mValue);        //Geschwindigkeit für Motor setzten
  }
  else {
    analogWrite(SpeedPin, 0);  //Motor ist aus.
  }
  
  //______________________________________________________________________  
  //8. Ausgänge für Einsatz, Programmierbar:
  //Port1:
  if (check(aux1funktion) == true) {
    if (aux1art == 0)
      setSirene(aux1,sireneArt);  //hier Sirene steuern. (blitz steht für Signalart)
    else {  
      if (aux1time <= flash) {
        auxtime = aux1time;
        aux1seq = setPort(aux1, aux1funktion, (aux1art-1), aux1seq, aux1blitz, aux1p1, aux1p2);
        aux1time = auxtime;
      }
    }
  }  
  //Port2:
  if (aux2time <= flash) {
    auxtime = aux2time;
    aux2seq = setPort(aux2, aux2funktion, aux2art, aux2seq, aux2blitz, aux2p1, aux2p2);
    aux2time = auxtime;
  }
  //Port3:
  if (aux3time <= flash) {  
    auxtime = aux3time;
    aux3seq = setPort(aux3, aux3funktion, aux3art, aux3seq, aux3blitz, aux3p1, aux3p2);
    aux3time = auxtime;
  }
  //Port4:
  if (aux4time <= flash) {
    auxtime = aux4time;
    aux4seq = setPort(aux4, aux4funktion, aux4art, aux4seq, aux4blitz, aux4p1, aux4p2);
    aux4time = auxtime;
  }  
  //Port5:
  if (aux5time <= flash) {
    auxtime = aux5time;
    aux5seq = setPort(aux5, aux5funktion, aux5art, aux5seq, aux5blitz, aux5p1, aux5p2);
    aux5time = auxtime;
  }  
  //Port6:
  if (aux6time <= flash) {
    auxtime = aux6time;
    aux6seq = setPort(aux6, aux6funktion, aux6art, aux6seq, aux6blitz, aux6p1, aux6p2);
    aux6time = auxtime;
  }
  //Rundumlicht
  if (check(rlichtfunktion) && rlichttime <= flash) {  
    if ((rlichtseq == 0 | rlichtseq == 8)) {
      rlichtseq = rlichtseq + 1;
      analogWrite(rundlichtpin,0);  //Ausgang Rundumlicht ansteuern
      rlichttime = flash + rlichtZeit;
      if (rlichtseq >= 8)
        rlichtseq = 0;
    }
    if ((rlichtseq == 1 | rlichtseq == 7)) {
      rlichtseq = rlichtseq + 1;
      analogWrite(rundlichtpin,40);  //Ausgang Rundumlicht ansteuern
      rlichttime = flash + 1;
    }
    if ((rlichtseq == 2 | rlichtseq == 6)) {
      rlichtseq = rlichtseq + 1;
      analogWrite(rundlichtpin,80);  //Ausgang Rundumlicht ansteuern
      rlichttime = flash + 1;
    }
    if ((rlichtseq == 3 | rlichtseq == 5)) {
      rlichtseq = rlichtseq + 1;
      analogWrite(rundlichtpin,150);  //Ausgang Rundumlicht ansteuern
      rlichttime = flash + 2;
    }
    if (rlichtseq == 4) {
      rlichtseq = rlichtseq + 1;
      analogWrite(rundlichtpin,255);  //Ausgang Rundumlicht ansteuern
      rlichttime = flash + 2;
    }
  }
  if (check(rlichtfunktion) == false) {
    digitalWrite(rundlichtpin,LOW);  //Ausgang Rundumlicht abschalten
    rlichttime = flash + 20;    //Pausieren der Abfrage für 200ms
  }
} //Ende setausgaenge

//****************************************************************  
//Programmierbare Ports setzen, nächste Sequenz errechnen.
int setPort(int pin, int funktion, int art, int seq, int blitz, int p1, int p2) {
  if (check(funktion) == true) {    //Prüfen ob die Funktion für den Ausgang AKTIV ist
    if (art > 0) {    //Auswahl von kein- bis dreifachblitz.
      auxtime = einsatz(pin,seq,auxtime,blitz,p1,p2);    //Blitzfequenz bestimmen
      seq = seq + 1;  //eine Sequenz weitergehen
      if (seq > (((art) * 2) - 1) )  //Anzahl der Sequenzen erreicht, Doppelblitz
        seq = 0;     //von vorne beginnen 
      return seq;
    }
    digitalWrite(pin,HIGH);    //kein Blinken (blau1art == 0)
    auxtime = flash + 20;
    return seq;  
  }
  digitalWrite(pin,LOW);    //Abschalten des Ausgang
  auxtime = flash + 20;          //Speicher der Flash Werte damit Wiedereinschalten klappt
  return seq;
}

int setPort(aux_pin_struct *auxpinp) {
  if (check(auxpinp->function) == true) {    //Prüfen ob die Funktion für den Ausgang AKTIV ist
    if (art > 0) {    //Auswahl von kein- bis dreifachblitz.
      auxtime = einsatz(auxpinp);    //Blitzfequenz bestimmen
      auxpinp->seq = auxpinp->seq + 1;  //eine Sequenz weitergehen
      if (auxpinp->seq > (((auxpinp->art) * 2) - 1) )  //Anzahl der Sequenzen erreicht, Doppelblitz
        auxpinp->seq = 0;     //von vorne beginnen 
      return auxpinp->seq;
    }
    digitalWrite(auxpinp->port,HIGH);    //kein Blinken (blau1art == 0)
    auxtime = flash + 20;
    return auxpinp->seq;  
  }
  digitalWrite(auxpinp->port,LOW);    //Abschalten des Ausgang
  auxtime = flash + 20;          //Speicher der Flash Werte damit Wiedereinschalten klappt
  return auxpinp->seq;
}


//****************************************************************  
//Funktion zur Ansteuerung der Sirene
void setSirene(int pin, int art) {
  if (check(sirenefunktion) == true) {
    switch(art) {      //Sirene einschalten.
      case 2: if (0 == flash % 180) 
                tone(pin, 466, 700); 
              if (0 == (flash+90) % 180) 
                tone(pin, 622, 700); 
              break; 
      case 3: if (0 == flash % 200)  //Signalhorn IFA W50L LF16 (wechsel pro sec.) von Ungarischen Firma Elektris
                tone(pin, 435, 900); 
              if (0 == (flash+100) % 200) 
                tone(pin, 580, 900);
              break;
      case 4: if (0 == flash % 200)     
                tone(pin, 450, 900); 
              if (0 == (flash+100) % 200) 
                tone(pin, 600, 900);
              break;              
      case 8: tone(pin, 400);        //Normale Hupe
              break;
      default: if (0 == flash % 180)       //Signalhorn
                 tone(pin, 440, 700); 
               if (0 == (flash+90) % 180) 
                 tone(pin, 585, 700); 
               break;
    }  
  }
  else noTone(pin);    //Sirene abschalten
}

//****************************************************************  
//Prüfen ob und welche Funktion einen bestimmten Ausgangspin schaltet und ob diese aktiv ist.
boolean check(int funktionen) {
  /*Prüft die 8 Bit
    bit 0 = F4
    bit 1 = F5
    bit 2 = F6
    bit 3 = F7
    bit 4 = F8
    bit 5 = F9
    bit 6 = F10
    bit 7 = F11
  */
  if (f[4] == true && bitRead(funktionen,0) == 1)  //Reaktion auf F4
    return true;    //Funktion ist aktiv - Ausgang EIN
  if (f[5] == true && bitRead(funktionen,1) == 1)  //Reaktion auf F5
    return  true;    //Funktion ist aktiv - Ausgang EIN
  if (f[6] == true && bitRead(funktionen,2) == 1)  //Reaktion auf F6
    return  true;    //Funktion ist aktiv - Ausgang EIN
  if (f[7] == true && bitRead(funktionen,3) == 1)  //Reaktion auf F7
    return  true;    //Funktion ist aktiv - Ausgang EIN
  if (f[8] == true && bitRead(funktionen,4) == 1)  //Reaktion auf F8
    return  true;    //Funktion ist aktiv - Ausgang EIN
  if (f[9] == true && bitRead(funktionen,5) == 1)  //Reaktion auf F9
    return  true;    //Funktion ist aktiv - Ausgang EIN
  if (f[10] == true && bitRead(funktionen,6) == 1)  //Reaktion auf F10
    return  true;    //Funktion ist aktiv - Ausgang EIN
  if (f[11] == true && bitRead(funktionen,7) == 1)  //Reaktion auf F11
    return  true;    //Funktion ist aktiv - Ausgang EIN
  return false;      //Rückgabe, die Funktion ist inaktiv!
}

//****************************************************************  
//Steuerung der Ausgänge für Einsatz
int einsatz(int ausgang, int seq, int time, int blitz, int pause1, int pause2) {
  int output = time;    //Rückgabe der alten Zeit
    if (seq == 0) {    //1. Blitz
      digitalWrite(ausgang,HIGH);
      output = flash + blitz;    //neue LED Zeit bestimmen
      return output;
    }
    if (seq == 1) {    //Pause
      digitalWrite(ausgang,LOW);
      output = flash + pause1;  //neue LED Zeit bestimmen
      return output;
    }
    if (seq == 2) {    //2. Blitz
      digitalWrite(ausgang,HIGH);
      output = flash + blitz;  //neue LED Zeit bestimmen
      return output;
    }
    if (seq == 3) {    //Pause
      digitalWrite(ausgang,LOW);
      output = flash + pause2;  //neue LED Zeit bestimmen
      return output;
    }
    if (seq == 4) {    //3. Blitz
      digitalWrite(ausgang,HIGH);
      output = flash + blitz;  //neue LED Zeit bestimmen
      return output;
    }
    if (seq >= 5) {    //Pause
      digitalWrite(ausgang,LOW);
      output = flash + pause2;  //neue LED Zeit bestimmen
      return output;
  }
  return output;  
}

//****************************************************************  
//Steuerung der Ausgänge für Einsatz
int einsatz(aux_pin_struct *auxp) {
  int output = time;    //Rückgabe der alten Zeit
    if (auxp->seq == 0) {    //1. Blitz
      digitalWrite(auxp->port,HIGH);
      output = flash + auxp->blitz;    //neue LED Zeit bestimmen
      return output;
    }
    if (auxp->seq == 1) {    //Pause
      digitalWrite(auxp->port,LOW);
      output = flash + auxp->p1;  //neue LED Zeit bestimmen
      return output;
    }
    if (auxp->seq == 2) {    //2. Blitz
      digitalWrite(auxp->port,HIGH);
      output = flash + auxp->blitz;  //neue LED Zeit bestimmen
      return output;
    }
    if (auxp->seq == 3) {    //Pause
      digitalWrite(auxp->port,LOW);
      output = flash + auxp->p2;  //neue LED Zeit bestimmen
      return output;
    }
    if (auxp->seq == 4) {    //3. Blitz
      digitalWrite(auxp->port,HIGH);
      output = flash + auxp->blitz;  //neue LED Zeit bestimmen
      return output;
    }
    if (auxp->seq >= 5) {    //Pause
      digitalWrite(auxp->port,LOW);
      output = flash + auxp->p2;  //neue LED Zeit bestimmen
      return output;
  }
  return output;  
}

//**************************************************************** 
//Aufblinken der Abblendlichter und Rücklichter(Bremslicht) zur Bestätigung der erfolgreichen Programmierung
void setLedBlitz(int anz, boolean dccon) {
  int delaytime = 1800;  //Verzögerung für Blitz wenn DCC Daten aktiv sind
  if (dccon == false)   //Prüfen ob DCC Daten inaktiv
    delaytime = 70;
  digitalWrite(lichtPin, LOW);
  digitalWrite(rueckPin, LOW);
  delay(30);
  for (int i = 0; i < anz; i++) {    //Anzahl der "Blitze"
    digitalWrite(lichtPin, HIGH);    //Ausgang aktivieren
    digitalWrite(rueckPin, HIGH);
    delay(delaytime);
    digitalWrite(lichtPin, LOW);      //Ausgang deaktivieren
    digitalWrite(rueckPin, LOW);
    if (dccon == false) 
      delay(delaytime);
    else delay(delaytime + 300);  
  }
}

//****************************************************************  
//Übernehmen der nutzerdefinierten Einstellungen aus dem EEPROM:
//Funktion zum Einlesen der CV Einstellungen beim Start
void getRegister() {  
  //Einlesen der Register und Speichern der Werte in den zugehörigen Variablen
  //______________________________________________________________________  
  //Grundregister Einlesen:
  decAdr = getCVwert(CVdecadr);    //Decoderadresse (kurz)
  if (decAdr > CVdecadrmax) {    //Erster Start des Decoder (EEPROM leer), Adresse wird sonst nie 255! 
                          //Max. möglicher Wert bis 111 für kurze Adressen im EEPROM 0.
    updateRegister(CVreset,0);    //setzten aller Werte des EEPROM mit den vorhandenen default Werten!
    return;               //Verlassen, da bereits alle Variablen bereits durch "updateeeprom" gesetzt worden.
  }
  if (decAdr == 0) {  //dann lange Adressen verwenden!
    decAdr = Adrrechnen(getCVwert(CVerweiterteadr1), getCVwert(CVerweiterteadr2));  //Adresse errechnen
  }
  startspannung = getCVwert(CVstartspannung);      //Spannung, die bei Fahrstufe 1 am Motor anliegt. (0 = 0 Volt und 255 = max. Spannung)
  beschleunigungsrate = getCVwert(CVbeschleunigungsrate);    //Zeit bis zum Hochschalten zur nächst höheren Fahrstufe  (CV3 * 0,9 sec / Anz. Fahrstufen)
  bremsrate = getCVwert(CVbremsrate);          //Zeit bis zum Herunterschalten zur nächst niedriegern Fahrstufe (CV3 * 0,9 sec / Anz. Fahrstufen)
  maximalspannung = getCVwert(CVmaxspannung);    //Spannung die bei höchster Fahrstufe anliegt. (0 = min. Spannung und 255 = max. Volt)
  if (maximalspannung == 0) {      //Bei einem Wert von Null auf Maximalspannung setzten!
    maximalspannung = 255;
  }
  
  //______________________________________________________________________    
  //Überprüfen der Dekoderversion
  int altDecVersion = getCVwert(CVdecversion);  //Decoderversion auslesen
  int neuDecVersion = 0;          //Bestimmen der Dekoderversion aus den drei Konstanten
  bitWrite(neuDecVersion,0,bitRead(decversion1,0));      //erste Stelle 2 Bit
  bitWrite(neuDecVersion,1,bitRead(decversion1,1));
  bitWrite(neuDecVersion,2,bitRead(decversion2,0));      //zweite Stelle 3 Bit
  bitWrite(neuDecVersion,3,bitRead(decversion2,1));
  bitWrite(neuDecVersion,4,bitRead(decversion2,2));
  bitWrite(neuDecVersion,5,bitRead(decversion3,0));      //dritte Stelle 3 Bit
  bitWrite(neuDecVersion,6,bitRead(decversion3,1));
  bitWrite(neuDecVersion,7,bitRead(decversion3,2));
  if (altDecVersion != neuDecVersion) {      //Prüfen ob der Wert schon im EEPRM steht?
    setCVwert(CVdecversion, neuDecVersion);   //aktuelle Decoderversion im EEPROM ablegen
    //Testphase: Dazu alle Ausgänge des Dekoder einschalten
    PORTD = B11011000;        //Alle Ausgänge an PortD einschalten, außer Motor (5) und Pin 0-2
    PORTC = B00001111;        //Alle Ausgänge an PortC einschalten, A0 bis A3
    PORTB = 0xFF;             //Alle Ausgänge an PortB einschalten 
    analogWrite(SpeedPin, (maximalspannung/2));    //Motor mit maxspeed/2 einschalten.
    delay(4000);        //Warten für 4 sec.
    //Testphase mit Blinken beenden!
    setLedBlitz(5,false);    //vier Blitze für eine erfolgreiche Programmierung (da die Ausgänge schon aktiv sind nur vier!).
    setPinMode();    //setzen der Modi für die Pins auf normal. Weiter im Programmablauf:
  }
  
  //______________________________________________________________________    
  //weitere Registerwerter auslesen:
  speedakkuwert = getCVwert(CVspeedakkuwert);    //Akkuwert andem sich die CV 2-5 orientieren
  blinkAkkuleer = getCVwert(CVblinkAkkuleer);     //Vergleichswert für Akkutest
  blinktakt = getCVwert(CVblinktakt);         //Blinkergeschwindigkeit
  dateninPause = getCVwert(CVdateninPause);      //Zeit die ohne Daten verstreichen kann bis Not-STOP
  ldrlichtein = getCVwert(CVldrlichtein);  //Licht EIN Schwellwert
  ldrdelayein = getCVwert(CVldrdelayein);  //Licht EIN Delay
  ldrlichtaus = getCVwert(CVldrlichtaus);  //Licht AUS Schwellwert
  ldrdelayaus = getCVwert(CVldrdelayaus);  //Licht AUS Delay
  bremslichtschwelle = getCVwert(CVbremslichtschwelle);    //Fahrstufe ab der Bremslicht sofort EIN geht
  bremslichtschnell = getCVwert(CVbremslichtschnell);     //Reduzierung der Geschwindigkeit um 1/Wert = EIN
  bremsnachleuchtstand = getCVwert(CVbremsnachleuchtstand);  //Nachleuchten des Bremslicht im Stand
  bremsnachleuchtfahrt = getCVwert(CVbremsnachleuchtfahrt);  //Nachleuchten des Bremslicht während der Fahrt
  rueckhelligkeit = getCVwert(CVrueckhelligkeit);   //Dimmen Ausgang Rücklicht
  bremshelligkeit = getCVwert(CVbremshelligkeit);   //Dimmen Ausgang Bremslicht
  binkhelligkeit = getCVwert(CVbinkhelligkeit);    //Dimmen Ausgang Blinklicht
  lichthelligkeit = getCVwert(CVlichthelligkeit);   //Dimmen Ausgang Licht (Abbledlicht vorn)
  datenSleepPause = getCVwert(CVdatenSleepPause);   //Sleep spart 90% Akku, wenn keine DCC Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
  SleepLowTime = getCVwert(CVSleepLowTime);   //Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)
  
  //______________________________________________________________________    
  //Programmierbare Ausgänge Register auslesen:
  sirenefunktion = getCVwert(CVsirenefunktion); //Zusätzliche Funktion die aktiv sein muss für die Sirene (zB. Blaulicht) 
  rlichtfunktion = getCVwert(CVrlichtfunktion); //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
  aux1funktion = getCVwert(CVaux1funktion); 
  aux2funktion = getCVwert(CVaux2funktion); 
  aux3funktion = getCVwert(CVaux3funktion); 
  aux4funktion = getCVwert(CVaux4funktion); 
  aux5funktion = getCVwert(CVaux5funktion); 
  aux6funktion = getCVwert(CVaux6funktion); 
  int cv67 = getCVwert(CVArtFunktionA);    //Art der Funktion von 0 bis 4 je Ausgang
  int cv68 = getCVwert(CVArtFunktionB);
  aux1art = 0;
  bitWrite(aux1art,0,bitRead(cv67,0));    //Auslesen des 0. und 1. Bit von CV67 für aux1
  bitWrite(aux1art,1,bitRead(cv67,1));
  aux2art = 0;
  bitWrite(aux2art,0,bitRead(cv67,2));    //Auslesen des 2. und 3. Bit von CV67 für aux2
  bitWrite(aux2art,1,bitRead(cv67,3));
  aux3art = 0;
  bitWrite(aux3art,0,bitRead(cv67,4));    //Auslesen des 4. und 5. Bit von CV67 für aux3
  bitWrite(aux3art,1,bitRead(cv67,5));
  aux4art = 0;
  bitWrite(aux4art,0,bitRead(cv67,6));    //Auslesen des 6. und 7. Bit von CV67 für aux4
  bitWrite(aux4art,1,bitRead(cv67,7));
  aux5art = 0;
  bitWrite(aux5art,0,bitRead(cv68,0));    //Auslesen des 0. und 1. Bit von CV68 für aux5
  bitWrite(aux5art,1,bitRead(cv68,1));
  aux6art = 0;
  bitWrite(aux6art,0,bitRead(cv68,2));    //Auslesen des 2. und 3. Bit von CV68 für aux6
  bitWrite(aux6art,1,bitRead(cv68,3));
  if (bitRead(cv68,4) == 1)   //Invertierung der Blinkeransteuerung
    blinkerinvertiert = true;
  else blinkerinvertiert = false;  
  if (bitRead(cv68,5) == 1)    //Abschaltung der Akkuwarnung
    akkuwarnunginaktiv = true;
  else akkuwarnunginaktiv = false;  
  sireneArt = getCVwert(CVsireneArt);    //Art der Sirene (2 Bit);
  rlichtZeit = getCVwert(CVrlichtZeit);   //Geschwindigkeit des Rundumlicht
  aux1blitz = getCVwert(CVaux1blitz);    //Blitzlänge aux1
  aux2blitz = getCVwert(CVaux2blitz);    //Blitzlänge aux2
  aux3blitz = getCVwert(CVaux3blitz);    //Blitzlänge aux3
  aux4blitz = getCVwert(CVaux4blitz);    //Blitzlänge aux4
  aux5blitz = getCVwert(CVaux5blitz);    //Blitzlänge aux5
  aux6blitz = getCVwert(CVaux6blitz);    //Blitzlänge aux6
  aux1p1 = getCVwert(CVaux1p1);     //Blinkpause1 aux1
  aux2p1 = getCVwert(CVaux2p1);     //Blinkpause1 aux2
  aux3p1 = getCVwert(CVaux3p1);     //Blinkpause1 aux3
  aux4p1 = getCVwert(CVaux4p1);     //Blinkpause1 aux4
  aux5p1 = getCVwert(CVaux5p1);     //Blinkpause1 aux5
  aux6p1 = getCVwert(CVaux6p1);     //Blinkpause1 aux6
  aux1p2 = getCVwert(CVaux1p2);     //Blinkpause2 aux1
  aux2p2 = getCVwert(CVaux2p2);     //Blinkpause2 aux2
  aux3p2 = getCVwert(CVaux3p2);     //Blinkpause2 aux3
  aux4p2 = getCVwert(CVaux4p2);     //Blinkpause2 aux4
  aux5p2 = getCVwert(CVaux5p2);     //Blinkpause2 aux5
  aux6p2 = getCVwert(CVaux6p2);     //Blinkpause2 aux6
  
  if (autoSerial == 256)  //Wert der Serial nur nach einem Reset aktiviert!
    autoSerial = getCVwert(CVautoSerial) * 10;    //Zeit für Time des aktiven Serial beim Einschalten;
} //Ende geteeprom

//****************************************************************  
//Funktion zum Setzten der Einstellungen im Betrieb durch neue CV Werte (über Decoder-Adr. 124)
void updateRegister(int cvAdr, int cvWert) {
  if (cvAdr == CVreset) {  //Reset (Eingabe eines beliebigen Wertes werden die Einstellungen im Auslieferungszustand wieder herstellt)
    /*setzten der Standartwerte im EEPROM
      Vorheriges Prüfen ob der Wert im EEPROM so schon vorhanden ist, dann kein write auf EEPROM durchführen.
      Hierzu ist kein EEPROM read notwendig, da alle Werte zusätzlich in Variablen gespeichert werden.
    */
    setCVwert(CVdecadr, CVdefaultdecadr);      //CV1 = Adresse = 3 normal
    setCVwert(CVstartspannung, CVdefaultstartspannung);    //CV2 = Spannung, die bei Fahrstufe 1 am Motor anliegt. (0 = 0 Volt und 255 = max. Spannung)
    setCVwert(CVbeschleunigungsrate, CVdefaultbeschleunigungsrate);   //CV3 = Zeit bis zum Hochschalten zur nächst höheren Fahrstufe  (CV3 * 0,9 sec / Anz. Fahrstufen)
    setCVwert(CVbremsrate, CVdefaultbremsrate);    //CV4 = Zeit bis zum Herunterschalten zur nächst niedriegern Fahrstufe (CV3 * 0,9 sec / Anz. Fahrstufen)
    setCVwert(CVmaxspannung, CVdefaultmaxspannung);     //CV5 = Spannung die bei höchster Fahrstufe anliegt. (0 = min. Spannung und 255 = max. Volt)
    setCVwert(CVspeedakkuwert,CVdefaultspeedakkuwert);    //Akkuwert andem sich die CV 2-5 orientieren
    setCVwert(CVblinkAkkuleer, CVdefaultblinkAkkuleer);    //Vergleichswert für Akkutest ca 3 Volt
    setCVwert(CVblinktakt, CVdefaultblinktakt);     //Blinkergeschwindigkeit
    setCVwert(CVdateninPause, CVdefaultdateninPause);   //Zeit die verstreichen kann ohne DCC-Daten
    setCVwert(CVldrlichtein, CVdefaultldrlichtein);  //Licht EIN Schwellwert
    setCVwert(CVldrdelayein, CVdefaultldrdelayein);  //Licht EIN Delay
    setCVwert(CVldrlichtaus, CVdefaultldrlichtaus);  //Licht AUS Schwellwert
    setCVwert(CVldrdelayaus, CVdefaultldrdelayaus);  //Licht AUS Delay
    setCVwert(CVbremslichtschwelle, CVdefaultbremslichtschwelle);    //Fahrstufe ab der das Bremslicht sofort EIN geht beim Reduzieren
    setCVwert(CVbremslichtschnell, CVdefaultbremslichtschnell);      //Bei Reduzierung der Fahrstufe von mehr als (1/bremslichtschnell) = EIN
    setCVwert(CVbremsnachleuchtstand, CVdefaultbremsnachleuchtstand);     //Zeit die das Bremslicht im Stand anbleibt (Wert * 10 = 2550ms)
    setCVwert(CVbremsnachleuchtfahrt, CVdefaultbremsnachleuchtfahrt);      //Zeit die das Bremslicht während der Fahrt anbleibt (Wert * 10 = 800ms)
    setCVwert(CVrueckhelligkeit, CVdefaultrueckhelligkeit);     //Dimmen des Ausgang Rücklicht
    setCVwert(CVbremshelligkeit, CVdefaultbremshelligkeit);    //Dimmen des Ausgang Bremslicht
    setCVwert(CVbinkhelligkeit, CVdefaultbinkhelligkeit);    //Dimmen des Ausgang Blinklicht
    setCVwert(CVlichthelligkeit, CVdefaultlichthelligkeit);   //Dimmen Ausgang Licht (Abbledlicht vorn)
    setCVwert(CVdatenSleepPause, CVdefaultdatenSleepPause);   //Sleep spart 90% Akku, wenn keine DCC Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
    setCVwert(CVSleepLowTime, CVdefaultSleepLowTime);   //Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)
    //Programmierbare Funktionen zurücksetzten
    setCVwert(CVsirenefunktion, CVdefaultsirenefunktion);  //Zusätzliche Funktion die aktiv sein muss für die Sirene (zB. Blaulicht)
    setCVwert(CVrlichtfunktion, CVdefaultrlichtfunktion);  //Auf welche Funktionstasten reagiert wird. (bitweise F11 bis F4).
    setCVwert(CVaux1funktion, CVdefaultaux1funktion);  
    setCVwert(CVaux2funktion, CVdefaultaux2funktion);  
    setCVwert(CVaux3funktion, CVdefaultaux3funktion);  
    setCVwert(CVaux4funktion, CVdefaultaux4funktion);  
    setCVwert(CVaux5funktion, CVdefaultaux5funktion);  
    setCVwert(CVaux6funktion, CVdefaultaux6funktion);  
    setCVwert(CVArtFunktionA, CVdefaultArtFunktionA);
    setCVwert(CVArtFunktionB, CVdefaultArtFunktionB);
    setCVwert(CVsireneArt, CVdefaultsireneArt);  //Art der Sirene (2 Bit);
    setCVwert(CVrlichtZeit, CVdefaultrlichtZeit);   //Geschwindigkeit des Rundumlicht
    setCVwert(CVaux1blitz, CVdefaultaux1blitz);  //Blitzlänge aux1
    setCVwert(CVaux2blitz, CVdefaultaux2blitz);  //Blitzlänge aux2
    setCVwert(CVaux3blitz, CVdefaultaux3blitz);  //Blitzlänge aux3
    setCVwert(CVaux4blitz, CVdefaultaux4blitz);    //Blitzlänge aux4
    setCVwert(CVaux5blitz, CVdefaultaux5blitz);  //Blitzlänge aux5
    setCVwert(CVaux6blitz, CVdefaultaux6blitz);   //Blitzlänge aux6
    setCVwert(CVaux1p1, CVdefaultaux1p1);   //Blinkpause1 aux1
    setCVwert(CVaux2p1, CVdefaultaux2p1);    //Blinkpause1 aux2
    setCVwert(CVaux3p1, CVdefaultaux3p1);    //Blinkpause1 aux3
    setCVwert(CVaux4p1, CVdefaultaux4p1);    //Blinkpause1 aux4
    setCVwert(CVaux5p1, CVdefaultaux5p1);      //Blinkpause1 aux5
    setCVwert(CVaux6p1, CVdefaultaux6p1);      //Blinkpause1 aux6
    setCVwert(CVaux1p2, CVdefaultaux6p1);      //Blinkpause2 aux1
    setCVwert(CVaux2p2, CVdefaultaux2p2);      //Blinkpause2 aux2
    setCVwert(CVaux3p2, CVdefaultaux3p2);        //Blinkpause2 aux3
    setCVwert(CVaux4p2, CVdefaultaux4p2);      //Blinkpause2 aux4
    setCVwert(CVaux5p2, CVdefaultaux5p2);     //Blinkpause2 aux5
    setCVwert(CVaux6p2, CVdefaultaux6p2);    //Blinkpause2 aux6
    setCVwert(CVautoSerial, CVdefaultautoSerial);    //Einschaltzeit für Serial beim Start
    
    goto ExitEEPROMwrite;      //Sprung zum Ende dieser Funktion
  }
  
  //Änderung der Adresse:
  if (cvAdr == CVdecadr) {              //Adresse
    if ((cvWert >= CVdecadrmin) && (cvWert <= CVdecadrmax))  //Prüfen ob Rahmen für kurze Adresse passt, sonst gibt es Probleme mit dem Programmieren
      setCVwert(cvAdr, cvWert);      //Speichere neuen Wert im EEPROM
    goto ExitEEPROMwrite;     //Sprung zum Ende dieser Funktion
  }
  if (cvAdr == CVerweiterteadr1) {  //Erweiterte Adresse
    if ((cvWert >= 192) && (cvWert <= 231)) {
      setCVwert(CVdecadr,0);  //löschen der kurzen Adresse
      setCVwert(cvAdr, cvWert);      //Speichere neuen Wert im EEPROM
    }
    goto ExitEEPROMwrite;   //Sprung zum Ende dieser Funktion 
  }
  if (cvAdr == CVerweiterteadr2) {  //Erweiterte Adresse
    setCVwert(CVdecadr,0);  //löschen der kurzen Adresse
    setCVwert(cvAdr, cvWert);      //Speichere neuen Wert im EEPROM
    goto ExitEEPROMwrite;   //Sprung zum Ende dieser Funktion 
  }
  
  //Serielle Kommunikation
  if (cvAdr == CVserial) { //aktiviere die Serielle Datenübertragung.
    setSerial(true);   //alle normalen Funktionen des Dekoder werden abgeschaltet!
    return;    //Einfach nur die Funktion beenden.
  }
  
  //Ab hier nur noch normale Änderungen!
  setCVwert(cvAdr, cvWert);      //Speichere neuen Wert im EEPROM
    
  //Sprungziel:
  ExitEEPROMwrite:            //Sprungmarke zum Verlassen und Aktualisieren.
  getRegister();              //Neusetzten der lokalen Variablen
  setLedBlitz(3,true);      //Aufblitzen der Lichter für erfolgreiche Programmierung
}  //Ende updateeeprom

//****************************************************************  
//Funktion zur Berechnen der langen Adresse aus den CV Werten (0...10239)
int Adrrechnen (int cv17, int cv18) {
  int Adresse = 256;    //cv17 ist im 256 Intervall
  if ((cv17 >= 192) && (cv17 <= 231)) {    //cv17 kann nur in diesem Bereich liegen
    Adresse = (cv17 - 192) * Adresse;  //bestimmen des Adressbereich
  }
  Adresse = Adresse + cv18;  //Aufaddieren des 2. Adressteils
  return Adresse;  //Rückgabe der berechneten Adresse.
}  //Ende lange Adresse berechnen

//PINs Konfigurieren
//****************************************************************  
//Start up Konfiguration laden
void setPinMode() {
  makePinMode(HIGH);  //Ausgänge aktiv setzten.
  attachInterrupt(0, dccdata, RISING);  //ISR für den Dateneingang
  pinMode(IRPowerPin, OUTPUT);      //IR-Empfänger Versorgungsspannung
  digitalWrite(IRPowerPin, LOW);    //IR-Empfänger einschalten
}
//****************************************************************  
//Sleep Konfiguration laden
void clearPinMode() {
  makePinMode(LOW);   //Ausgänge inaktiv setzten.
}
//****************************************************************  
//Setzten je nach Modi der Pin Konfiguration
void makePinMode (int start) {
  int Mode = OUTPUT;  //OUTPUT = OUTPUT (HIGH)
  if (start == LOW)   //OUTPUT = INPUT  (LOW)
    Mode = INPUT;
  if (start == HIGH) pinMode(dccPin, INPUT);    //INPUT - Dateneingang mit IRQ
  
  if (start == HIGH) pinMode(ldrPin, INPUT);    //INPUT - Lichtsensor für Automatiklicht
  if (start == HIGH) pinMode(akkuPin, INPUT);    //INPUT - Akkuspannung
  
//  if (start == HIGH) SoftPWMBegin();
//  else SoftPWMEnd(ALL);
  
  pinMode(SpeedPin, OUTPUT);  //OUT - Motoranschluss
  digitalWrite(SpeedPin, LOW);    //Pin Abschalten
  
  //Ausgänge für Funktionen festlegen:
  if (start == LOW) digitalWrite(lichtPin, LOW);    //Pin Abschalten
  pinMode(lichtPin, Mode);     //OUT - Abblendlicht (vorn)
  if (start == LOW) digitalWrite(F1Pin, LOW);    //Pin Abschalten
  pinMode(F1Pin, Mode);        //OUT - Blinker links
  if (start == LOW) digitalWrite(F2Pin, LOW);    //Pin Abschalten
  pinMode(F2Pin, Mode);        //OUT - Blinker rechts
  
  if (start == LOW) digitalWrite(rueckPin, LOW);    //Pin Abschalten
  pinMode(rueckPin, Mode);     //OUT - Rücklicht und Bremslicht (hinten)
  
  //Ausgänge für Einsatz: (Programmierbar)
  if (start == LOW) digitalWrite(rundlichtpin, LOW);    //Pin Abschalten
  pinMode(rundlichtpin, Mode); //OUT - Port für Rundunmlicht
  if (start == LOW) digitalWrite(aux1, LOW);    //Pin Abschalten
  pinMode(aux1, Mode);           //OUT - Port1 für Blaulicht
  if (start == LOW) digitalWrite(aux2, LOW);    //Pin Abschalten
  pinMode(aux2, Mode);        //OUT - Port2 für Blaulicht
  if (start == LOW) digitalWrite(aux3, LOW);    //Pin Abschalten
  pinMode(aux3, Mode);        //OUT - Port3 für Blaulicht
  if (start == LOW) digitalWrite(aux4, LOW);    //Pin Abschalten
  pinMode(aux4, Mode);        //OUT - Port4 für Blaulicht
  if (start == LOW) digitalWrite(aux5, LOW);    //Pin Abschalten
  pinMode(aux5, Mode);        //OUT - Port5 für Blaulicht
  if (start == LOW) digitalWrite(aux6, LOW);    //Pin Abschalten
  pinMode(aux6, Mode);        //OUT - Port6 für Blaulicht
}

//****************************************************************  
//setzt den Microprozessor in den Sleep Mode
void setsleep() {
  long SleepTime = 0;          //Zeit die der Prozessor schon schläft.
  boolean lowSleep = false;    //wenn aktiv denn 99,8% Energie sparen.
  if (makeSleep == true) {  //Prüfen ob Ruhemodus aktiviert werden soll?
    clearPinMode();      //set all used ports to input to save power here
    setup_watchdog(9);   //Watchdog Timer auf 8 sec. einschalten.
    digitalWrite(IRPowerPin, LOW);    //IR-Empfänger einschalten
    pinMode(IRPowerPin, OUTPUT);      //IR-Empfänger Versorgungsspannung
    cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF here
    
    while (makeSleep == true) {    //in die Sleepschleife gehen.
      SleepTime += 1;    //Zeit die, die CPU schläft (Erhöhung mit WDT 8 sec.)
   
      if ((SleepTime > SleepLowTime) && (lowSleep == false))
        lowSleep = true;      //Tiefschlaf, auch IR OFF schalten
    
      if (lowSleep == true) {  //Tiefschlaf aktivieren?
        digitalWrite(IRPowerPin, HIGH);    //IR-Empfänger abschalten
        pinMode(IRPowerPin, OUTPUT);      //IR-Empfänger Versorgungsspannung
      }
    
      system_sleep();     //Prozessor abschalten!
    
      //Watchdogtimer weckt auf, Prüfen ob Daten vorhanden?
    
      digitalWrite(IRPowerPin, LOW);    //IR-Empfänger einschalten
      delay(50);    //Warten auf INPUT Interrupt
    }
    sbi(ADCSRA,ADEN);         // switch Analog to Digitalconverter ON here
    wdt_disable();            //Watchdog abschalten
    setPinMode();             // set all ports into state before sleep here
  }
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep mode.
     *
     * In the avr/sleep.h file, the call names of these sleep modes are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     */
     
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out 
  
  //wird in setsleep gesetzt um mehr einzusparen.
//  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
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
  //weckt Sleep auf, prüft ob IRQ von IR vorhanden ist.
}

//****************************************************************  
//aktiviere und deaktiviere die serielle Kommunikation
void setSerial(boolean aktive) {
  if (aktive == true) {
    serialEnable = true;        //Starten der seriellen Kommunikation, alle andere anschalten
    Serial.println("Serielle Kommunikation gestartet.");
    if (autoSerial > 0)        //Signalisierung über Abblendlicht
      setLedBlitz(4,false);      //kein DCC vorhanden  
    else setLedBlitz(4,true);        //DCC Verarbeitung
    clearPinMode();           //set all used ports to input = off
    detachInterrupt(0);       //ISR für den Dateneingang = off
    Serial.print("Version ist: ");
    writeSerialDecVersion();
/*  Beim einkommentieren dieser Zeilen läuft der Dekoder nicht!!!!  
    Serial.println("Befehlsübersicht: (alle Befehle enden mit ';')");
    Serial.println("get[zahl]; - Gibt Registerwert der CV aus.");
    Serial.println("[zahl];    - Gibt Registerwert der CV aus.");
    Serial.println("set[zahl]; - Setzten CV mit neuem Wert.");
    Serial.println("akku;      - Gibt alle Daten des Akku aus.");
    Serial.println("ldr;       - Anzeige der Lichteinstellungen.");
    Serial.println("alle;/all; - Alle Registerwerte anzeigen.");
    Serial.println("exit;      - Beenden der Kommunikation.");
    Serial.println("");*/
    
    //LDR Lichtsensor überprüfen
    if ((analogRead(ldrPin)/4) == 0)  //keine Daten vorhanden!
      Serial.println("LDR Lichtsensor defekt, keine Daten!");
  }
  else {
    Serial.println("Serial OFF, serielle Kommunikation beendet.");
    serialEnable = false;        //Alles wieder einschalten, keine serielle Kommunikation
    setPinMode();             // set all ports into state before
    setLedBlitz(2,false);
  }
}

//****************************************************************  
//Serielle Kommunikation mit dem PC
void checkSerial() {
  char Serialin;    //Zeichen das gerade eingelesen wurde.
  boolean befehl = false;    //Ein kompletter Befehl erkannt
  if (Serial.available() > 0) {  //Prüfen ob Daten verfügbar?
    Serialin = Serial.read();    //Einlesen der Seriellen Eingabe
    if ((Serialin >= '0') && (Serialin <= '9')) 
      szahl = szahl * 10 + (Serialin - 48);
    else {
      if (Serialin == ';')    //Suchen nach Befehlsabschluss
        befehl = true;        //Befehl zu Ende, dann auswerten.
      if ((Serialin != ';') && (Serialin != ' '))   //Alle anderen Eingaben speichern
        sbuffer = sbuffer + Serialin;
    }
  }
  if (befehl == true) {  //Datenende, Auswertung des gesendeten Befehl
    if (((szahl >= 1) && (szahl <= 255)) || ((sSetAdr > 0) && (szahl >= 0) && (szahl <= 255))) {    //Mehr als 255 CVs wird es hier nicht geben!
      if (sSetAdr > 0) {    //Set wurde auf eine CV aktiviert!
        Serial.print("Der neue Wert (CV");  
        Serial.print(sSetAdr);    //Ausgabe des gewünschten CV Wert
        Serial.print(") lautet: ");
        Serial.println(szahl);    //Ausgabe des Wertes der in die CV geschrieben wird.
        Serial.println("Bitte warten ...");
        updateRegister((sSetAdr-1), szahl);    //Wert in EEPROM eintragen und Variable aktualisieren.
        sSetAdr = 0;        //Set deaktivieren
        sbuffer = "";    //Eingabe zurücksetzten
        szahl = 0;       //Eingegebene Zahl löschen
        Serial.println(" Fertig!");
        return;
      }
      printSerialCV(szahl);  //Alles über diese CV ausgeben.
      if ((sbuffer == "set") && (szahl != 7)) {    //Setzten eines neuen Wert für eine CV, außer für Dekoderversion
        Serial.println("Bitte einen Wert angeben:");
        sSetAdr = szahl;    //Serial set aktivieren.
      }
      sbuffer = "";    //Eingabe zurücksetzten
      szahl = 0;      //Eingegebene Zahl löschen
      return;
    }
    if (sbuffer == "exit") {    //Ende, keine weiteren Eingaben möglich.
      setSerial(false);   //Beendet die serielle Kommunikation
      sbuffer = "";    //Eingabe zurücksetzten
      szahl = 0;      //Eingegebene Zahl löschen
      return;
    }
    if (sbuffer == "akku") {    //Gibt aktuellen Spannungswert des Akku aus.
      Serial.print("Akkuspannung: ");
      Serial.print(analogRead(akkuPin)/4);      //Vergleichswert ausgeben, nur bis maximal 255 möglich (EEPROM)
      akkuSpannung = analogRead(akkuPin) / 203.0;    //Akkuspannung in Volt umrechnen
      Serial.print(" Digital (0-255) - In Volt: ");
      Serial.println(akkuSpannung);      //Akkuspannung ausgeben
      printSerialCV(28);      //Akkuspannungswarnung Wert ausgeben
      printSerialCV(27);      //Akkuorientierung der Geschwindigkeiten
      sbuffer = "";      //Eingabe zurücksetzten
      szahl = 0;        //Eingegebene Zahl löschen
      return;
    } 
    if (sbuffer == "ldr") {    //Gibt aktuellen helligkeitswert des Helligkeitssensor aus.
      Serial.print("Helligkeit(0-255): ");
      Serial.println((analogRead(ldrPin)/4));    //Helligkeitswert herunterbrechen und ausgeben
      printSerialCV(42);      //Schwellwert für EIN ausgeben
      printSerialCV(44);      //Schwellwert für AUS ausgeben
      sbuffer = "";  //Eingabe zurücksetzten
      szahl = 0;    //Eingegebene Zahl löschen
      return;
    }
    if (sbuffer == "all" || sbuffer == "alle") {    //Gibt alle CV aus.
      for (int i = 1; i <= 5; i++)
        printSerialCV(i);  //Alles über diese CV ausgeben.
      for (int i = 7; i <= 8; i++)
        printSerialCV(i);  //Alles über diese CV ausgeben. 
      for (int i = 16; i <= 17; i++)
        printSerialCV(i);  //Alles über diese CV ausgeben. 
      for (int i = 27; i <= 29; i++)
        printSerialCV(i);  //Alles über diese CV ausgeben.  
      for (int i = 41; i <= 55; i++)
        printSerialCV(i);  //Alles über diese CV ausgeben.  
      for (int i = 59; i <= 88; i++)
        printSerialCV(i);  //Alles über diese CV ausgeben.
      printSerialCV(90);  //Alles über diese CV ausgeben.  
      sbuffer = "";    //Eingabe zurücksetzten
      szahl = 0;       //Eingegebene Zahl löschen
      return;
    }
    Serial.println("Fehler, Zeichen konnten nicht erkannt werden!");
    sbuffer = "";    //Eingabe zurücksetzten
    szahl = 0;       //Eingegebene Zahl löschen
  }
}

//**************************************************************** 
//Ausgabe von einer speziellen CV auf der Console.
void printSerialCV(int wertcv) {
  Serial.print("CV");
  Serial.print(wertcv);      //Gesuchte CV ausgeben
  Serial.print(" = ");
  int wert = getCVwert((wertcv-1));  //CV Wert aus EEPROM lesen.
  Serial.print(wert);    //Wert der CV ausgeben
  Serial.println(getSerialCV(wertcv));     //Art der CV ausgeben (Beschreibung)
}

//**************************************************************** 
//Serial Ausgabe der aufgerufenen CV Konfigurationsvariable
String getSerialCV(int cv) {
  //Beschreibung der gewünschten CV ausgeben.
  if (cv == 1) return " - Dekoderadresse"; 
  if (cv == 2) return " - Startspannung";
  if (cv == 3) return " - Beschleunigungsrate";
  if (cv == 4) return " - Bremsrate";
  if (cv == 5) return " - Maximalspannung";
  if (cv == 7) return " - Dekoder Version (oben)";
  if (cv == 8) return " - Hersteller: Philipp Gahtow";
  if (cv == 16 || cv == 17) return " - lange Dekoderadresse";
  if (cv == 27) return " - Akkuwert fuer Fahreigenschaft";
  if (cv == 28) return " - Akkuwarnung";
  if (cv == 29) return " - Blinkergeschwindigkeit";
  if (cv == 41) return " - Zeit ohne Daten bis Notstop";
  if (cv == 42) return " - Licht EIN Schwellwert";
  if (cv == 43) return " - Licht EIN Schwellwert Delay";
  if (cv == 44) return " - Licht AUS Schwellwert";
  if (cv == 45) return " - Licht AUS Schwellwert Delay";
  if (cv == 46) return " - Bremslicht EIN Fahrstufe";
  if (cv == 47) return " - Bremslicht bei Red. d. Fahrstufe";
  if (cv == 48) return " - Nachleuchten Bremslicht Stand";
  if (cv == 49) return " - Nachleuchten Bremslicht Fahrt";
  if (cv == 50) return " - Dimmen Ruecklicht";
  if (cv == 51) return " - Dimmen Bremslicht";
  if (cv == 52) return " - Dimmen Blinklicht";
  if (cv == 53) return " - Dimmen Abbledlicht vorn";
  if (cv == 54) return " - Zeit ohne Daten bis Sleep (in sec.)";
  if (cv == 55) return " - Zeit ohne Daten bis LowSleep (*8 = sec.)";
  if(cv >= 59 && cv <= 66) return " - Funktion (bitweise F11 bis F4)";
  if(cv >= 67 && cv <= 68) return " - Art der Funktionen";
  if(cv == 69) return " - Tonart der Sirene";
  if(cv == 70) return " - Geschwindigkeit Rundumlicht";
  if (cv >= 71 && cv <= 76) return " - Blitzzeit";
  if (cv >= 77 && cv <= 82) return " - Blinkpause1";
  if (cv >= 83 && cv <= 88) return " - Blinkpause2";
  if (cv == 90) return " - Serial alive";
  return "";
}

//**************************************************************** 
//Serial Ausgabe der aktuellen Decoderversion
void writeSerialDecVersion() {
  Serial.print(decversion1);
  Serial.print(".");
  Serial.print(decversion2);
  Serial.println(decversion3);
}
