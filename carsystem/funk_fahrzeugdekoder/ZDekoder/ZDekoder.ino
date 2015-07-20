/*
  RFM12 Fahrzeugdecoder
  Atmega32U4 - Arduino Leonardo mit 8 MHz

  Serial:
  Über USB via COM interface
  
  Inputs:
  - Lichtsensor PIN 
  - Akkuspannung PIN
  - LiPo Charger PIN
  - Lastregelung PIN  
  
  Outputs:
  - Motor PIN 
  - Abblendlicht (F0) PIN 
  - Blinken rechts (F1) PIN 
  - Blinken links (F2) PIN 
  - Rücklicht mit Bremslicht PIN
  - x10 Funktionsausgang (AUX) 
*/
#define devVersion 11
/*  Versionshirachie:
                     zusätzliche EEPROM Variablen für: 
                     Abschaltbare Lastregelung; Soundansteuerung ( tone(8, 100, 100); noTone(8); );
                     RFM12 Seldewiederholung, Intervall und Frequenz; Akkuwarndely (Zeit zwischen Akkuwarnungen);
                     Pausenzeit für Programmschleifeninterval
                     
  v0.11  18.07.2013  Korrektur im Sleepalgorithmus. Initialisierung hinzugefügt.                   
  v0.10  16.06.2013  Lastregelung optimiert und Rückmeldung für Akku leer/voll hinzugefügt, zusätzlich über CV10 abfragbar
  v0.09  31.05.2013  Sleep-Modus mit WDC angepasst/verbessert und Lastregelung angepasst
  v0.08  28.05.2013  Lastregelung auf Timer3 --> Probleme Akkuspannungsabfrage schwankt
  v0.07  22.05.2013  Serial Abfragen implementiert
  v0.06  09.03.2013  Programmierbare Ausgänge realisiert 
  v0.05  05.03.2013  Blinkeranpassung mit Invertierung und sofort EIN
  v0.02  25.02.2013  Rückmeldung Akkuwarnung per Funk
  v0.01  15.02.2013  erste Funkübertragung mit Decodierung, Debug via Serial
  
  Autor: Philipp Gahtow
*/

//Pinconfiguration
  //Ausgänge
#define lichtPin A1    //Abblendlicht   (Ausgange mit SOFTPWM)
#define BLPin 5     //blinken links   (Ausgange mit SOFTPWM)
#define BRPin 10      //blinken rechts  (Ausgang mit SOFTPWM)
#define rueckPin 13    //Rücklichtlicht + Bremslicht  (Ausgang mit SoftPWM)
#define speedPin A4    //Motoranschluss  (Ausgang mit Timer3)
#define auxmax 10    //Anzahl der Programmierbaren Ausgänge
uint8_t auxPin[auxmax] = {9, 8, 6, 12, 4, 1, 0, 2, 11, A2};     //Programmierbarer Ausgänge (SoftPWM)
/*                   AUX: 0  1  2   3  4  5  6  7  8   9      */
const uint16_t CVaux[] = {99, 119, 139, 159, 179, 199, 219, 239, 259, 279};     //Programmierbarer Ausgang
/*             AUX:       0    1    2    3    4    5    6    7    8    9    */

  //Eingänge:
#define lastPin A3         //Messeingang Lastregelung  
#define ldrPin A0 //A4;     //18 - Helligkeitssensor Eingang
#define akkuPin A5 //A5;    //19 - Akkuspannung wird geprüft, zu niedrig => Warnblinken
#define chargerPin 7        //Ladezustands Meldung LiPo Charger Digital

unsigned int AkkuValue = 0;      //Akkuspannung, regelmäßige Messung
boolean AnalogMessung = false;    //Ist TRUE falls Analogmessung aktiv!

boolean chargerPullUp = false;      //Pull Up des Eingangs Rückmeldung Laderegler LiPo
boolean chargerState = false;      //Zustand des Ladereglers
boolean Akkutoggle = false;      //Blinken der Rücklicher bei Ladung(fertig)
boolean AkkuFertig = false;      //Ladung ist beendet
boolean AkkuLadung = false;      //Akku wird gerade geladen
boolean Akkuwarnsend = false;    //Akkuwarnung - Akkuwert gering
boolean Akkukritisch = false;    //Akkuwarnung - Akkuzustand kritisch

#include <EEPROM.h>      //Speicher für CV Werte
#include "CV_Data.h"     //Enthält Zuweisungen der Register zum Speicherinhalt

//Variablen
  //Decoderadresse
int decAdr = CVdefaultdecadr;    //Adresse auf welcher der Fahrzeugdecoder reagieren soll.

//Status der Ausgänge
#define interval 15          // interval für setAusgänge in milliseconds
int flash = 0;               //globale Blinkvariable für ein, oder mehrere Ausgänge
int Bflash = 0;               //Blinkvariable nur für Blinker links und rechts

//Abblendlicht und Bremslicht
byte lichthelligkeit = CVdefaultlichthelligkeit;  //Helligkeit des Abblendlichts
byte lichthelligkeit2 = CVdefaultlichthelligkeit2;  //Helligkeit des Fernlichts
byte lichtfunc2 = CVdefaultlichtfunc2;            //Funktion für Fernlicht
byte lichtinvert = CVdefaultlichtinvert;    //Invertierung des Abbledlichts
boolean licht = false;      //Abblendlicht EIN/AUS für Bremslichtsteuerung mit PWM
boolean bremslicht = false; //Bremslicht EIN/AUS für Bremslicht mit integriertem Abblendlicht (PWM)

  //Blinker  & Akkuwarnung
byte blinkhelligkeitL = CVdefaultblinkhelligkeitL;    //Helligkeit des Blinklichts links
byte blinkhelligkeitR = CVdefaultblinkhelligkeitR;    //Helligkeit des Blinklichts rechts
byte blinkfuncL = CVdefaultblinkfuncL;            //Funktion von Blinklichts links
byte blinkfuncR = CVdefaultblinkfuncR;            //Funktion von Blinklichts rechts
byte blinktaktON= CVdefaultblinktaktON;          //Wert * 10 = ms Blinkzeit für jeweils EIN
byte blinktaktOFF= CVdefaultblinktaktOFF;          //Wert * 10 = ms Blinkzeit für jeweils AUS
byte blinkAkkuleer = CVdefaultblinkAkkuleer;      //Akkuspannung unter welcher der Decoder die Warnblinkanlage einschaltet. (Wert * 4; bei 1024 = 5 Volt)
boolean blinkinvertL = CVdefaultblinkinvertL;      //Invertierung des Blinklichts links
boolean blinkinvertR = CVdefaultblinkinvertR;      //Invertierung des Blinklichts rechts
byte blinkkomfort = CVdefaultblinkkomfort;        //Komfortblinker 0 = Inaktiv, sonst = Anzahl der Mindesblinkvorgänge
byte blinkanzahl = 0;            //Anzahl der Blinkauslösungen
boolean akkuwarnunginaktiv = false; //Warnblinker bei unterschreiten der eingestellen Warnspannung
boolean blinkzustand = false;  //wenn Blinker aktiv, enthält aktuellen Zustand der Blinkleuchten
#define akkuwarnungdelay 4000      //Zeit bis zur nächsten Akkuwarnung;
int akkudelay = 0;            //Berechnung der Verzögerung bis zur nächsten Akkuwarnung

 //Rücklicht
