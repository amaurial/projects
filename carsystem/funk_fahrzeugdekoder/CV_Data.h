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
#define CVdecadrmax 0xFF     //größte kurze Dekoderadresse
#define CVdefaultdecadr 3   //default Dekorderadresse

#define CVstartspannung 1            //Register enthält Startspannung
#define CVdefaultstartspannung 5        //default Registerwert

#define CVbeschleunigungsrate 2     //Register enthält Beschleunigungsrate
#define CVdefaultbeschleunigungsrate 18    //default Registerwert

#define CVbremsrate 3            //Register enthält Bremsrate
#define CVdefaultbremsrate 5       //default Registerwert

#define CVmaxspannung 4             //Register enthält Maximalspannung
#define CVdefaultmaxspannung 255       //default Registerwert

#define CVmidspannung 5          //Register enthält mittlere Spannung
#define CVdefaultmidspannung 128    //default Registerwert

#define CVdecversion 6             //Register enthält die aktuelle Softwareversion

#define CVreset 7             //Register um beim schreiben einen Reset aller Register auszulösen
#define CVHersteller 7      //Herstellerkennung Dekoder
#define CVdefaultHersteller 0x0D  //Öffentliche und Selbstbaudekoder

#define CVAkkuVolt 8      //MAPPED - Register das die Akkuspannung zurückgibt.
#define CVLiPo 9          //MAPPED - Register Rückmeldung des LiPo Ladereglers

/*  RFM12 Config  */
#define CVNoteIDRFM12 10                //Register für RFM12 Note ID
#define CVdefaultNoteIDRFM12 1         //default Registerwert
#define CVNetGroupRFM12 11                //Register für RFM12 Network Group
#define CVdefaultNetGroupRFM12 33         //default Registerwert

#define CVMasterIDRFM12 12                //Register für RFM12 Master Note ID
#define CVdefaultMasterIDRFM12 15         //default Registerwert

#define CVfreqRFM12 13        //Register für RFM12 Frequenz (1 = 433, 2 = 868, 3 = 915 Mhz)
#define CVdefaultfreqRFM12 2       //Defaultwert der Funkfrequenz

#define CVrepeatRFM12 14         //Register für RFM12 Seldewiederholung
#define CVdefaultrepeatRFM12 3   //Defaultwert, Abbruch nach x Versuchen
#define CVsendintRFM12 15        //Register für RFM12 Wiederholungsintervallrate
#define CVdefaultsendintRFM12 60   //Defaultwert, Wiederholungsrate in Millisekunden
/*   RFM12 Config ende */

#define CVerweiterteadrhigh 16            //Register enthält lange Teil 1 der Adresse  (1 ... 10239)
#define CVerweiterteadrlow 17            //Register enthält lange Teil 2 der Adresse
#define CVdefaulterweiterteadr 0x0000           //default Registerwert

/*--------------------------------------------------------------------------*/

#define CVAkkuwarnadrhigh 19      //Register 3Bit Akkuleer Warnung Rückmeldeadresse
#define CVAkkuwarnadrlow 20        //Register Akkuleer Warnung Rückmeldeadresse (1..2048)
#define CVdefaultAkkuwarnadr 40    //default Registerwert

/*--------------------------------------------------------------------------*/
/*#define CVFbIntervall 21        //Register Anpassung Reifengröße für Rückmeldung
#define CVdefaultFbIntervall 100    //default Registerwert
*/
/*---------------------------Blinker-----------------------------------------------*/
#define CVblinkAkkuleer 28            //Register Vergleichswert für Akkutest ca 3 Volt
#define CVdefaultblinkAkkuleer 160        //default Registerwert

#define CVblinktaktON 29             //Register Blinkergeschwindigkeit ON Zeit
#define CVdefaultblinktaktON 40        //default Registerwert

#define CVblinktaktOFF 30             //Register Blinkergeschwindigkeit OFF Zeit
#define CVdefaultblinktaktOFF 30        //default Registerwert

#define CVblinkfuncL 31             //Register Blinkerfunktion (F0-F28) links (4 Bit)
#define CVdefaultblinkfuncL 0x01        //default Registerwert

#define CVblinkfuncR 32             //Register Blinkerfunktion (F0-F28) rechts (4 Bit)
#define CVdefaultblinkfuncR 0x02        //default Registerwert

#define CVblinkhelligkeitL 33             //Register Dimmen des Ausgang Blinklicht links
#define CVdefaultblinkhelligkeitL 0xFF        //default Registerwert

#define CVblinkhelligkeitR 34             //Register Dimmen des Ausgang Blinklicht rechts
#define CVdefaultblinkhelligkeitR 0xFF        //default Registerwert

#define CVblinkinvertL 35             //Register Invertierung Blinklicht rechts
#define CVdefaultblinkinvertR 0x00        //default Registerwert

#define CVblinkinvertR 36             //Register Invertierung Blinklicht links
#define CVdefaultblinkinvertL 0x00        //default Registerwert

#define CVblinkdimm 37          //Glühbrineneffekt für Blinker, Ein-/Ausdimmen (fade-up time/fade-down time)
#define CVdefaultblinkdimm 0x10        //default Registerwert

