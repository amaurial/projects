
//****************************************************************  
//Serial Zahl einlesen
int SerialReadZahl() {
  unsigned long Zeit;
  char Data[8];
  int i = 0;
  do {
       // Wenn Daten verf�gbar Zeichen in Data schreiben bis 7 Zeichen erreicht oder 0,5 Sekunden Warten nach dem ersten �bertragenen byte
        if (Serial.available()) {       
          Data[i] = Serial.read();
          i++;
          Zeit = millis(); 
        }     
  }
  while (i<7&&(millis()-Zeit) < 500);
  // Abschlie�ende Null f�r g�ltigen String
  Data[i] = 0;    
  return atof(Data);  // Wert von String zu Zahl wandeln
}

//****************************************************************  
//Serielle Kommunikation mit dem PC
boolean checkSerial() {
  char Serialin;    //Zeichen das gerade eingelesen wurde.
  if (Serial.available() > 0) {  //Prüfen ob Daten verfügbar?
    Serialin = Serial.read();    //Einlesen der Seriellen Eingabe
    if (Serialin == 'r') {
      Serial.println("Register RESET - JA?");
      while (!Serial.available()) {}  //wait
      Serialin = Serial.read();    //Einlesen der Seriellen Eingabe
      if (Serialin == 'j') {
        setdefaultCVRegister();  
        Serial.println("Register RESET!");
      }
    }
    if (Serialin == 'o') {     //Overview
      Serial.print("Verion: ");
      Serial.println(getCVwert(CVdecversion));
      Serial.print("Adresse: ");
      Serial.print(getCVwert(CVdecadr));
      Serial.print(" - LOW: ");
      Serial.print(getCVwert(CVerweiterteadrlow));
      Serial.print(" - HIGH: ");
      Serial.println(getCVwert(CVerweiterteadrhigh));
      Serial.print("Note ID: ");
      Serial.println(NoteIDRFM12);    //Note ID
      Serial.print("Network Group: ");
      Serial.println(NetGroupRFM12);   //Network Group
      Serial.print("ID Master Note: ");
      Serial.println(MasterIDRFM12); //ID Master Note
    }
    if (Serialin == 'a') {
      Serial.print("Akku Value: ");
      Serial.print(AkkuValue/4); //Vergleichswert ausgeben, nur bis maximal 255 m�glich (EEPROM)
      Serial.print(" [0-255] = ");
      float akkuSpannung = AkkuValue/200.8;  //Akkuspannung in Volt umrechnen
      Serial.print(akkuSpannung,2); 
      Serial.println(" Volt");
      Serial.print("LiPo Akku: ");
      if (AkkuFertig == true) 
        Serial.print("voll");
      else {
        if (AkkuLadung == false) {
          if ((Akkuwarnsend == true) || (Akkukritisch == true)) {
            if (Akkukritisch == true)
              Serial.print("kritisch ");
            Serial.print("leer");
          }
          else Serial.print("kein Ladegeraet!");
        }  
        else Serial.print("Ladung aktiv!");  
      }
      Serial.println();
      Serial.print("Chip Temperatur: ");
      Serial.print(getTemp(),1);
      Serial.println(" C");
    }
    if (Serialin == 'l') {
      byte ldrval = analogRead(ldrPin);
      if (ldrval == 0)
        Serial.println("LDR nicht vorhanden!");
      else {
        Serial.print("LDR Value: ");
        Serial.println(ldrval/4);
      }
    }
    if (Serialin == 'g') {
      int number = SerialReadZahl();
      if (number > 0 && number <= 1024) {
        Serial.print("GET CV");
        Serial.print(number);
        Serial.print(": ");
        Serial.print(getCVwert(number-1));
        Serial.print(", B");
        Serial.println(getCVwert(number-1), BIN);
      }
      else {
        Serial.print(number);
        Serial.println(" - Nicht gefunden!");
      }
    }
    if (Serialin == 'm') {
      Serial.print("Motor ");
      Serial.print(val/valc);
      Serial.print(" Last, Speed: ");
      Serial.print(MSpeed);
      Serial.print(" MotorSOll: ");
      Serial.println(MotorValue);
    }
  }
}
