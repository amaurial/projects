/*
  RCMC Car System Zentrale
  
  Steuert die Kommunikation zwischen der Computeranwendung (P50X)
  und der Fahrzeuge (RCMC) sowie der Fahrstraße (LocoNet).

  Autor: Philipp Gahtow
  Jahr: August, 2013
*/
#include <EEPROM.h>

#include <LocoNet.h>

#include <JeeLib.h>     //RFM12B Funkmodul

#include <p50x.h> 
p50xClass p50x;

#define CarID 1   //ID der CarDekoder
#define SenderID 15  //ID des Funk Sender 
#define GroupID 33  //Gruppe in der die Daten gesendet werden

#define debug true    //Serial Ausgabe EIN/AUS

static  lnMsg        *LnPacket;
static  LnBuf        LnTxBuffer;

byte cts = 6;    //CTS Pin des COM-Ports
byte rfm12led = 10;
byte p50xled = 13;
byte Lnled = 5;

#define repeatUpdate 1500    //Warten zwischen den Aktualisierungen

long rfm12ledtime = 0;
#define LokSlotUpdateInt 600  //Intervall in den die Lok Pakete wiederholt werden
long Lnledtime = 0;
#define ledontime 500    //Zeit die LED Status signalisiert

typedef struct {uint16_t TAdr; byte Data1; } PayloadTX1;      // create structure - a neat way of packaging data for RF comms
PayloadTX1 emontx1;

typedef struct {uint16_t TAdr; byte Data1, Data2; } PayloadTX2;      // create structure - a neat way of packaging data for RF comms
PayloadTX2 emontx2;

typedef struct {uint16_t TAdr; byte Data1, Data2, Data3; } PayloadTX3;      // create structure - a neat way of packaging data for RF comms
PayloadTX3 emontx3;

boolean Railpower = false;    //keine Gleisspannung, Autos inaktiv.

boolean FunkACK = true;  //Erwarten Ack auf Funkübertragung
boolean sendACK = false;  //Empfänger braucht ACK
unsigned long ACKtime = 0;
#define maxACKtime 30

typedef struct	//Arraydatentyp für das Zwischenspeichern und Funkdatenpaketen
{
	uint8_t repeat;		// Sendeversuche
        uint8_t length;         //Länge
        uint16_t TAdr;          //Adresse
	uint8_t data1;		//Datenbyte1
	uint8_t data2;		//Datenbyte2
	uint8_t data3;		//Datenbyte3
} FunkDataFild;

#define maxFunkData 25  //Speicherarray Größe
#define maxRepeat 3
FunkDataFild FunkData[maxFunkData];

byte FunkDataSend = 0;   //Sendeposition
byte FunkDataReq = 0;    //Eintrageposition

void setup()
{
  pinMode(rfm12led, OUTPUT);
  pinMode(p50xled, OUTPUT);
  pinMode(Lnled, OUTPUT);
  
  pinMode(cts, OUTPUT);
  digitalWrite(cts, LOW);
  if (EEPROM.read(6) != 0) 
    EEPROM.write(6,0);  //255 = CTS is unused
    
  // First initialize the LocoNet interface
  LocoNet.init(12);  //PD6
  
  p50x.setup(57600);  //Initializise P50x

  //Node ID = 15 Base Station
  //Network Group = 33
  rf12_initialize(SenderID, RF12_868MHZ, GroupID);  //Initializise RFM12 Funkmodul
  rf12_control(0xC605);    //Data Rate =57.471kbps -> http://tools.jeelabs.org/rfm12b.html

  if (debug)
    Serial.begin(57600);
  
}

void loop()
{  
  p50x.receive();    //Datenempfang P50X
    
//  LNreceive();
//  LocoNet.processSwitchSensorMessage(LocoNet.receive());
  
  rfm12receive();    //Datenempfang RFM12 Funkmodul und LocoNet
  
  unsigned long currentMillis = millis();
  if (currentMillis - rfm12ledtime > LokSlotUpdateInt) {  //wiederholendes Senden von Fahrzeugstati
    digitalWrite(rfm12led, LOW);
    
    if ((millis() - ACKtime) >= repeatUpdate && Railpower) {  //Leerlaufzeit nach letzten Datenpaket
      p50x.xLokStsUpdate();    //Ermitteln der aktiven Fahrzeuge und senden der Daten
//      rfm12sendData(0x02); //Broadcast GO
      Serial.println("Update");
    }
  }
  
  if (currentMillis - Lnledtime > ledontime)
    digitalWrite(Lnled, LOW);
}