byte rueckhelligkeit = CVdefaultrueckhelligkeit;    //Rücklichter leuchten gedimmt mit dieser Stärke wenn sie EIN sind
byte rueckinvert = CVdefaultrueckinvert;        //Invertierung Rücklicht
int bremsdelay = 0;          //Berechnung der Verzögerung für die Bremslichter
byte bremshelligkeit = CVdefaultbremshelligkeit;   //Helligkeit des Bremslichtes
byte bremslichtschwelle = CVdefaultbremslichtschwelle;     //Fahrstufe ab der das Bremslicht sofort EIN geht beim Reduzieren
byte bremslichtschnell = CVdefaultbremslichtschnell;       //Bei Reduzierung der Fahrstufe von mehr als (1/bremslichtschnell) = EIN
byte bremsnachleuchtstand = CVdefaultbremsnachleuchtstand;  //Zeit die das Bremslicht im Stand anbleibt (Wert * 10 = 2550ms)
byte bremsnachleuchtfahrt = CVdefaultbremsnachleuchtfahrt;   //Zeit die das Bremslicht während der Fahrt anbleibt (Wert * 10 = 800ms)

  //Motoransteuerung - Fahrdynamik
byte startspannung = CVdefaultstartspannung;      //Spannung, die bei Fahrstufe 1 am Motor anliegt. (0 = 0 Volt und 255 = max. Spannung)
byte beschleunigungsrate = CVdefaultbeschleunigungsrate;    //Zeit bis zum Hochschalten zur nächst höheren Fahrstufe  (CV3 * 0,9 sec / Anz. Fahrstufen)
byte bremsrate = CVdefaultbremsrate;          //Zeit bis zum Herunterschalten zur nächst niedriegern Fahrstufe (CV3 * 0,9 sec / Anz. Fahrstufen)
byte maximalspannung = CVdefaultmaxspannung;    //Spannung die bei höchster Fahrstufe anliegt.  (0 = min. Spannung und 255 = max. Volt)
byte midspannung = CVdefaultmidspannung;      //Mittelspannung für mittlere Motorspeed
byte sollspeed = 0;          //Geschwindigkeit des Motors
byte istspeed = 0;           //aktuelle fahrende Geschwindigkeit
boolean output = HIGH;    //aktueller Zustand des Ausgangs
int MotorValue = 0;        //Angepasster Motorwert auf Min-/Maximalspannung - Optimal zu erreichen SOLL Motorspannung
int MSpeed = 0;            //IST Motorspannung
unsigned long val = 0;    //Umdrehungen des Motors für Lastregelung
byte valc = 0;            //Anzahl der Umdrehungen

/*
//Rückmeldung - Dynamisch
boolean FbActive = false;    //Erzeugung von Rückmeldeereignissen
int FbAdr = 0;               //Aktuelle Adresse für FB-Sensor 
int FbInt = CVdefaultFbIntervall;    //Entfernungsanpassung
unsigned long Fbval = 0;          //Umdrehungen des Motors
*/

  //LDR schwelle für Abblendlicht  (0 = dunkel bis 1024 = hell)
int ldrdelay = 0;            //Berechnung der Verzögerung für an/aus mit LDR  
byte ldrfunc = CVdefaultldrfunc;        //Funktion um LDR aktiv zu schalten
byte ldrlichtein = CVdefaultldrlichtein;      //unter diesem Wert geht das Licht EIN (normal: 470/4)
byte ldrdelayein = CVdefaultldrdelayein;      //Wartezeit unter dem Wert bis Licht EIN geht (Wert * 10 = ms)
byte ldrlichtaus = CVdefaultldrlichtaus;      //über diesem Wert geht das Licht AUS  (normal: 630/4)
byte ldrdelayaus = CVdefaultldrdelayaus;      //Wartezeit über dem Wert bis Licht AUS geht  (Wert * 10 = ms)

  //Zustände der Funktionen
boolean f0 = false;        //Status vom Ausgang F0
byte func = 0x00;      //Status F1 bis F8
byte func2 = 0x00;      //Status F9 bis F16
byte func3 = 0x00;      //Status F17 bis F24
byte func4 = 0x00;      //Status F25 bis F28

//Automatische STOP Funktion - wenn DCC Daten erkannt wurden, kann das Auto weiterfahren.
int dateninZeit = 0;        //Der Zeitpunkt an dem Daten erkannt wurden.
//boolean makeSleep = false;  //Variable die anzeigt ob die CPU im Ruhemodus ist
int dateninPause = CVdefaultdateninPause;      //Zeit die das DCC Signal nicht Empfangen wird bis gestoppt wird. (dateninPause+1 * 10 * 2 = 1001 ms)
int datenSleepPause = CVdefaultdatenSleepPause;   //Sleep spart 90% Akku, wenn keine DCC Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
int SleepLowTime = CVdefaultSleepLowTime;      //Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)

  //Programmierbare Ausgang
typedef struct 
{  
        //interne Variable:
        uint16_t time;   //Zeitzähler Ausgang
        uint8_t count;  //Zähler Zeitzustand
        //CV Konfiguration:
        uint8_t p0;	//Blinkpause 0
	uint8_t p1;	//Blinkpause 1
	uint8_t p2;	//Blinkpause 2
        uint8_t p3;	//Blinkpause 3
        uint8_t p4;	//Blinkpause 4
        uint8_t p5;	//Blinkpause 5
        uint8_t p6;	//Blinkpause 6
        uint8_t p7;	//Blinkpause 7
        uint8_t p8;	//Blinkpause 8
        uint8_t p9;	//Blinkpause 9
	uint8_t helligkeit;	//Ausgangshelligkeit
	uint8_t func;		//Funktion 1 (1..28)
	uint8_t func2;		//oder Funktion 2 (1..28)
	uint8_t func3;		//Funktion 3 (AND Funktionsabhängigkeit) (1..28)
	uint8_t helligkeit4;	//Ausgangshelligkeit für Zweitfunktion
        uint8_t func4;		//Funktion 4 der Zweitfunktion (1..28)
        uint8_t invert;		//nvertierung des Ausgangs
        uint8_t dimmON;		//Einblendverzögerung des Ausgangs
        uint8_t dimmOFF;	//Ausblendverzögerung des Ausgangs
} auxconf;
auxconf auxfunc[auxmax];    //Array mit den Werten für jeden Ausgang

#include <JeeLib.h>      //RFM12 Funkmodul
#include <SoftPWM.h>     //für PWM auf allen Ausgängen (Up to 20 different channels can be created on Timer2)
#include "IntTemp.h"    //Chip Internal Temperatur
#include "sleep.h"      //Konfiguration WDC und Sleep

#include "wireless.h"  //RFM12 Funkübertragung (Funk-Protokoll)
#include "serial.h"     //Debug und Programming over USB CDC-Serial

//****************************************************************  
//System Start routine 
void setup() {
    //1. Referenzspannung
    analogReference(INTERNAL);    //Refernzspannung auf 2,56 Volt stellen
  
    //2. Timer3 Konfigurieren
    SetupTimer3();  //PWM Motor Timer initializieren
    
    //3. Laden der nutzerdefinierte Einstellungen aus Register:
    getRegister();  //Einlesen der CV Konfigurationswerte und Ausgänge setzen
    
    //4. RFM12 initializieren
    initFunk();  
    
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
}

