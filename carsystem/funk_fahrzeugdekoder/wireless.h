//Datenpaket Struktur für Sendevorgänge
typedef struct {byte typ; byte data; } PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX payload;

//RFM12 Configuartion:
byte NoteIDRFM12 = CVdefaultNoteIDRFM12;    //Note ID
byte NetGroupRFM12 = CVdefaultNetGroupRFM12;  //Network Group
byte MasterIDRFM12 = CVdefaultMasterIDRFM12;  //ID des Master Note

//RFM12 Seldewiederholung
#define rfm12repeat 3    //Abbruch nach x Versuchen
#define rfm12sendinterval 60    //Wiederholungsrate in Millisekunden
int rfm12count = -1;    //Zähler über Sendeversuche, 0 = keine Daten zu senden!

//****************************************************************  
//RFM12 Funkmodul aktivieren
void initFunk() {
    //Node ID = 1 Control nodes
    //Network Group = 33
    rf12_initialize(NoteIDRFM12, RF12_868MHZ, NetGroupRFM12);
    //31 nodeIDs (individually for each device), freq, 250 network groups (same for all devices in the system)
    //Node 0 is used for On/Off Keying (OOK) and node 31 is for broadcast messages  
    rf12_control(0xC605);    //Data Rate =57.471kbps -> http://tools.jeelabs.org/rfm12b.html
    rf12_control(0xC0EB);    //Low Battery Detect 3,3 Volt  
    //leeres Datenpacket anlegen:
    payload.typ = 0x00;
    payload.data = 0x00;
}

//****************************************************************  
void setFunkSleep() {
//  rf12_sleep(RF12_SLEEP); //sleep right away to save power
  rf12_sleep(120); //sleep right away to save power N * 32 ms. 
}

//****************************************************************  
void setFunkWakeUp() {
  rf12_sleep(RF12_WAKEUP);
}

//****************************************************************  
//Datenpaket Typ bestimmen
byte rfm12Command(byte data) {
  return data >> 6;
}

//****************************************************************  
//RFM12 Daten einlesen und auswerten
void rfm12receive() {
  static boolean sendACK = false;
  
  if (rf12_recvDone() && rf12_crc == 0) {  //Daten wurden erkannt und die CRC Summe stimmt. 
  
    //Not-STOP nach einer eingestellten Zeit?  
    dateninZeit = flash;  //Auto kann ohne bedenken weiterfahren. Zeitpunkt des Dateneingang wird festgehalten.
    if ((rf12_hdr & RF12_HDR_MASK) == MasterIDRFM12) {    //Prüfen ob daten von Master Note kommen
       
       if (word(rf12_data[1],rf12_data[0]) == 0 && rf12_len == 3) {  //3 Byte Broadcast Nachricht (Byte0 und Byte1 = 0x00)
          switch (rf12_data[2]) {
            case 0x01:  //keine Gleisspannung
                        setNotStop(); //Broadcast Not Halt 
                        return;  //Nichts zu tun hier!
            case 0x02:  if (getFstate(blinkfuncL) == true && getFstate(blinkfuncR) == true && (sollspeed > 0 || istspeed > 0)) {  //Broadcast GO 
                          setFstate(blinkfuncL, false);  //Blinker links AUS
                          setFstate(blinkfuncR, false);  //Blinker rechts AUS
                        }
                        istspeed = 0;        //langsam anfahren
                         //Gleisspannung aktiv
                        return;  //Nichts zu tun hier!
          }
        }
        if (rf12_len == 1 && rfm12count >= 0) {  //Broadcast Nachricht: ACK received, when needed?
          rfm12count = -1;     //Zahler für Versuche rücksetzten, Zentrale hat Packet erhalten
          return;  //Nichts zu tun hier!
        }
        int Adr = word(rf12_data[1] & 0x3F,rf12_data[0]);    //gesendete Adresse bestimmen
        if (Adr != decAdr) //Ist es meine Adresse?
          return;  //verlassen
        //Eigene Adressdaten auswerten:  
        if (rfm12Command(rf12_data[1]) == 0x00 && rf12_len == 3) {    //Geschwindigkeit und F0 gesendet, 3 Byte Befehl
          sollspeed = rf12_data[2] & B01111111;
          f0 = (rf12_data[2] & B10000000) >> 7;    
        }
        else if (rfm12Command(rf12_data[1]) == 0x01) {   //Funktion F8-F1 gesendet
          func = rf12_data[2];
        }
        else if (rfm12Command(rf12_data[1]) == 0x02) {  //Funktion F16-F9 gesendet
          func2 = rf12_data[2];
        }
        else if (rfm12Command(rf12_data[1]) == 0x03) { //Funktion F28-F17 gesendet
/*          //dyn. Rückmeldung
          if (FbActive == false && bitRead(rf12_data[3], 3) == 1) {    
            FbAdr = (rf12_data[2]+1) * 8;        //Bei Aktivierung Startadresse F17-F24 übernehmen
            Fbval = 0;      //Motorzähler rücksetzten!
          }
          else {    */  //Normales schalten der Funktionen
            func3 = rf12_data[2];
            if (rf12_len > 3)
              func4 = rf12_data[3] & B00001111;
            else func4 = 0x00;
/*          }
          if (bitRead(rf12_data[3], 3) == 1)
            FbActive = true;     //F28 Zustand setzten   
          else FbActive = false;     //deaktivieren
*/          
        }
        else if (rfm12Command(rf12_data[1]) == 0x00 && rf12_len == 4) {    //PT lesen, 4 Byte Befehl
          payload.typ = 0x00;
          payload.data = getCVwert( (rf12_data[2] << 8) | rf12_data[3] );  //Rückgabe Wert aus CV
          if (payload.data <= 0xFF)  //Nur senden wenn "gültig"
            rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
          return; //Nichts zu tun hier!  
        }
        else if (rfm12Command(rf12_data[1]) == 0x00 && rf12_len == 5) {    //PT schreiben, 5 Byte Befehl
          if (word(rf12_data[2],rf12_data[3]) == CVreset)    //Schreiben auf CVreset fürhrt zum Rücksetzten aller CV!
            setdefaultCVRegister();
          setCVwert( (rf12_data[2] << 8) | rf12_data[3] , rf12_data[4]);    //Wert überschreiben
          //Rückantwort und Daten schreiben
          payload.typ = 0x00; 
          payload.data = getCVwert( (rf12_data[2] << 8) | rf12_data[3] );  //Rückgabe Wert aus CV
          if (payload.data <= 0xFF)  //Nur senden wenn "gültig" (ungültig = 256)
            rfm12count = rfm12repeat; //Sendebereit - Senden im nächsten Durchlauf!
          clearPinMode();  //Ausgange zurücksetzten  
          getRegister();  //Einlesen der CV Konfigurationswerte
          return; //Nichts zu tun hier!
        }
      //  sendACK = true;
    }
  }
  if (sendACK == true && rf12_canSend()) {  //Send ACK:
        delay(1);
        rf12_sendStart(MasterIDRFM12, 0x00, 1);
        sendACK = false;
  }
}

//****************************************************************  
//RFM12 Daten senden
void rfm12send() {
  if (rfm12count > 0) {
    if (rf12_canSend()) {
      
/*  int i = 0; 
  while (!rf12_canSend() && i<30) {rfm12receive(); i++;}  */      
      delay(1);
      rf12_sendStart(MasterIDRFM12, &payload, sizeof payload);  //OK
      rfm12count--;    //erster Sendeversuch
    }
  }
}
