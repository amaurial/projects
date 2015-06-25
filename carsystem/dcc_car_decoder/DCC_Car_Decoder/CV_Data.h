/*
  CV_Data.h

  Enthält die Zuweisung der Register zu den Funktionen.
  Zusätzlich wird zu jedem Register ein default Wert angegeben.
  
  Der Lese und Schreibzugriff auf die Register im EEPROM wird hier von
  speziellen Funktionen übernommen. Diese Prüfen vor einem Zufriff,
  ob alles korrekt ist. Für Schreibzugriffe gilt spezielle Vorsicht, 
  da diese auf maximal 100 000 Zyklen pro Chip begrenzt sind!
*/


/*
  ACHTUNG:
  Alle #define CV Werte sind in einem Register niedriger gespeichert. 
  Das stimmt dann wieder mit den Werten die bei der Programmierung gesendet werden überein.
*/

#define CVdecadr 0          //Dekoderadresse
#define CVdecadrmin 1       //kleinste Dekoderadresse
#define CVdecadrmax 111     //größte kurze Dekoderadresse
#define CVdefaultdecadr 3   //default Dekorderadresse

#define CVstartspannung 1            //Register enthält Startspannung
#define CVdefaultstartspannung 5        //default Registerwert

#define CVbeschleunigungsrate 2     //Register enthält Beschleunigungsrate
#define CVdefaultbeschleunigungsrate 18    //default Registerwert

#define CVbremsrate 3            //Register enthält Bremsrate
#define CVdefaultbremsrate 5       //default Registerwert

#define CVmaxspannung 4             //Register enthält Maximalspannung
#define CVdefaultmaxspannung 255       //default Registerwert

#define CVdecversion 6             //Register enthält die aktuelle Softwareversion

#define CVreset 7             //Register um beim schreiben einen Reset aller Register auszulösen

#define CVserial 11          //Register für den Wechsel zur seriellen Kommunikation

#define CVerweiterteadr1 16            //Register enthält lange Teil 1 der Adresse
#define CVdefault 255           //default Registerwert

#define CVerweiterteadr2 17            //Register enthält lange Teil 2 der Adresse
#define CVdefault 255           //default Registerwert

#define CVspeedakkuwert 26  //Register Akkuwert andem sich die CV 2-5 orientieren
#define CVdefaultspeedakkuwert 185  //default Registerwert

#define CVblinkAkkuleer 27            //Register Vergleichswert für Akkutest ca 3 Volt
#define CVdefaultblinkAkkuleer 178        //default Registerwert

#define CVblinktakt 28             //Register Blinkergeschwindigkeit
#define CVdefaultblinktakt 40        //default Registerwert

#define CVdateninPause 40            //Register Zeit die verstreichen kann ohne DCC-Daten
#define CVdefaultdateninPause 50       //default Registerwert

#define CVldrlichtein 41             //Register Licht EIN Schwellwert
#define CVdefaultldrlichtein 105        //default Registerwert

#define CVldrdelayein 42            //Register Licht EIN Delay
#define CVdefaultldrdelayein 100       //default Registerwert

#define CVldrlichtaus 43            //Register Licht AUS Schwellwert
#define CVdefaultldrlichtaus 145        //default Registerwert

#define CVldrdelayaus 44            //Register Licht AUS Delay
#define CVdefaultldrdelayaus 200        //default Registerwert

#define CVbremslichtschwelle 45            //Register Fahrstufe ab der das Bremslicht sofort EIN geht beim Reduzieren
#define CVdefaultbremslichtschwelle 30       //default Registerwert

#define CVbremslichtschnell 46             //Register Bei Reduzierung der Fahrstufe von mehr als (1/bremslichtschnell) = EIN
#define CVdefaultbremslichtschnell 4        //default Registerwert

#define CVbremsnachleuchtstand 47             //Register Zeit die das Bremslicht im Stand anbleibt (Wert * 10 = 2550ms)
#define CVdefaultbremsnachleuchtstand 255        //default Registerwert

#define CVbremsnachleuchtfahrt 48             //Register Zeit die das Bremslicht während der Fahrt anbleibt (Wert * 10 = 800ms)
#define CVdefaultbremsnachleuchtfahrt 80        //default Registerwert

#define CVrueckhelligkeit 49            //Register Dimmen des Ausgang Rücklicht
#define CVdefaultrueckhelligkeit 70       //default Registerwert

#define CVbremshelligkeit 50           //Register Dimmen des Ausgang Bremslicht
#define CVdefaultbremshelligkeit 255        //default Registerwert

#define CVbinkhelligkeit 51             //Register Dimmen des Ausgang Blinklicht
#define CVdefaultbinkhelligkeit 255        //default Registerwert