//****************************************************************  
//Programmablauf 
void loop () {
  static unsigned long letztenMillis = 0;      //bestimmt die Aufrufe von setAusgänge
  static unsigned long rfm12sendMillis = 0;    //bestimmt die Wiederholungsrate RFM12Send
  unsigned long currentMillis = millis();    //aktuelle Zeit bestimmen
  if(currentMillis - letztenMillis > interval) {  //delay(10)
    letztenMillis = currentMillis; 
    setausgaenge();    //Schalten der Ausgänge des Dekoders
    checkSerial();     //Serial kommunikation mit dem Nutzer
  }
  rfm12receive();    //RFM12 Daten auswerten und Funktionen setzten
  if (rfm12count == rfm12repeat || (currentMillis - rfm12sendMillis > rfm12sendinterval)) {
    rfm12send();    //Daten senden
    rfm12sendMillis = currentMillis;
  }
//  EnginePWM();    //Lastberechnung Motor
}

//PINs Konfigurieren
//****************************************************************  
//Start up Konfiguration laden
void setPinMode() {
  makePinMode(HIGH);  //Ausgänge aktiv setzten.
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
  
  //Eingänge initialisieren
  pinMode(ldrPin, INPUT);    //INPUT - Lichtsensor für Automatiklicht
  pinMode(akkuPin, INPUT);    //INPUT - Akkuspannung wird geprüft, zu niedrig => Warnblinken
  pinMode(lastPin, INPUT);  //INPUT - Lastregelung Motorsteuerung
  pinMode(chargerPin, INPUT);  //INPUT - Rückmeldung LiPo Laderegler
  
  pinMode(speedPin, Mode);    //OUTPUT - Motor
  
  if (start == HIGH) {
    SoftPWMBegin();     //SoftPWM starten
    setOutputPWM(BLPin, blinkinvertL);     //Blinker links löschen, falls gerade bei HIGHT abgeschaltet
    setOutputPWM(BRPin, blinkinvertR);     //Blinker rechts löschen, falls gerade bei HIGHT abgeschaltet
    SoftPWMSetFadeTime(BRPin, getCVwert(CVblinkdimm) * 2, getCVwert(CVblinkdimm) * 2);    //fade Time Range: 0 to 4000 
    SoftPWMSetFadeTime(BLPin, getCVwert(CVblinkdimm) * 2, getCVwert(CVblinkdimm) * 2);    //fade Time Range: 0 to 4000 
    for (int i = 0; i < auxmax; i++) {
      setOutputPWM(auxPin[i], auxfunc[i].invert); 
      SoftPWMSetFadeTime(auxPin[i], auxfunc[i].dimmON * 5, auxfunc[i].dimmOFF * 5); //fade Time Range: 0 to 4000  
    }
  }
  else {
    SoftPWMEnd(ALL);  //setzte alle angemeldeten Ausgänge auf INPUT
    digitalWrite(BLPin, LOW);
    digitalWrite(BRPin, LOW);
    pinMode(BLPin, Mode);    //Blinker links
    pinMode(BRPin, Mode);    //Blinker rechts
    for (int i = 0; i < auxmax; i++) {
      digitalWrite(auxPin[i], LOW);
      pinMode(auxPin[i], Mode);
    }
  }
}  //ENDE makePinMode


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
  Bflash +=1;      //Taktgeber nur für Blinker links und rechts
  AnalogMessung = true;    //Meldung für Timer das AnalogWert gerade gelesen wird!
  int akkuwertakt = analogRead(akkuPin);  //Akkuspannung messen
  AnalogMessung = false;    //Meldung für Timer das AnalogWert gerade gelesen wird!
  if (akkuwertakt > 400)    //Prüfen des Akkumesswertes
    AkkuValue = akkuwertakt;    //Globalen Wert setzten.
  else Serial.println("Akkuwert zu klein!");
  int akkuwert = AkkuValue / 4;    //lokaler Akku Funktionswert
  
  //______________________________________________________________________  
  //1. Automatisches Anhalten
  //Prüfen ob immernoch Funkdaten gesendet werden?
  if ((AkkuLadung == false || AkkuFertig == false) && istspeed == 0) {
  
    if (diff(dateninZeit,flash) > (dateninPause+1)*6) {  //DCC-Daten wurden in (Zeit < dateninpause+1 * 10 * 6 ms) empfangen, wenn nicht dann Not-STOP
      //Zeit in der DCC Signale erfasst werden müssen ist abgelaufen.
  
      setNotStop();
    }
  
  //______________________________________________________________________  
  //2. Einschlafen Prüfen, ist ausreichend Zeit ohne Daten verstichen?
    if (diff(dateninZeit,flash) > (datenSleepPause+1)*100) {   //Prüfen ob die Zeit ohne Daten abgelaufen ist?
//    makeSleep = true;    //Prozessor in den Ruhemodus setzten.
      setsleep();          //Prüfen ob Decoder abgeschaltet werden kann
  
    }
  }
  
  //______________________________________________________________________ 
  //Akkuwarung Rückmelden
  if (((akkuwert < blinkAkkuleer+10) /*|| (rf12_lowbat() == true) */ )  && (akkuwarnunginaktiv == false)) {  //(blinkAkkuleer * 4) / 2.03 = Volt
    if (diff(akkudelay,flash) > akkuwarnungdelay) {
      payload.typ = B01001000 | (0x07 & getCVwert(CVAkkuwarnadrhigh));
      payload.data = getCVwert(CVAkkuwarnadrlow); //
      rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
      rfm12send();
      akkudelay = flash;    //Zeit bis zur nächsten Warnung (normal)
      Akkuwarnsend = true;
    }
  }
  else {
    akkudelay = flash;    //Zeit bis zur nächsten Warnung (normal)
    if ((akkuwarnunginaktiv == false) && (Akkuwarnsend == true) && (AkkuLadung == true)) {
      Akkuwarnsend = false;
      payload.typ = B01000000 | (0x07 & getCVwert(CVAkkuwarnadrhigh)); 
      payload.data = getCVwert(CVAkkuwarnadrlow); //
      rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
      rfm12send();
    }
  }
  
  if (akkuwert < blinkAkkuleer && Akkuwarnsend == true)  //Akkuwert unter Schwellwert
    Akkukritisch = true;

  //dynamischer Pull-UP für LiPo Charger Ladungserkennung
  static byte akkucount = 2;
  if (0 == flash % 300) { 
    chargerState = digitalRead(chargerPin);
    if (chargerPullUp == true)    //Zustand letzter Messung
      pinMode(chargerPin, INPUT_PULLUP);  ////internen Pull-UP einschalten
    else {
      pinMode(chargerPin, INPUT);  //INPUT - Rückmeldung LiPo Laderegler
      digitalWrite(chargerPin, LOW);    //internen Pull-UP ausschalten
    }
    chargerPullUp = !chargerPullUp;
 
    if (chargerState == LOW && chargerPullUp == true) {    //Akku wird geladen!
      if (akkucount == 0) {
        Akkukritisch = false;
        AkkuLadung = true;
      }
      else akkucount--;
    }
    if (chargerState == HIGH && chargerPullUp == true) {   //Kein Ladegerät verbunden
      akkucount = 2;
      AkkuLadung = false;    //Kein Ladegerät verbunden! 
      if (AkkuFertig == true && sollspeed > 0) {  //Abschaltung Akkuladung beendet Meldung
        AkkuFertig = false;
        if (getCVwert(CVAkkuwarnadrlow) == 0xFF) {    //nächste Adresse, Überlauf verhindern.
          payload.typ = B01000000 | (0x07 & (getCVwert(CVAkkuwarnadrhigh) + 1) );
          payload.data = getCVwert(CVAkkuwarnadrlow); //
        }
        else {
          payload.typ = B01000000 | (0x07 & getCVwert(CVAkkuwarnadrhigh) );
          payload.data = getCVwert(CVAkkuwarnadrlow) + 1; //
        }
        rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
        rfm12send();
      }
    } 
    if (chargerState == HIGH && chargerPullUp == false && (akkuwert > 200)) {    //Akku hat mehr als 4,1 Volt und State = HIGH
      AkkuFertig = true;      //Akkuladung beendet
      if (getCVwert(CVAkkuwarnadrlow) == 0xFF) {    //nächste Adresse, Überlauf verhindern.
        payload.typ = B01001000 | (0x07 & (getCVwert(CVAkkuwarnadrhigh) + 1) );
        payload.data = getCVwert(CVAkkuwarnadrlow); //
      }
      else {
        payload.typ = B01001000 | (0x07 & getCVwert(CVAkkuwarnadrhigh) );
        payload.data = getCVwert(CVAkkuwarnadrlow) + 1; //
      }
      rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
      rfm12send();
    }
  }
  
  //______________________________________________________________________ 
  //3. Blinker ansteuern
  if ((0 == Bflash % blinktaktOFF && blinkzustand == false) || (0 == Bflash % blinktaktON && blinkzustand == true)) {      //Blinkfequenz für Blinker links und rechts
    Bflash = 0;    //Zähler Rücksetzten
    blinkzustand = !blinkzustand;
    if (((getFstate(blinkfuncL) == true) && (getFstate(blinkfuncR) == true)) || ((akkuwert < blinkAkkuleer) && (akkuwarnunginaktiv == false))) {  //Warnblinkanlage ((blinkAkkuleer * 4) / 2.03 = Volt)
      if (blinkzustand == true) {
        setOutputPWM(BLPin, blinkhelligkeitL, blinkinvertL);     //Blinker links setzten
        setOutputPWM(BRPin, blinkhelligkeitR, blinkinvertR);     //Blinker rechts setzten
      }
      else {
        setOutputPWM(BLPin, blinkinvertL);     //Blinker links löschen
        setOutputPWM(BRPin, blinkinvertR);     //Blinker rechts löschen
      }
    }
    else {
      if (getFstate(blinkfuncL) == true) {  //blinken links
        if (blinkzustand == true)
          setOutputPWM(BLPin, blinkhelligkeitL, blinkinvertL);     //Blinker links setzten
        else setOutputPWM(BLPin, blinkinvertL);     //Blinker links löschen
      }
      else {
        if (getFstate(blinkfuncR) == false) { //blinken rechts und links nicht aktiv?
          blinkzustand = false;  //Rücksetzten
          Bflash = blinktaktOFF-1;  //Sofort starten, wenn Funktion aktiv geht!
        }
        setOutputPWM(BLPin, blinkinvertL);     //Blinker links löschen, falls gerade bei HIGHT abgeschaltet
      }
      if (getFstate(blinkfuncR) == true) {  //blinken rechts
        if (blinkzustand == true)
          setOutputPWM(BRPin, blinkhelligkeitR, blinkinvertR);     //Blinker rechts setzten
        else setOutputPWM(BRPin, blinkinvertR);     //Blinker rechts löschen
      }
      else setOutputPWM(BRPin, blinkinvertR);     //Blinker rechts löschen, falls gerade bei HIGHT abgeschaltet
    }
  }
  
  //______________________________________________________________________
  //4. Abblendlicht ansteuern
  if (f0 == true) {  //Abblendlicht ist aktiv.
    if (getFstate(ldrfunc) == true) {  //Lichtautomatik ist aktiv
      AnalogMessung = true;    //Meldung für Timer das AnalogWert gerade gelesen wird!
      int val = analogRead(ldrPin);    // read the input ldrPin
      AnalogMessung = false;    //Meldung für Timer das AnalogWert gerade gelesen wird!
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
      bremslicht = false;   //Bremslicht AUS
 //  else bremslicht = true;    
  }
  else {                    //Bremslicht nachleuchten wärend der Fahrt
    if ((bremsdelay+bremsnachleuchtfahrt) < flash)    //Bremslicht nach 10*80 = 800ms abschalten
      bremslicht = false;   //Bremslicht AUS
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
          bremslicht = true;  //Bremslicht EIN
      }
    }
  }
  if (sollspeed > istspeed) {    //Beschleunigen des Motors
    if (0 == flash % (beschleunigungsrate+1)) { //CV Beschleunigungsrate, diese darf nicht Null sein.
      istspeed += 1;          //Geschwindigkeit leicht erhöhen
      if (sollspeed > 0)      //Bremslicht erst wirklich beim beschleunigen Abschalten nicht vorher!
        bremslicht = false;   //Bremslicht AUS
    }
  }
  
  //Akkuladerückmeldung über Rücklicht
  if (((AkkuLadung == true && 0 == flash % 80) || (AkkuFertig == true && 0 == flash % 10)) && (sollspeed == 0))
    Akkutoggle = !Akkutoggle;
  if (AkkuLadung == false && AkkuFertig == false)  //Meldung abschalten
    Akkutoggle = false;

  //______________________________________________________________________  
  //6. Bremslicht und Abblendlicht setzen
  if (licht == true || Akkutoggle == true) {     //wenn F0 Licht EIN 
    if (bremslicht == true)    //wenn Bremslicht EIN
      setOutputPWM(rueckPin, bremshelligkeit, rueckinvert);   //Bremslicht einschalten
    else setOutputPWM(rueckPin, rueckhelligkeit, rueckinvert);  //gedimmtes Rücklicht
  }
  else {    //wenn F0 Licht AUS
    if (bremslicht == true)    //wenn Bremslicht EIN
      setOutputPWM(rueckPin, bremshelligkeit, rueckinvert);   //Bremslicht einschalten
    else setOutputPWM(rueckPin, rueckinvert);   //Bremslicht ausschalten
  }
  //Fernlicht/Lichthupe
  if (getFstate(lichtfunc2))
    setOutputPWM(lichtPin, lichthelligkeit2, lichtinvert);   //Fernlicht EIN
  else {    //Abblendlicht
    if (licht == true)
      setOutputPWM(lichtPin, lichthelligkeit, lichtinvert);   //Abblendlicht EIN
    else setOutputPWM(lichtPin, lichtinvert);   //Abblendlicht AUS  
  }

  //______________________________________________________________________  
  //7. Motor ansteuern, Berechnung der Motorgeschwindigkeit für 8 Bit (0-255)
  if (istspeed > 0) { // && akkuwert > 167) {    //wenn Motor nicht steht und Akku mehr als 3,3 Volt hat (Wert * 4 / 203 = Volt)
    GoEngine();
    MotorValue = map(istspeed, 1, 127, startspannung, (maximalspannung*4));    //Geschwindigkeit für Motor berechnen
  }
  else StopEngine();
  
  //______________________________________________________________________  
  //8. Ausgänge für Einsatz, Programmierbar:
  for (int i = 0; i < auxmax; i++) {
    if ( ((getFstate(auxfunc[i].func) || getFstate(auxfunc[i].func2)) && getFstate(auxfunc[i].func3)) || ((auxfunc[i].func3 == 0 || auxfunc[i].func3 > 28) && (getFstate(auxfunc[i].func) || getFstate(auxfunc[i].func2))) ) {
      auxfunc[i].time += 1;
      if (0 == (auxfunc[i].time) % (getauxNextTime(i))) {
        if (0 == auxfunc[i].count % 2) {          
          if (getFstate(auxfunc[i].func4))
            setOutputPWM(auxPin[i], auxfunc[i].helligkeit4, auxfunc[i].invert);    //Ausgang ON - Helligkeit2
          else setOutputPWM(auxPin[i], auxfunc[i].helligkeit, auxfunc[i].invert);    //Ausgang ON - Helligkeit
        }
        else setOutputPWM(auxPin[i], auxfunc[i].invert);    //Ausgang OFF
        auxfunc[i].time = 0;  //Zähler Rücksetzten
        auxfunc[i].count++;  //nächste Zeit holen
      }
    }
    else {
      setOutputPWM(auxPin[i], auxfunc[i].invert);    //Ausgang OFF
      auxfunc[i].time = 0;    //Zeitzähler Ausgang
      auxfunc[i].count = 0;    //Zähler Zeitzustand
    }
  }  