#define CVblinkkomfort 38          //Komfortblinken, 0 = AUS, > 0 Mindesanzahl blinken
#define CVdefaultblinkkomfort 0x03       //default Registerwert

/*----------------------------Rücklicht------------------------------------*/
#define CVrueckhelligkeit 39            //Register Dimmen des Ausgang Rücklicht
#define CVdefaultrueckhelligkeit 70       //default Registerwert

#define CVrueckinvert 40            //Register Invertierung Ausgang Rücklicht
#define CVdefaultrueckinvert 0       //default Registerwert

/*----------------------------Bremslicht---------------------------------------*/
#define CVbremshelligkeit 41           //Register Dimmen des Ausgang Bremslicht
#define CVdefaultbremshelligkeit 255        //default Registerwert

#define CVbremslichtschwelle 42            //Register Fahrstufe ab der das Bremslicht sofort EIN geht beim Reduzieren
#define CVdefaultbremslichtschwelle 30       //default Registerwert

#define CVbremslichtschnell 43             //Register Bei Reduzierung der Fahrstufe von mehr als (1/bremslichtschnell) = EIN
#define CVdefaultbremslichtschnell 4        //default Registerwert

#define CVbremsnachleuchtstand 44             //Register Zeit die das Bremslicht im Stand anbleibt (Wert * 10 = 2550ms)
#define CVdefaultbremsnachleuchtstand 255        //default Registerwert

#define CVbremsnachleuchtfahrt 45             //Register Zeit die das Bremslicht während der Fahrt anbleibt (Wert * 10 = 800ms)
#define CVdefaultbremsnachleuchtfahrt 80        //default Registerwert

/*----------------------------Abblendlicht------------------------------------*/
#define CVlichtfunc2 46               //Abblendlicht (vorn), Fernlichtfunktion
#define CVdefaultlichtfunc2 0x04        //default Registerwert

#define CVlichthelligkeit 47             //Register Dimmen Ausgang Licht Funktion F0 (Abbledlicht vorn)
#define CVdefaultlichthelligkeit 90        //default Registerwert

#define CVlichthelligkeit2 48             //Register Dimmen Ausgang Licht Funktion2 (Abbledlicht vorn)
#define CVdefaultlichthelligkeit2 0xFF        //default Registerwert

#define CVlichtinvert 49             //Register Invertierung Abbledlicht (vorn)
#define CVdefaultlichtinvert 0x00        //default Registerwert

/*------------------------------LDR-------------------------------------------*/
#define CVldrfunc 50              //Register für LDR Aktivschaltung
#define CVdefaultldrfunc 0x03        //default Registerwert

#define CVldrlichtein 51             //Register Licht EIN Schwellwert
#define CVdefaultldrlichtein 105        //default Registerwert

#define CVldrdelayein 52            //Register Licht EIN Delay
#define CVdefaultldrdelayein 100       //default Registerwert

#define CVldrlichtaus 53            //Register Licht AUS Schwellwert
#define CVdefaultldrlichtaus 145        //default Registerwert

#define CVldrdelayaus 54            //Register Licht AUS Delay
#define CVdefaultldrdelayaus 200        //default Registerwert

/*--------------------------Sleep Data---------------------------------------------*/
#define CVdateninPause 62            //Register Zeit die verstreichen kann ohne Funk-Daten
#define CVdefaultdateninPause 100       //default Registerwert

#define CVdatenSleepPause 63             //Register Sleep spart 90% Akku, wenn keine Funk Daten vorhanden.  [datenSleepPause * 10 * 100 = ms  (60 = 1 min)]
#define CVdefaultdatenSleepPause 30        //default Registerwert

#define CVSleepLowTime 64             //Register Zeit bis zu vollständigen Abschaltung (WDT 8 sec * SleepLowTime = sec.)
#define CVdefaultSleepLowTime 6       //default Registerwert



/*----------------------------AUX0 bis AUX9----------------------------------------*/
#define CVauxpause0  0            //Link zur 1. Blinkpausenzeit
#define CVdefaultauxpause0  0x00     //default Registerwert

#define CVauxpause1  1            //Link zur 2. Blinkpausenzeit
#define CVdefaultauxpause1  0x00     //default Registerwert

#define CVauxpause2  2            //Link zur 3. Blinkpausenzeit
#define CVdefaultauxpause2  0x00     //default Registerwert

#define CVauxpause3  3            //Link zur 4. Blinkpausenzeit
#define CVdefaultauxpause3  0x00     //default Registerwert

#define CVauxpause4  4            //Link zur 5. Blinkpausenzeit
#define CVdefaultauxpause4  0x00     //default Registerwert

#define CVauxpause5  5            //Link zur 6. Blinkpausenzeit
#define CVdefaultauxpause5  0x00     //default Registerwert

#define CVauxpause6  6            //Link zur 7. Blinkpausenzeit
#define CVdefaultauxpause6  0x00     //default Registerwert

#define CVauxpause7  7            //Link zur 8. Blinkpausenzeit
#define CVdefaultauxpause7  0x00     //default Registerwert