//---------------------------------------LocoNet BEGIN---------------------------------------
  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Sensor messages
void notifySensor( uint16_t Address, uint8_t State )
{
  if (Serial && debug) {
    Serial.print("Sensor: ");
    Serial.print(Address, DEC);
    Serial.print(" - ");
    Serial.println( State ? "Active" : "Inactive" );
  }
  p50x.FBsend (Address, State);  //Eintragen
  LNStatusON();  //Status LED
  int data = Address & 0x7FF;    //11 Bit
  bitWrite(data, 4, State ? 1 : 0);
//  if (Railpower) 
//    rfm12send(data);
}
/*
  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch Request messages
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  if (Serial && debug) {
    Serial.print("Switch Request: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "Closed" : "Thrown");
    Serial.print(" - ");
    Serial.println(Output ? "On" : "Off");
  }
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch Report messages
void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  if (Serial && debug) {
    Serial.print("Switch Report: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "Closed" : "Thrown");
    Serial.print(" - ");
    Serial.println(Output ? "On" : "Off");
  }
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch State messages
void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  if (Serial && debug) {
    Serial.print("Switch State: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "Closed" : "Thrown");
    Serial.print(" - ");
    Serial.println(Output ? "On" : "Off");
  }
}

// Rückmeldesonsor Daten senden
boolean LNsendSensor(int Adr, boolean state) {
  //Adressen von 1 bis 4096 akzeptieren
  if (Adr <= 0)        //nur korrekte Adressen
    return false;
  Adr = Adr - 1;    
  int D2 = Adr >> 1;    //Adresse Teil 1 erstellen
  bitWrite(D2,7,0);        //A1 bis A7
    
  int D3 = Adr >> 8;     //Adresse Teil 2 erstellen, A8 bis A11
  bitWrite(D3,4, state);    //Zustand ausgeben
  bitWrite(D3,5,bitRead(Adr,0));    //Adr Bit0 = A0
    
  //Checksum bestimmen:  
  int D4 = 0xFF;        //Invertierung setzten
  D4 = OPC_INPUT_REP ^ D2 ^ D3 ^ D4;  //XOR
  bitWrite(D4,7,0);     //MSB Null setzten
  
  addByteLnBuf( &LnTxBuffer, OPC_INPUT_REP ) ;    //0xB2
  addByteLnBuf( &LnTxBuffer, D2 ) ;    //1. Daten Byte IN2
  addByteLnBuf( &LnTxBuffer, D3 ) ;    //2. Daten Byte IN2
  addByteLnBuf( &LnTxBuffer, D4 ) ;    //Prüfsumme
  addByteLnBuf( &LnTxBuffer, 0xFF ) ;    //Trennbit

        // Check to see if we have received a complete packet yet
  LnPacket = recvLnMsg( &LnTxBuffer );    //Senden vorbereiten
  if(LnPacket ) {        //korrektheit Prüfen
    if (LocoNet.send( LnPacket ) == LN_DONE)  // Send the received packet from the PC to the LocoNet
      return true;
  }
  return false;
}

//LocoNet Paket empfangen
void LNreceive() {
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive();
  if( LnPacket )
  {
    LNStatusON();
       // First print out the packet in HEX
    if (Serial && debug) {       
      Serial.print("RX: ");
      uint8_t msgLen = getLnMsgSize(LnPacket); 
      for (uint8_t x = 0; x < msgLen; x++)
      {
        uint8_t val = LnPacket->data[x];
          // Print a leading 0 if less than 16 to make 2 HEX digits
        if(val < 16)
          Serial.print('0');
          
        Serial.print(val, HEX);
        Serial.print(' ');
      }
    }
    
      // If this packet was not a Switch or Sensor Message then print a new line 
    if(!LocoNet.processSwitchSensorMessage(LnPacket)) {
      if (Serial && debug) 
        Serial.println();
    }
  }
}
*/

//Status LED setzten
void LNStatusON() {
  digitalWrite(Lnled, HIGH);
  Lnledtime = millis();
}
//---------------------------------------LocoNet ENDE---------------------------------------

//------------------------------------------------p50x BEGIN-----------------------------------------

void notifyRS232 (uint8_t State)
{
  digitalWrite(p50xled, State);
}