/*  
  //______________________________________________________________________  
  //9. Rückmeldung dynamisch mit Motorspeed
  if (FbActive == true) {  //Meldungen erzeugen!
    if (Fbval > (FbInt * 400)) {
      Fbval = 0;
      if (rfm12count >= 0)  //letzte Paket noch nicht fertig!
        payload.typ = B10001000 | (0x07 & highByte(FbAdr));    //vorletzte Wiederholen!
      else payload.typ = B10000000 | (0x07 & highByte(FbAdr));  
      payload.data = lowByte(FbAdr); //
      rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
      rfm12send();
      if (FbAdr < 2000)
        FbAdr++;      //nächste Rückmeldeadresse
    }
  }  */
}

//****************************************************************  
//nächste Zeit des Ausgangs bestimmen
int getauxNextTime(byte aux) {
  int time = auxfunc[aux].p0;
  switch (auxfunc[aux].count) {
    case 1: time = auxfunc[aux].p1; break;
    case 2: time = auxfunc[aux].p2; break;
    case 3: time = auxfunc[aux].p3; break;
    case 4: time = auxfunc[aux].p4; break;
    case 5: time = auxfunc[aux].p5; break;
    case 6: time = auxfunc[aux].p6; break;
    case 7: time = auxfunc[aux].p7; break;
    case 8: time = auxfunc[aux].p8; break;
    case 9: time = auxfunc[aux].p9; break;
    default: auxfunc[aux].count = 0;
  }
  if (time == 0 && auxfunc[aux].p0 > 0) {
    auxfunc[aux].count = 0;    //Zähler Rücksetzten, wenn keine weitere Zeit vorhanden.
    time = auxfunc[aux].p0;
  }
  return time;
}