#define CVlichthelligkeit 52             //Register Dimmen Ausgang Licht (Abbledlicht vorn)
#define CVdefaultlichthelligkeit 110        //default Registerwert

#define CVdatenSleepPause 53             //Register Sleep spart 90% Akku, wenn keine DCC Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
#define CVdefaultdatenSleepPause 20        //default Registerwert

#define CVSleepLowTime 54             //Register Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)
#define CVdefaultSleepLowTime 4        //default Registerwert

#define CVsirenefunktion 58             //Register Zusätzliche Funktion die aktiv sein muss für die Sirene (zB. Blaulicht)
#define CVdefaultsirenefunktion 1        //default Registerwert

#define CVrlichtfunktion 59             //Register Rundumlicht (Auf welche Funktionstasten reagiert wird.)
#define CVdefaultrlichtfunktion 1        //default Registerwert

#define CVaux1funktion 60             //Register
#define CVdefaultaux1funktion 1       //default Registerwert

#define CVaux2funktion 61            //Register
#define CVdefaultaux2funktion 1        //default Registerwert

#define CVaux3funktion 62             //Register
#define CVdefaultaux3funktion 1        //default Registerwert

#define CVaux4funktion 63             //Register
#define CVdefaultaux4funktion 1        //default Registerwert

#define CVaux5funktion 64             //Register
#define CVdefaultaux5funktion 1        //default Registerwert

#define CVaux6funktion 65             //Register
#define CVdefaultaux6funktion 1         //default Registerwert

#define CVArtFunktionA 66             //Register mit den Funktionsraten von aux1 bis aux4
#define CVdefaultArtFunktionA 120        //default Registerwert

#define CVArtFunktionB 67             //Register mit Funktionsarten aux5 bis aux6 und Blinkerinvertierung & Blinkwarnung bei akkuleer
#define CVdefaultArtFunktionB 7        //default Registerwert

#define CVsireneArt 68             //Register Art der Sirene (2 Bit)
#define CVdefaultsireneArt 3        //default Registerwert

#define CVrlichtZeit 69            //Register Geschwindigkeit des Rundumlicht
#define CVdefaultrlichtZeit 40        //default Registerwert

#define CVaux1blitz 70             //Register Blitzlänge aux1
#define CVdefaultaux1blitz 1        //default Registerwert

#define CVaux2blitz 71             //Register Blitzlänge aux2
#define CVdefaultaux2blitz 3        //default Registerwert

#define CVaux3blitz 72             //Register Blitzlänge aux3
#define CVdefaultaux3blitz 2        //default Registerwert

#define CVaux4blitz 73             //Register Blitzlänge aux4
#define CVdefaultaux4blitz 2        //default Registerwert

#define CVaux5blitz 74             //Register Blitzlänge aux5
#define CVdefaultaux5blitz 3        //default Registerwert

#define CVaux6blitz 75             //Register Blitzlänge aux6
#define CVdefaultaux6blitz 20        //default Registerwert

#define CVaux1p1 76            //Register Blinkpause1 aux1
#define CVdefaultaux1p1 6        //default Registerwert

#define CVaux2p1 77             //Register Blinkpause1 aux2
#define CVdefaultaux2p1 6        //default Registerwert

#define CVaux3p1 78            //Register Blinkpause1 aux3
#define CVdefaultaux3p1 5        //default Registerwert

#define CVaux4p1 79             //Register Blinkpause1 aux4
#define CVdefaultaux4p1 15        //default Registerwert

#define CVaux5p1 80             //Register Blinkpause1 aux5
#define CVdefaultaux5p1 6        //default Registerwert

#define CVaux6p1 81             //Register Blinkpause1 aux6
#define CVdefaultaux6p1 20        //default Registerwert

#define CVaux1p2 82            //Register Blinkpause2 aux1
#define CVdefaultaux1p2 2        //default Registerwert

#define CVaux2p2 83             //Register Blinkpause2 aux2
#define CVdefaultaux2p2 40        //default Registerwert

#define CVaux3p2 84            //Register Blinkpause2 aux3
#define CVdefaultaux3p2 30        //default Registerwert

#define CVaux4p2 85             //Register Blinkpause2 aux4
#define CVdefaultaux4p2 30        //default Registerwert

#define CVaux5p2 86             //Register Blinkpause2 aux5
#define CVdefaultaux5p2 43        //default Registerwert

#define CVaux6p2 87             //Register Blinkpause2 aux6
#define CVdefaultaux6p2 0        //default Registerwert

#define CVautoSerial 89          //Register für automatisches Prüfen der Serial beim Einschalten
#define CVdefaultautoSerial 50    //default Registerwert