void notifyTrntRequest( uint16_t Address, uint8_t State, uint8_t Direction, uint8_t Lock )
{
  if (Serial && debug) {
    Serial.print("Switch Request: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "closed" : "thrown");
    Serial.print(" - ");
    Serial.print(State ? "ein" : "aus");
    Serial.print(" - Locked: ");
    Serial.println(Lock ? "On" : "Off");
  }
  LocoNet.requestSwitch(Address, Direction, State);
}

void notifyLokRequest( uint16_t Address, uint8_t Speed, uint8_t Direction, uint8_t F0, uint8_t repeat)
{
  if (Serial && debug) {
    Serial.print("XLok Request - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "vorwaerts" : "rueckwaerts");
    Serial.print(" - Speed: ");
    Serial.print(Speed, DEC);
    Serial.print(" - lights=");
    Serial.println(F0 ? "on" : "off");
  }
  // Speed = XSSS SSSS (7 Bit)
  byte byte1 = Speed;
  bitWrite(byte1, 7, F0);  //Add 1 Bit Funktion 0
  if (Railpower)
    rfm12send(getTAdr ( 0x00, Address), byte1, repeat);
}


void notifyLokF1F4Request( uint16_t Address, uint8_t Function, uint8_t repeat )
{
  if (Serial && debug) {
    Serial.print("Function Request - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(", F4-F1: ");
    Serial.println(Function, BIN);
  }
}


void notifyLokFuncRequest( uint16_t Address, uint8_t Function, uint8_t repeat )
{
  if (Serial && debug) {
    Serial.print("Func Request - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(", F8-F1: ");
    Serial.println(Function, BIN);
  }
  if (Railpower)
    rfm12send(getTAdr ( 0x01, Address), Function, repeat);
}

void notifyLokFunc2Request( uint16_t Address, uint8_t Function2, uint8_t repeat )
{
  if (Serial && debug) {
    Serial.print("Func Request - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(", F16-F9: ");
    Serial.println(Function2, BIN);
  }
  if (Railpower) 
    rfm12send(getTAdr ( 0x02, Address), Function2, repeat);
}

void notifyLokFunc34Request( uint16_t Address, uint8_t Function3, uint8_t Function4, uint8_t repeat )
{
  if (Serial && debug) {
    Serial.print("Func Request - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(", F24-F17: ");
    Serial.print(Function3, BIN);
    Serial.print(", F28-F25: ");
    Serial.println(Function4, BIN);
  }
  if (Railpower) {
    if (Function4 == 0x00) 
      rfm12send(getTAdr ( 0x03, Address), Function3, repeat);    //3 Byte
    else rfm12send(getTAdr ( 0x03, Address), Function3, Function4, repeat);  //4 Byte
  }
}
					

void notifyPowerRequest (uint8_t Power) {
  if (Serial && debug) 
    Serial.println(Power ? "Power ON" : "Power OFF");
  Railpower = Power;  
  if (Railpower) {
    rfm12send(0, 0x02, 4);  //Broadcast GO
//    TIMSK3 |= (1<<TOIE3);  //Timer DCC Start
  }
  else {
//    p50x.xLokStsUpdateClear();    //Update Circle Clear
//    p50x.setHalt();      //Autos stoppen
    rfm12send(0, 0x01, 4);  //Broadcast Not Halt
//    TIMSK3 &= ~(1<<TOIE3);  //Timer DCC STOP
  }
}

void notifyXDCC_PDRRequest( uint16_t Address, uint16_t CVAddress, uint8_t repeat ) {
  if (Serial && debug) {
    Serial.print("XDCC_PDR PoM Read - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(", CV: ");
    Serial.println(CVAddress, DEC);
  }
  int CVAdr = CVAddress - 1;  //0-1023
  rfm12send(getTAdr ( 0x00, Address), highByte(CVAdr), lowByte(CVAdr), repeat);
}

void notifyXDCC_PDRequest( uint16_t Address, uint16_t CVAddress, uint8_t Value, uint8_t repeat ) {
  if (Serial && debug) {
    Serial.print("XDCC_PDR PoM Write - Adr: ");
    Serial.print(Address, DEC);
    Serial.print(", CV: ");
    Serial.print(CVAddress, DEC);
    Serial.print(", Value: ");
    Serial.println(Value, DEC);
  }
  int CVAdr = CVAddress - 1;  //0-1023
  rfm12send(getTAdr ( 0x00, Address), highByte(CVAdr), lowByte(CVAdr), Value, repeat);
}
//------------------------------------------------p50x ENDE-----------------------------------------

//------------------------------------------------rfm12 BEGIN-----------------------------------------

void rfm12sendSave (byte l, uint16_t TAdr, byte d1, byte d2, byte d3, byte repeat) {
  static int lastReq = 0;
  
  if (lastReq != FunkDataReq && TAdr == FunkData[lastReq].TAdr && d1 == FunkData[lastReq].data1) {
       if (d2 == FunkData[lastReq].data2 && d3 == FunkData[lastReq].data3 && TAdr > 0) {
           Serial.println("DOPPELT!");
           return;
       } 
  }

  FunkData[FunkDataReq].repeat = repeat;
  FunkData[FunkDataReq].length = l;
  FunkData[FunkDataReq].TAdr = TAdr;
  FunkData[FunkDataReq].data1 = d1;
  FunkData[FunkDataReq].data2 = d2;
  FunkData[FunkDataReq].data3 = d3;
  if (lastReq != FunkDataReq)
   lastReq++;
  FunkDataReq++;
  if (FunkDataReq >= maxFunkData)
    FunkDataReq = 0;
  if (lastReq >= maxFunkData)
    lastReq = 0;  
}

void rfm12send (uint16_t  data, byte repeat) {
   rfm12sendSave(0,data,0,0,0,repeat);
}
void rfm12send (uint16_t TAdr, byte byte1, byte repeat) {
  rfm12sendSave(1,TAdr,byte1,0,0,repeat);
}
void rfm12send (uint16_t TAdr, byte byte1, byte byte2, byte repeat) {
  rfm12sendSave(2,TAdr,byte1,byte2,0,repeat);
}
void rfm12send (uint16_t TAdr, byte byte1, byte byte2, byte byte3, byte repeat) {
  rfm12sendSave(3,TAdr,byte1,byte2,byte3,repeat);
}

//Adresse mit Datentyp erstellen als 16 Bit Wert
uint16_t getTAdr (byte Type, uint16_t Address)
//  2 Bit Datentyp + 14 Bit Adresse (1 .. 10239 <> 2^14  = 16384)
//  TT AA AA AA AA  AA AA AA AA
{
  uint16_t TAdr = Address;
  bitWrite(TAdr, 14, bitRead( Type, 0));
  bitWrite(TAdr, 15, bitRead( Type, 1));
  return TAdr;
}

void rfm12sendData (uint16_t  data) {    //Rückmelderereignis
  rfm12StatusON();
//  int i = 0; while (!rf12_canSend() && i<20) {rfm12receive(); /*rf12_recvDone(); */ i++;}
  rf12_sendStart(CarID, &data, sizeof data);
}
//Senden der Daten über Funk
void rfm12sendData (uint16_t TAdr, byte byte1) {
  rfm12StatusON();
  emontx1.TAdr = TAdr;
  emontx1.Data1 = byte1;
//  int i = 0; while (!rf12_canSend() && i<20) {rfm12receive(); /*rf12_recvDone(); */ i++;}
  rf12_sendStart(CarID, &emontx1, sizeof emontx1);
}
void rfm12sendData (uint16_t TAdr, byte byte1, byte byte2) {
  rfm12StatusON();
  emontx2.TAdr = TAdr;
  emontx2.Data1 = byte1;
  emontx2.Data2 = byte2;
//  int i = 0; while (!rf12_canSend() && i<20) {rfm12receive(); /*rf12_recvDone(); */ i++;}
  rf12_sendStart(CarID, &emontx2, sizeof emontx2);
}
void rfm12sendData (uint16_t TAdr, byte byte1, byte byte2, byte byte3) {
  rfm12StatusON();
  emontx3.TAdr = TAdr;
  emontx3.Data1 = byte1;
  emontx3.Data2 = byte2;
  emontx3.Data3 = byte3;
//  int i = 0; while (!rf12_canSend() && i<20) {rfm12receive(); /*rf12_recvDone(); */ i++;}
  rf12_sendStart(CarID, &emontx3, sizeof emontx3);
}

//Daten Senden und Wiederholt senden
void rfm12ACK(boolean ACK) {
  if (FunkDataSend != FunkDataReq) {
    if (FunkACK == false) {
      if (ACK == true) {
        Serial.print("ACK OK ");
        Serial.println(millis() - ACKtime);
        FunkACK = true;
        FunkDataSend++;
        if (FunkDataSend >= maxFunkData)
          FunkDataSend = 0;
      }
      else {
        if ((millis() - ACKtime) >= maxACKtime) {
          FunkACK = true;
          Serial.print(millis() - ACKtime);
          Serial.println(" - Repeat Send!");
        }
      }
    }
    else {
      if (FunkData[FunkDataSend].repeat > 0) {
        if (!rf12_canSend())
          return;
        switch (FunkData[FunkDataSend].length) {
          case 0: rfm12sendData(FunkData[FunkDataSend].TAdr); break; 
          case 1: rfm12sendData(FunkData[FunkDataSend].TAdr, FunkData[FunkDataSend].data1); break;
          case 2: rfm12sendData(FunkData[FunkDataSend].TAdr, FunkData[FunkDataSend].data1, FunkData[FunkDataSend].data2); break;
          case 3: rfm12sendData(FunkData[FunkDataSend].TAdr, FunkData[FunkDataSend].data1, FunkData[FunkDataSend].data2, FunkData[FunkDataSend].data3); break;
        }
        FunkData[FunkDataSend].repeat--;
        FunkACK = false;
        ACKtime = millis();
      }
      else {
        FunkDataSend++;
        FunkACK = true;
        if (FunkDataSend >= maxFunkData)
          FunkDataSend = 0;
      }
    }
  }
}

//ACK Senden
void rfm12SendACK() {
  if (sendACK == true && rf12_canSend()) {
    rf12_sendStart(CarID, 0x00, 1); //Bestätigung
    if (Serial && debug)
      Serial.println("Send ACK");
    sendACK = false;  
  }
}

//Empfang von RFM12 Daten
void rfm12receive() {
  boolean receivedACK = false;
  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_MASK) == CarID) {
        rfm12StatusON();
        if (Serial && debug) {
          Serial.print("RX(main): ");
          for (byte i = 0; i < rf12_len; ++i) {
              Serial.print(rf12_data[i], HEX);
              Serial.print(', ');
          }
          Serial.println();
        }
        
        if (rf12_data[0] == 0x00 && rf12_len == 2) {  //2 Byte, PT Return Msg
          p50x.ReturnPT(rf12_data[1]);   //CV Wert weiterleiten
          sendACK = true;
          receivedACK = true;
          if (Serial && debug) {
            Serial.print("Get CV Value: ");
            Serial.println(rf12_data[1]);
          }
        }
        else if ((rf12_data[0] >> 6) == B01 && rf12_len == 2) {  //2 Byte, Rückmelder Msg (ON-OFF)
          boolean State = false;
          if ((rf12_data[0] & B00001000) >> 3 == 1)
            State = true;
          int RAdr = word(rf12_data[0] & B00000111, rf12_data[1]);
          sendACK = true;
          p50x.FBsend (RAdr, State);  //Rückmeldung weiterleiten
          if (Serial && debug) {
            Serial.print("Feedback, Batterie: ");
            Serial.print(RAdr);
            Serial.println(State ? ", ON" : ", OFF"); 
          }
        }
/*        else if ((rf12_data[0] >> 6) == B10 && rf12_len == 2) {  //2 Byte, Rückmelder Msg (AUTO)
          boolean last = (rf12_data[0] & B00001000) >> 3;
          int RAdr = word(rf12_data[0] & B00000111, rf12_data[1]);
          rfm12send(0, 0x00);  //Bestätigung
          if (last == true) {
            if (Serial && debug) {
              Serial.println("Wiederholung letztes!");
//              p50x.FBsend (RAdr-1, true);  //Rückmeldung weiterleiten
              p50x.FBsend (RAdr-1, false);  //Rückmeldung weiterleiten
            }
            
          }
          p50x.FBsend (RAdr-1, false);  //Rückmeldung weiterleiten
          p50x.FBsend (RAdr, true);  //Rückmeldung weiterleiten
          p50x.FBsend (RAdr, false);  //Rückmeldung weiterleiten
          if (Serial && debug) {
             Serial.print("Feedback, AUTO: ");
             Serial.println(RAdr);
          }
        }    */
        else {
          if (rf12_len == 1)  //ACK received
            receivedACK = true;
          else { if (Serial && debug) Serial.println("Unknown"); }
        }
  }
  rfm12ACK(receivedACK);
  rfm12SendACK();   //Bestätigung
  LocoNet.processSwitchSensorMessage(LocoNet.receive());    //prüfe ob Daten vorhanden?
}

//Status LED setzten
void rfm12StatusON() {
  digitalWrite(rfm12led, HIGH);
  rfm12ledtime = millis();    //LED Zeit
}
//------------------------------------------------rfm12 ENDE-----------------------------------------