//****************************************************************  
//Übernehmen der nutzerdefinierten Einstellungen aus dem EEPROM:
//Funktion zum Einlesen der CV Einstellungen beim Start
void getRegister() {  
  if (getCVwert(CVdecversion) != devVersion)    //Versionsnummer setzten.
    EEPROM.write(CVdecversion, devVersion);
  
  //Einlesen der Register und Speichern der Werte in den zugehörigen Variablen
  //______________________________________________________________________  
  //Grundregister Einlesen:
  decAdr = getCVwert(CVdecadr);    //Decoderadresse (kurz)
  if (getCVwert(CVerweiterteadrhigh) == 0xFF && decAdr == 0xFF && getCVwert(CVerweiterteadrlow) == 0xFF) {    //Erster Start des Decoder (EEPROM leer)
    setdefaultCVRegister();    //setzten aller Werte des EEPROM mit den vorhandenen default Werten!
    return;               //Verlassen, da bereits alle Variablen bereits durch "updateeeprom" gesetzt worden.
  }
  if (decAdr == 0) {  //dann lange Adressen verwenden!
    decAdr = 255 + word(getCVwert(CVerweiterteadrhigh) & B00111111, getCVwert(CVerweiterteadrlow));  //Adresse errechnen
  }
  startspannung = getCVwert(CVstartspannung);      //Spannung, die bei Fahrstufe 1 am Motor anliegt. (0 = 0 Volt und 255 = max. Spannung)
  beschleunigungsrate = getCVwert(CVbeschleunigungsrate);    //Zeit bis zum Hochschalten zur nächst höheren Fahrstufe  (CV3 * 0,9 sec / Anz. Fahrstufen)
  bremsrate = getCVwert(CVbremsrate);          //Zeit bis zum Herunterschalten zur nächst niedriegern Fahrstufe (CV3 * 0,9 sec / Anz. Fahrstufen)
  maximalspannung = getCVwert(CVmaxspannung);    //Spannung die bei höchster Fahrstufe anliegt. (0 = min. Spannung und 255 = max. Volt)
  midspannung = getCVwert(CVmidspannung);      //Mittelspannung für mittlere Motorspeed
  if (maximalspannung == 0) {      //Bei einem Wert von Null auf Maximalspannung setzten!
    maximalspannung = 255;
  }
  
//  FbInt = getCVwert(CVFbIntervall);    //Anpassung Rückmeldung Fahrstrecke
  
  //______________________________________________________________________      
  //Blinker:
  blinkAkkuleer = getCVwert(CVblinkAkkuleer);      //Akkuspannung unter welcher der Decoder die Warnblinkanlage einschaltet. (Wert * 4; bei 1024 = 5 Volt)
  blinktaktON = getCVwert(CVblinktaktON);          //Wert * 10 = ms Blinkzeit für jeweils EIN
  blinktaktOFF = getCVwert(CVblinktaktOFF);          //Wert * 10 = ms Blinkzeit für jeweils AUS
  blinkfuncL = getCVwert(CVblinkfuncL);            //Funktion von Blinklichts links
  blinkfuncR = getCVwert(CVblinkfuncR);            //Funktion von Blinklichts rechts
  blinkhelligkeitL = getCVwert(CVblinkhelligkeitL);    //Helligkeit des Blinklichts links
  blinkhelligkeitR = getCVwert(CVblinkhelligkeitR);    //Helligkeit des Blinklichts rechts
  blinkinvertL = getCVwert(CVblinkinvertL);      //Invertierung des Blinklichts links
  blinkinvertR = getCVwert(CVblinkinvertR);      //Invertierung des Blinklichts rechts
  blinkkomfort = getCVwert(CVblinkkomfort);     //Komfortblinker
  
  //______________________________________________________________________    
  //weitere Registerwerter auslesen:
  ldrfunc = getCVwert(CVldrfunc);        //Funktion um LDR aktiv zu schalten
  ldrlichtein = getCVwert(CVldrlichtein);  //Licht EIN Schwellwert
  ldrdelayein = getCVwert(CVldrdelayein);  //Licht EIN Delay
  ldrlichtaus = getCVwert(CVldrlichtaus);  //Licht AUS Schwellwert
  ldrdelayaus = getCVwert(CVldrdelayaus);  //Licht AUS Delay
  
  bremshelligkeit = getCVwert(CVbremshelligkeit);   //Dimmen Ausgang Bremslicht
  bremslichtschwelle = getCVwert(CVbremslichtschwelle);    //Fahrstufe ab der Bremslicht sofort EIN geht
  bremslichtschnell = getCVwert(CVbremslichtschnell);     //Reduzierung der Geschwindigkeit um 1/Wert = EIN
  bremsnachleuchtstand = getCVwert(CVbremsnachleuchtstand);  //Nachleuchten des Bremslicht im Stand
  bremsnachleuchtfahrt = getCVwert(CVbremsnachleuchtfahrt);  //Nachleuchten des Bremslicht während der Fahrt
  rueckhelligkeit = getCVwert(CVrueckhelligkeit);   //Dimmen Ausgang Rücklicht
  rueckinvert = getCVwert(CVrueckinvert);        //Invertierung Rücklicht
  
  lichthelligkeit = getCVwert(CVlichthelligkeit);   //Dimmen Ausgang Licht (Abbledlicht vorn)
  lichtinvert = getCVwert(CVlichtinvert);    //Invertierung des Abbledlichts
  lichthelligkeit2 = getCVwert(CVlichthelligkeit2);  //Helligkeit des Fernlichts
  lichtfunc2 = getCVwert(CVlichtfunc2);            //Funktion für Fernlicht

  //______________________________________________________________________  
  //Automatisches Abschalten/Schlafen
  dateninPause = getCVwert(CVdateninPause);      //Zeit die ohne Daten verstreichen kann bis Not-STOP
  datenSleepPause = getCVwert(CVdatenSleepPause);   //Sleep spart 90% Akku, wenn keine DCC Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
  SleepLowTime = getCVwert(CVSleepLowTime);   //Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)
  
  //______________________________________________________________________  
  //Programmierbare Ausgänge
  for (int i = 0; i < auxmax; i++) {
    auxfunc[i].time = 0;   //Zeitzähler Ausgang
    auxfunc[i].count = 0;  //Zähler Zeitzustand
    auxfunc[i].p0 = getCVwert(CVaux[i]+CVauxpause0);	//Blinkpause 0
    auxfunc[i].p1 = getCVwert(CVaux[i]+CVauxpause1);	//Blinkpause 1
    auxfunc[i].p2 = getCVwert(CVaux[i]+CVauxpause2);	//Blinkpause 2
    auxfunc[i].p3 = getCVwert(CVaux[i]+CVauxpause3);	//Blinkpause 3
    auxfunc[i].p4 = getCVwert(CVaux[i]+CVauxpause4);	//Blinkpause 4
    auxfunc[i].p5 = getCVwert(CVaux[i]+CVauxpause5);	//Blinkpause 5
    auxfunc[i].p6 = getCVwert(CVaux[i]+CVauxpause6);	//Blinkpause 6
    auxfunc[i].p7 = getCVwert(CVaux[i]+CVauxpause7);	//Blinkpause 7
    auxfunc[i].p8 = getCVwert(CVaux[i]+CVauxpause8);	//Blinkpause 8
    auxfunc[i].p9 = getCVwert(CVaux[i]+CVauxpause9);	//Blinkpause 9
    auxfunc[i].helligkeit = getCVwert(CVaux[i]+CVauxhelligkeit);	//Ausgangshelligkeit
    auxfunc[i].func = getCVwert(CVaux[i]+CVauxfunc1);		//Funktion 1 (1..28)
    auxfunc[i].func2 = getCVwert(CVaux[i]+CVauxfunc2);		//oder Funktion 2 (1..28)
    auxfunc[i].func3 = getCVwert(CVaux[i]+CVauxfunc3);		//Funktion 3 (AND Funktionsabhängigkeit) (1..28)
    auxfunc[i].helligkeit4 = getCVwert(CVaux[i]+CVauxhelligkeit4);	//Ausgangshelligkeit für Zweitfunktion
    auxfunc[i].func4 = getCVwert(CVaux[i]+CVauxfunc4);		//Funktion 4 der Zweitfunktion (1..28)
    auxfunc[i].invert = getCVwert(CVaux[i]+CVauxinvert);		//nvertierung des Ausgangs
    auxfunc[i].dimmON = getCVwert(CVaux[i]+CVauxdimmON);		//Einblendverzögerung des Ausgangs
    auxfunc[i].dimmOFF = getCVwert(CVaux[i]+CVauxdimmOFF);	//Ausblendverzögerung des Ausgangs
  }
  
  //Setzten der jeweiligen Ausgänge/Eingänge
  setPinMode();  //Ausgänge aktiv setzten. - Nach Registerlesen!
  
  if (getCVwert(CVNoteIDRFM12) < 32 && getCVwert(CVNoteIDRFM12) > 0 && getCVwert(CVNetGroupRFM12) <= 250 && getCVwert(CVNetGroupRFM12) > 0 && getCVwert(CVMasterIDRFM12) < 32 && getCVwert(CVMasterIDRFM12) > 0) {
    NoteIDRFM12 = getCVwert(CVNoteIDRFM12);    //Note ID
    NetGroupRFM12 = getCVwert(CVNetGroupRFM12);  //Network Group
    MasterIDRFM12 = getCVwert(CVMasterIDRFM12);  //ID des Master Note
  }
  
} //Ende geteeprom


