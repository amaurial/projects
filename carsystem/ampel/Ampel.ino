/*
  Ampel
 */
 
const int ampelPin = 2;      //erster Pin
const int ampelSperrPin = 14;    //erster Pin

const int ampelOn = LOW;    //Potential für aktive Ampel
const int sperrON = HIGH;    //Potential für rote Ampel
 
int mode = 0;
long time = 0;

const int ampelmax = 3;    //maximale Anzahl der Ampeln
int ampelstatus[ampelmax];    //aktuelle Anzeige der Ampel

             //rot, rotgelb, grün, gelb
int umschalt[] = {3, 8, 50, 10};    //Zeiten für die Phasen

//Einsatz Aktivierung:
const int einsatzPin = A5;    //Ausgang
boolean einsatzOff = true;      //Einsatzzustand
int einsatztime = 0;    //Zählt alle 250ms
int einsatzmaxtime = 960;    //4 Minuten
int einsatzmintime = 240;     //1 Minute
int einsatzwahr = einsatzmintime + random(einsatzmaxtime-einsatzmintime);

void setup() {                
  for (int i = 0; i < (ampelmax*3); i++) {
    pinMode(ampelPin + i, OUTPUT);     
    digitalWrite(ampelPin + i, !ampelOn);
  }
    
  for (int i = 0; i < ampelmax; i++) {
    ampelstatus[i] = 0;    //rot
    pinMode(ampelSperrPin + i, OUTPUT);
    digitalWrite(ampelSperrPin + i, sperrON);    //alles sperren
  }
  configAmpel();    //rote Ampeln
  
  pinMode(einsatzPin, OUTPUT);
  digitalWrite(einsatzPin, einsatzOff);
}

void loop() {
  delay(250);
  time++;
  makeAmpel();
  configAmpel();
  getEinsatz();
}

void makeAmpel() {
  if (time % umschalt[ampelstatus[mode]] == 0) {
    time = 0;    //Zeit Rücksetzten
    ampelstatus[mode] = ampelstatus[mode] + 1;    //Ampel weiterschalten
    if (ampelstatus[mode] > 3) {    //zur nächsten Ampel
      ampelstatus[mode] = 0;    //Ampel rot
      mode++;
    }
    if (mode >= ampelmax)    //zurück zur ersten Ampel
      mode = 0;
      
  }
}

//Ampel ansteuern
void setAmpel(int firstPin, int s, int lockPin) {
  if (s == 1) {  //rot gelb
    digitalWrite(firstPin, ampelOn);
    digitalWrite(firstPin+1, ampelOn);
    digitalWrite(firstPin+2, !ampelOn);
    digitalWrite(lockPin, !sperrON);      //freigeben
    return;
  }  
  if (s == 2) {  //grün
    digitalWrite(firstPin, !ampelOn);
    digitalWrite(firstPin+1, !ampelOn);
    digitalWrite(firstPin+2, ampelOn);
    digitalWrite(lockPin, !sperrON);      //freigeben
    return;
  }
  if (s == 3) {  //gelb
    digitalWrite(firstPin, !ampelOn);
    digitalWrite(firstPin+1, ampelOn);
    digitalWrite(firstPin+2, !ampelOn);
    digitalWrite(lockPin, sperrON);      //sperren
    return;
  }
  //rot
  digitalWrite(firstPin, ampelOn);
  digitalWrite(firstPin+1, !ampelOn);
  digitalWrite(firstPin+2, !ampelOn);
  digitalWrite(lockPin, sperrON);        //sperren
}

void configAmpel() {
  for (int i = 0; i < ampelmax; i++) {
    setAmpel((i*3)+ampelPin, ampelstatus[i], ampelSperrPin+i);
  }
}

void getEinsatz() {
  einsatztime++;
  if (einsatztime % einsatzwahr == 0) {
    einsatztime = 0;
    einsatzwahr = einsatzmintime + random(einsatzmaxtime-einsatzmintime);
    einsatzOff = !einsatzOff;
    digitalWrite(einsatzPin, einsatzOff); 
  }
}