//****************************************************************  
//Prüft ob der übergebende Wert ein gültigen CV Variable entspricht,
//da nicht alle Register belegt sind.
boolean isvaluedCV (int CV) {
  if (CV == CVdecadr) return true;   //kurze Adresse
  if (CV == CVstartspannung) return true;    //
  if (CV == CVbeschleunigungsrate) return true;    //
  if (CV == CVbremsrate) return true;    //
  if (CV == CVmaxspannung) return true;    //
  if (CV == CVdecversion) return true;    //
  if (CV == CVreset) return true;    //
  if (CV == CVserial) return true;    //
  if (CV == CVerweiterteadr1) return true;    //
  if (CV == CVerweiterteadr2) return true;    //
  if (CV == CVspeedakkuwert) return true;    //
  if (CV == CVblinkAkkuleer) return true;    //
  if (CV == CVblinktakt) return true;    //
  if (CV == CVdateninPause) return true;    //
  if (CV == CVldrlichtein) return true;    //
  if (CV == CVldrdelayein) return true;    //
  if (CV == CVldrlichtaus) return true;    //
  if (CV == CVldrdelayaus) return true;    //
  if (CV == CVbremslichtschwelle) return true;    //
  if (CV == CVbremslichtschnell) return true;    //
  if (CV == CVbremsnachleuchtstand) return true;    //
  if (CV == CVbremsnachleuchtfahrt) return true;    //
  if (CV == CVrueckhelligkeit) return true;    //
  if (CV == CVbremshelligkeit) return true;    //
  if (CV == CVbinkhelligkeit) return true;    //
  if (CV == CVlichthelligkeit) return true;    //
  if (CV == CVdatenSleepPause) return true;    //
  if (CV == CVSleepLowTime) return true;    //54
  if (CV == CVsirenefunktion) return true;    //58
  if (CV == CVrlichtfunktion) return true;    //
  if (CV == CVaux1funktion) return true;    //
  if (CV == CVaux2funktion) return true;    //
  if (CV == CVaux3funktion) return true;    //
  if (CV == CVaux4funktion) return true;    //
  if (CV == CVaux5funktion) return true;    //
  if (CV == CVaux6funktion) return true;    //
  if (CV == CVArtFunktionA) return true;    //
  if (CV == CVArtFunktionB) return true;    //
  if (CV == CVsireneArt) return true;    //
  if (CV == CVrlichtZeit) return true;    //
  if (CV == CVaux1blitz) return true;    //
  if (CV == CVaux2blitz) return true;    //
  if (CV == CVaux3blitz) return true;    //
  if (CV == CVaux4blitz) return true;    //
  if (CV == CVaux5blitz) return true;    //
  if (CV == CVaux6blitz) return true;    //
  if (CV == CVaux1p1) return true;    //
  if (CV == CVaux2p1) return true;    //
  if (CV == CVaux3p1) return true;    //
  if (CV == CVaux4p1) return true;    //
  if (CV == CVaux5p1) return true;    //
  if (CV == CVaux6p1) return true;    //
  if (CV == CVaux1p2) return true;    //
  if (CV == CVaux2p2) return true;    //
  if (CV == CVaux3p2) return true;    //
  if (CV == CVaux4p2) return true;    //
  if (CV == CVaux5p2) return true;    //
  if (CV == CVaux6p2) return true;    //
  if (CV == CVautoSerial) return true;  
  
  return false;  //kein Register zu der CV vorhanden!
}

//****************************************************************  
//auslesen eines Wertes aus einem Register
int getCVwert (int CV) {
  if (isvaluedCV(CV))  //Prüfen ob das Register existiert
    return EEPROM.read(CV);  //Wert auslesen
  return 256;  //Wenn Register nicht vorhanden, Fehler ausgeben!
}

//______________________________________________________________________ 
//Nicht alle CV-Werte dürfen geschrieben werden. Hier wird das geprüft.
boolean isWriteable (int CV, int Wert) {
  switch (CV) { //Register mit nur Lesezugriff suchen:
//    case CVdecversion: return false;    //kein Schreibzugriff
    case CVreset: return false;    //kein Schreibzugriff
    case CVserial: return false;    //kein Schreibzugriff
  }
  
  //Register darf geschrieben werden
  if (getCVwert(CV) != Wert)  //Ist dieses Register vorhanden.
    return true;    //Register hat auch unterschiedlichen Wert.
    
  return false;  //Register nicht vorhanden oder unterschiedlicher Wert.
}

//****************************************************************  
//schreiben eines neuen Wertes in das passende Register
void setCVwert (int CV, int Wert) {
  if (isWriteable(CV, Wert))  //Prüfen ob Register vorhanden und Wert noch nicht existiert.
    EEPROM.write(CV, Wert);    //Eintragen des neuen Wertes in das Register
}