//****************************************************************  
//Funktion zum Rücksetzten auf default Registerwerte
void setdefaultCVRegister() {
  //Reset (Eingabe eines beliebigen Wertes werden die Einstellungen im Auslieferungszustand wieder herstellt)
    /*setzten der Standartwerte im EEPROM
      Vorheriges Prüfen ob der Wert im EEPROM so schon vorhanden ist, dann kein write auf EEPROM durchführen.
      Hierzu ist kein EEPROM read notwendig, da alle Werte zusätzlich in Variablen gespeichert werden.
    */
    setCVwert(CVdecadr, CVdefaultdecadr);      //CV1 = Adresse = 3 normal
    setCVwert(CVerweiterteadrlow, lowByte(CVdefaulterweiterteadr));    //CV17 = Register enthält lange Teil 1 der Adresse
    setCVwert(CVerweiterteadrhigh, highByte(CVdefaulterweiterteadr));  //CV18 = Register enthält lange Teil 2 der Adresse
    setCVwert(CVstartspannung, CVdefaultstartspannung);    //CV2 = Spannung, die bei Fahrstufe 1 am Motor anliegt. (0 = 0 Volt und 255 = max. Spannung)
    setCVwert(CVbeschleunigungsrate, CVdefaultbeschleunigungsrate);   //CV3 = Zeit bis zum Hochschalten zur nächst höheren Fahrstufe  (CV3 * 0,9 sec / Anz. Fahrstufen)
    setCVwert(CVbremsrate, CVdefaultbremsrate);    //CV4 = Zeit bis zum Herunterschalten zur nächst niedriegern Fahrstufe (CV3 * 0,9 sec / Anz. Fahrstufen)
    setCVwert(CVmaxspannung, CVdefaultmaxspannung);     //CV5 = Spannung die bei höchster Fahrstufe anliegt. (0 = min. Spannung und 255 = max. Volt)
    setCVwert(CVmidspannung, CVdefaultmidspannung);      //CV 6 = Mittelspannung für mittlere Motorspeed
    if (getCVwert(CVreset) != CVdefaultHersteller)  setCVwert(CVHersteller, CVdefaultHersteller);  //Herstellerkennung
    
    setCVwert(CVNoteIDRFM12, CVdefaultNoteIDRFM12);    //Note ID
    setCVwert(CVNetGroupRFM12, CVdefaultNetGroupRFM12);  //Network Group
    setCVwert(CVMasterIDRFM12, CVdefaultMasterIDRFM12);  //ID des Master Note
    setCVwert(CVfreqRFM12, CVdefaultfreqRFM12);          //Fequenz des Funkmodul
    setCVwert(CVrepeatRFM12, CVdefaultrepeatRFM12);    //RFM12 Seldewiederholung
    setCVwert(CVsendintRFM12, CVdefaultsendintRFM12);    //RFM12 Wiederholungsintervallrate (ms)
    
    setCVwert(CVAkkuwarnadrhigh, highByte(CVdefaultAkkuwarnadr));    //Register HIGH für Akkuwarn Rückmeldeadresse
    setCVwert(CVAkkuwarnadrlow, lowByte(CVdefaultAkkuwarnadr));    //Register HIGH für Akkuwarn Rückmeldeadresse
    
//    setCVwert(CVFbIntervall, CVdefaultFbIntervall);    //Anpassung Rückmeldung Fahrstrecke
    
    //Blinker:
    setCVwert(CVblinkAkkuleer, CVdefaultblinkAkkuleer); //Register Vergleichswert für Akkutest ca 3 Volt
    setCVwert(CVblinktaktON, CVdefaultblinktaktON);
    setCVwert(CVblinktaktOFF, CVdefaultblinktaktOFF);
    setCVwert(CVblinkfuncR, CVdefaultblinkfuncR);
    setCVwert(CVblinkfuncL, CVdefaultblinkfuncL);
    setCVwert(CVblinkhelligkeitR, CVdefaultblinkhelligkeitR);
    setCVwert(CVblinkhelligkeitL, CVdefaultblinkhelligkeitL);
    setCVwert(CVblinkinvertR, CVdefaultblinkinvertR);
    setCVwert(CVblinkinvertL, CVdefaultblinkinvertL);
    setCVwert(CVblinkdimm, CVdefaultblinkdimm);
    setCVwert(CVblinkkomfort, CVdefaultblinkkomfort);   
    
    //Rücklicht
    setCVwert(CVrueckhelligkeit, CVdefaultrueckhelligkeit);
    setCVwert(CVrueckinvert, CVdefaultrueckinvert);
    
    //Bremslicht
    setCVwert(CVbremshelligkeit, CVdefaultbremshelligkeit);
    setCVwert(CVbremslichtschwelle, CVdefaultbremslichtschwelle);
    setCVwert(CVbremslichtschnell, CVdefaultbremslichtschnell);
    setCVwert(CVbremsnachleuchtstand, CVdefaultbremsnachleuchtstand);
    setCVwert(CVdefaultbremsnachleuchtstand, CVdefaultbremsnachleuchtstand);
    
    //Abblendicht
    setCVwert(CVlichtfunc2, CVdefaultlichtfunc2);
    setCVwert(CVlichthelligkeit, CVdefaultlichthelligkeit);
    setCVwert(CVlichthelligkeit2, CVdefaultlichthelligkeit2);
    setCVwert(CVlichtinvert, CVdefaultlichtinvert);
    
    //LDR
    setCVwert(CVldrfunc, CVdefaultldrfunc);
    setCVwert(CVldrlichtein, CVdefaultldrlichtein);
    setCVwert(CVldrdelayein, CVdefaultldrdelayein);
    setCVwert(CVldrlichtaus, CVdefaultldrlichtaus);
    setCVwert(CVldrdelayaus, CVdefaultldrdelayaus);
    
    //Sleep
    setCVwert(CVdateninPause, CVdefaultdateninPause);
    setCVwert(CVdatenSleepPause, CVdefaultdatenSleepPause);
    setCVwert(CVSleepLowTime, CVdefaultSleepLowTime);
    
    //Programmierbare Ausgänge
    for (int i = 0; i < auxmax; i++) {
      setCVwert(CVaux[i]+CVauxpause0, CVdefaultauxpause0);	//Blinkpause 0
      setCVwert(CVaux[i]+CVauxpause1, CVdefaultauxpause1);	//Blinkpause 1
      setCVwert(CVaux[i]+CVauxpause2, CVdefaultauxpause2);	//Blinkpause 2
      setCVwert(CVaux[i]+CVauxpause3, CVdefaultauxpause3);	//Blinkpause 3
      setCVwert(CVaux[i]+CVauxpause4, CVdefaultauxpause4);	//Blinkpause 4
      setCVwert(CVaux[i]+CVauxpause5, CVdefaultauxpause5);	//Blinkpause 5
      setCVwert(CVaux[i]+CVauxpause6, CVdefaultauxpause6);	//Blinkpause 6
      setCVwert(CVaux[i]+CVauxpause7, CVdefaultauxpause7);	//Blinkpause 7
      setCVwert(CVaux[i]+CVauxpause8, CVdefaultauxpause8);	//Blinkpause 8
      setCVwert(CVaux[i]+CVauxpause9, CVdefaultauxpause9);	//Blinkpause 9
      setCVwert(CVaux[i]+CVauxhelligkeit, CVdefaultauxhelligkeit);	//Ausgangshelligkeit
      setCVwert(CVaux[i]+CVauxfunc1, CVdefaultauxfunc1);		//Funktion 1 (1..28)
      setCVwert(CVaux[i]+CVauxfunc2, CVdefaultauxfunc2);		//oder Funktion 2 (1..28)
      setCVwert(CVaux[i]+CVauxfunc3, CVdefaultauxfunc3);		//Funktion 3 (AND Funktionsabhängigkeit) (1..28)
      setCVwert(CVaux[i]+CVauxhelligkeit4, CVdefaultauxhelligkeit4);	//Ausgangshelligkeit für Zweitfunktion
      setCVwert(CVaux[i]+CVauxfunc4, CVdefaultauxfunc4);		//Funktion 4 der Zweitfunktion (1..28)
      setCVwert(CVaux[i]+CVauxinvert, CVdefaultauxinvert);		//nvertierung des Ausgangs
      setCVwert(CVaux[i]+CVauxdimmON, CVdefaultauxdimmON);		//Einblendverzögerung des Ausgangs
      setCVwert(CVaux[i]+CVauxdimmOFF, CVdefaultauxdimmOFF);	//Ausblendverzögerung des Ausgangs
    }
   
    
  getRegister();              //Neusetzten der lokalen Variablen
}