#define CVauxpause8  8            //Link zur 9. Blinkpausenzeit
#define CVdefaultauxpause8  0x00     //default Registerwert

#define CVauxpause9  9            //Link zur 10. Blinkpausenzeit
#define CVdefaultauxpause9  0x00     //default Registerwert

#define CVauxhelligkeit 10       //Link zur Ausgangshelligkeit
#define CVdefaultauxhelligkeit 0xFF       //default Registerwert

#define CVauxfunc1 11       //Link zur Funktion 1
#define CVdefaultauxfunc1 0xFF       //default Registerwert

#define CVauxfunc2 12       //Link zur Funktion 2
#define CVdefaultauxfunc2 0xFF       //default Registerwert

#define CVauxfunc3 13       //Link zur Funktion 3 (AND Funktionsabhängigkeit)
#define CVdefaultauxfunc3 0xFF       //default Registerwert

#define CVauxhelligkeit4 14       //Link zur Ausgangshelligkeit für Zweitfunktion
#define CVdefaultauxhelligkeit4 0xFF       //default Registerwert

#define CVauxfunc4 15       //Link zur Funktion 4 der Zweitfunktion
#define CVdefaultauxfunc4 0xFF       //default Registerwert

#define CVauxinvert 16      //Link zur Invertierung des Ausgangs
#define CVdefaultauxinvert 0x00    //default Registerwert

#define CVauxdimmON 17        //Einblendverzögerung des Ausgangs (zum Beispiel Rundumlicht Glühbirne)  (fade-up time)
#define CVdefaultauxdimmON 0x00        //default Registerwert

#define CVauxdimmOFF 18      //Ausblendverzögerung des Ausgangs (zum Beispiel Rundumlicht, Glühbirne) (fade-down time)
#define CVdefaultauxdimmOFF 0x00        //default Registerwert

/*--------------------------------------------------------------------------*/
/*

#define CVsirenefunktion 58             //Register Zusätzliche Funktion die aktiv sein muss für die Sirene (zB. Blaulicht)
#define CVdefaultsirenefunktion 1        //default Registerwert

#define CVautoSerial 89          //Register für automatisches Prüfen der Serial beim Einschalten
#define CVdefaultautoSerial 50    //default Registerwert
*/

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
//  if (CV == CVserial) return true;    //
  if (CV == CVerweiterteadrlow) return true;    //
  if (CV == CVerweiterteadrhigh) return true;    //
  if (CV == CVblinkAkkuleer) return true;    //

//  if (CV == CVblinktakt) return true;    //
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
//  if (CV == CVbinkhelligkeitR) return true;    //
  if (CV == CVlichthelligkeit) return true;    //
  if (CV == CVdatenSleepPause) return true;    //
  if (CV == CVSleepLowTime) return true;    //54
/*  
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
*/  
  return true;  //kein Register zu der CV vorhanden!
}

//****************************************************************  
//auslesen eines Wertes aus einem Register
int getCVwert (int CV) {
  if (CV == CVAkkuVolt)
    return AkkuValue/20;  //Akkuspannung zurückgegeben
  if (CV == CVLiPo) {
     byte wert = 0;
     bitWrite(wert,0,AkkuFertig);      //1x = Ladung ist beendet
     bitWrite(wert,1,AkkuLadung);      //2x = Akku wird gerade geladen
     bitWrite(wert,2,Akkuwarnsend);    //4x = Akkuwarnung - Akkuwert gering
     bitWrite(wert,3,Akkukritisch);    //8x = Akkuwarnung - Akkuzustand kritisch
     bitWrite(wert,5,digitalRead(chargerPin));  //32x = Zustand LiPo-Charger ohne/mit PullUp
     bitWrite(wert,6,chargerState);  //64x = Zustand LiPo-Charger ohne/mit PullUp
     bitWrite(wert,7,chargerPullUp);  //128x = Zustand PullUp
     return wert;
  }
  int val = EEPROM.read(CV);
  if (isvaluedCV(CV))  //Prüfen ob das Register existiert
    return val;  //Wert auslesen
  return 256;  //Wenn Register nicht vorhanden, Fehler ausgeben!
}

//______________________________________________________________________ 
//Nicht alle CV-Werte dürfen geschrieben werden. Hier wird das geprüft.
boolean isWriteable (int CV, byte Wert) {
  switch (CV) { //Register mit nur Lesezugriff suchen:
    case CVdecversion: return false;    //kein Schreibzugriff
    case CVreset: return false;        //kein Schreibzugriff
    case CVAkkuVolt: return false;     //kein Schreibzugriff
  }
  
  //Register darf geschrieben werden
  if (getCVwert(CV) != Wert)  //Ist dieses Register vorhanden.
    return true;    //Register hat auch unterschiedlichen Wert.
    
  return false;  //Register nicht vorhanden oder unterschiedlicher Wert.
}

//****************************************************************  
//schreiben eines neuen Wertes in das passende Register
void setCVwert (int CV, byte Wert) {
  if (isWriteable(CV, Wert)) { //Prüfen ob Register vorhanden und Wert noch nicht existiert.
    EEPROM.write(CV, Wert);    //Eintragen des neuen Wertes in das Register
  }
}