//****************************************************************  
//*Status der gefragten Funktion F1 bis F28 rückgeben
boolean getFstate (int Funktion)  {
  boolean state = false; //keine Funktion mit dieser Nummer vorhanden!  
  if (Funktion == 0)
    state = false;  //kein F0 rückgeben, Funktion is inaktiv!
  else if (Funktion < 9)
    state = bitRead(func, Funktion - 1);
  else if (Funktion < 17)
    state = bitRead(func2, Funktion - 9);
  else if (Funktion < 25)
    state = bitRead(func3, Funktion - 17);
  else if (Funktion < 29)
    state = bitRead(func4, Funktion - 25);  
  return state;
}

//****************************************************************  
//*Status der Funktion F0 bis F28 überschreiben
void setFstate (int Funktion, boolean state)  {
  if (Funktion == 0)
    f0 = state;
  else if (Funktion < 9)
    bitWrite(func, Funktion - 1, state);
  else if (Funktion < 17)
    bitWrite(func, Funktion - 9, state);
  else if (Funktion < 25)
    bitWrite(func, Funktion - 17, state);
  else if (Funktion < 29)
    bitWrite(func, Funktion - 25, state);
}

//****************************************************************  
//Ausgang setzten mit SoftPWM
void setOutputPWM (int pin, byte Value, boolean invert) {
  if (invert == true)
    SoftPWMSetPolarity(pin, SOFTPWM_INVERTED);
  else SoftPWMSetPolarity(pin, SOFTPWM_NORMAL);
  SoftPWMSet(pin, Value);  
}

void setOutputPWM (int pin, boolean invert) {
  setOutputPWM(pin, LOW, invert);
}


//****************************************************************  
//Motor PWM Timer Setup
void SetupTimer3() {
  float prescaler = 16.0;
  //during setup, disable all the interrupts based on timer3
  TIMSK3 &= ~((1<<TOIE3) | (1<<OCIE3A) | (1<<OCIE3B) | (1<<OCIE3C) | (1<<ICIE3));
    //normal mode: counter incremented until overflow, prescaler set to /1
//    TCCR3A &= ~((1<<WGM31) | (1<<WGM30));
//    TCCR3B &= ~((1<<WGM33) | (1<<WGM32) | (1<<CS32) | (1<<CS31));
//    TCCR3B |= (1<<CS30);
    TCCR3A = 0; 
    TCCR3C = 0; 
//    OCR3C = 0xFF;
    TCCR3B = 0x01;
    
    TCNT3 = 65536 - (uint16_t)((float)F_CPU * 0.001 / prescaler); //for 16 MHz: 49536

    StopEngine();
}

//****************************************************************  
//Not Halt Ausführen, sofortiges anhalten des Fahrzeugs
void setNotStop() {
  if (sollspeed > 0 || istspeed > 0) {  //Prüfen ob Auto fährt
    //Warnblinkanlage einschalten - Funktion für Blinker aktivieren!
    setFstate(blinkfuncL, true);  //Blinker links EIN
    setFstate(blinkfuncR, true);  //Blinker rechts EIN
  }
  //Auto anhalten damit kein Schaden entsteht.
  sollspeed = 0;
  istspeed = 0;
  StopEngine();
}

//****************************************************************  
void StopEngine() {    //Motor Abschalten
  TIMSK3 &= ~(1<<TOIE3);   //PWM Timer Stop
  MotorValue = 0;    //Motor ist aus.
  MSpeed = 0;        //Rücksetzeten Lastregelung
  digitalWrite(speedPin, LOW);  //PWM  (Port PF1)
}

//****************************************************************  
void GoEngine() {    //Motor Einschalten
  TIMSK3 |= (1<<TOIE3);  //Start 
}


//****************************************************************  
//Motor PWM Timer ISR
//Timer3 overflow Interrupt vector handler
ISR(TIMER3_OVF_vect) {
   if (output == LOW) {
     TCNT3 = 65536 - ((MSpeed * 63)+1);
     if (AnalogMessung == false) {    //Motorspannung befor Ausgang aktiv wird bestimmen
       val += analogRead(lastPin);    //Eingelesende Spannung
       valc++;
       if (valc > 2)   //Anzahl der Drehzahlmessungen
         EnginePWM();
     }
     PORTF |= (1 << (1));    //Set Port Bit speedPin (HIGH)   
   }
   else {
     TCNT3 = (MSpeed * 63)+1;  
     PORTF &= ~(1 << (1));    //clear Port Bit speedPin (LOW)
   }
   output = !output;
//   digitalWrite(speedPin, output);  //PWM  (Port PF1)
   
}
//****************************************************************  
//PWM-Lastcontrol des Motors
void EnginePWM () {
    int last = val/valc;    //Motordrehzahl Mittelwert berechnen
    val = 0;
    valc = 0;
    if (last < (MotorValue - 10) && MSpeed < 1024 && sollspeed != 0) {      //schneller (Beschleunigen)
     if (MSpeed < 1015 && (MotorValue - last) > 40 && bremslicht == false) {
       if (MSpeed < 950 && (MotorValue - last) > 70) {
          if ((last < 20) && (MSpeed > 100))    //plötzliche Belastung?
            setNotStop();
          else MSpeed = MSpeed + 2; 
       }
       else MSpeed = MSpeed + 1;
     }
     else MSpeed = MSpeed + 1;
   }
   else {
     if (last  > (MotorValue + 8) && MSpeed > 0) {    //langsamer (Bremsen)
      if (MSpeed > 4 && (last - MotorValue) > 40) {
        if (MSpeed > 20 && (last - MotorValue) > 70) {
          if (istspeed < sollspeed)
            MSpeed = MSpeed - 1;  //2
          else {
            MSpeed = MSpeed - 2;   //5
            if ((last - MotorValue) > 120)
              MSpeed = MSpeed - 1;   //2
          }
        }
        else MSpeed = MSpeed - 2;  //3
      }
      else MSpeed = MSpeed - 1; 
     }
   }
}

//****************************************************************  
//setzt den Microprozessor in den Sleep Mode
void setsleep() {
  long SleepTime = 0;          //Zeit die der Prozessor schon schläft.
  boolean lowSleep = false;    //wenn aktiv denn 99,8% Energie sparen.
//  if (makeSleep == true) {  //Prüfen ob Ruhemodus aktiviert werden soll?
  setNotStop();
  clearPinMode();      //set all used ports to input to save power here
//  wdt_enable(WDTO_8S);    //Watchdog Timer auf 8 sec. einschalten.
  power_adc_disable();    // switch Analog to Digitalconverter OFF here
  power_timer1_disable();
  power_timer3_disable();
  power_timer2_disable();
  power_twi_disable();
  
//  pinMode(chargerPin, INPUT_PULLUP);  ////internen Pull-UP einschalten
         
  while ((dateninZeit != flash)) {    //in die Sleepschleife gehen, Abbruch bei Ladung oder Datenempfang
      SleepTime += 1;    //Zeit die, die CPU schläft (Erhöhung mit WDT 8 sec.)
   
      if (SleepTime > SleepLowTime)
        lowSleep = true;      //Tiefschlaf, auch Funk OFF schalten
    
      if (lowSleep == true) {  //Tiefschlaf aktivieren?
        setFunkSleep();
        power_spi_disable();
        power_timer0_disable();
      }
      
      system_sleep();     //Prozessor abschalten!
      
      //Funkmodultimer weckt auf, Prüfen ob Daten vorhanden?
      
      if (lowSleep == true) {
        power_spi_enable();
        power_timer0_enable();
        setFunkWakeUp();
      }
      int i = 15;
      while (i > 0) {   //Warten ob Funk INPUT Interrupt    
        delay(2);
        rfm12receive();
        if (dateninZeit == flash)
          break;
        i = i - 1;
      }
    }
    
    power_all_enable();
//    wdt_disable();            //Watchdog abschalten
    setPinMode();             // set all ports into state before sleep here
    
//  }
}

