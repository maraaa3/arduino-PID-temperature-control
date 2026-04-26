#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <DHT.h>

// --- HARDWARE ---
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Configurare DHT11
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int PIN_INCALZIRE = 3; 

// Butoane
const int BTN_OK = 6;
const int BTN_CANCEL = 7;
const int BTN_PLUS = 8;
const int BTN_MINUS = 9;

// --- VARIABILE PARAMETRI ---
float T_set = 40.0;
float Kp = 80.0;
float Ki = 0.5;
float Kd = 1.0;
int t_incalzire = 30; 
int t_mentinere = 20;
int t_racire = 30;

// --- VARIABILE INTERNE ---
float T_curenta = 0.0;     
float Moving_SetPoint = 0.0; 
float Offset_Calibrare = 0.0;

// Perturbatii
bool perturbatie_activa = false;

// PID si Proces
float eroare_anterioara = 0, suma_erori = 0;
unsigned long last_pid_time = 0;
unsigned long timp_start_proces = 0;
float T_start_proces = 0.0;
bool proces_activ = false;

// Timer pentru citirea DHT
unsigned long last_dht_read = 0;

enum StareSistem { 
  MENIU_PRINCIPAL, EDIT_TSET, EDIT_KP, EDIT_KI, EDIT_KD, 
  EDIT_T_INC, EDIT_T_MEN, EDIT_T_RAC, RULARE_PROCES 
};
StareSistem stare = MENIU_PRINCIPAL;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  dht.begin();
  
  pinMode(PIN_INCALZIRE, OUTPUT);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_CANCEL, INPUT_PULLUP);
  pinMode(BTN_PLUS, INPUT_PULLUP);
  pinMode(BTN_MINUS, INPUT_PULLUP);

  float check; EEPROM.get(0, check);
  if(!isnan(check) && check > 0 && check < 150) {
      int addr = 0;
      EEPROM.get(addr, T_set); addr += sizeof(float);
      EEPROM.get(addr, Kp);    addr += sizeof(float);
      EEPROM.get(addr, Ki);    addr += sizeof(float);
      EEPROM.get(addr, Kd);    addr += sizeof(float);
      EEPROM.get(addr, t_incalzire); addr += sizeof(int);
      EEPROM.get(addr, t_mentinere); addr += sizeof(int);
      EEPROM.get(addr, t_racire);    addr += sizeof(int);
  } else { salvareEEPROM(); }
  
  lcd.setCursor(0,0); lcd.print("DHT11 Activat!"); delay(1000); lcd.clear();
}

void loop() {
  citesteSenzorDHT();

  switch (stare) {
    case MENIU_PRINCIPAL:
      analogWrite(PIN_INCALZIRE, 0);
      afisarePrincipala();
      verificaNavigarePrincipal();
      break;

    case EDIT_TSET:  gestioneazaEditare("Set Temp:", T_set, 1.0, EDIT_KP); break;
    case EDIT_KP:    gestioneazaEditare("Set Kp:", Kp, 5.0, EDIT_KI); break;
    case EDIT_KI:    gestioneazaEditare("Set Ki:", Ki, 0.1, EDIT_KD); break;
    case EDIT_KD:    gestioneazaEditare("Set Kd:", Kd, 0.1, EDIT_T_INC); break;
    case EDIT_T_INC: gestioneazaEditareInt("Timp Inc(s):", t_incalzire, 5, EDIT_T_MEN); break;
    case EDIT_T_MEN: gestioneazaEditareInt("Timp Men(s):", t_mentinere, 5, EDIT_T_RAC); break;
    case EDIT_T_RAC: gestioneazaEditareInt("Timp Rac(s):", t_racire, 5, MENIU_PRINCIPAL); break;

    case RULARE_PROCES:
      calculPID_si_Profil();
      afisareRulare();
      verificaNavigareRulare();
      break;
  }
}

void citesteSenzorDHT() {
  if (millis() - last_dht_read > 1000) {
      float t = dht.readTemperature();
      
      if (!isnan(t)) {
          float temp_reala = t + Offset_Calibrare;
          
          if (perturbatie_activa && proces_activ) {
             T_curenta = temp_reala + (random(-50, 50) / 10.0);
          } else {
             T_curenta = temp_reala;
          }
      }
      last_dht_read = millis();
  }
}

void salvareEEPROM() {
  int addr = 0;
  EEPROM.put(addr, T_set); addr += sizeof(float);
  EEPROM.put(addr, Kp);    addr += sizeof(float);
  EEPROM.put(addr, Ki);    addr += sizeof(float);
  EEPROM.put(addr, Kd);    addr += sizeof(float);
  EEPROM.put(addr, t_incalzire); addr += sizeof(int);
  EEPROM.put(addr, t_mentinere); addr += sizeof(int);
  EEPROM.put(addr, t_racire);    addr += sizeof(int);
}

void calculPID_si_Profil() {
  unsigned long now = millis();
  if (now - last_pid_time >= 100) {
    float sec = (now - timp_start_proces) / 1000.0;
    
    if (sec <= t_incalzire) {
        float procent = sec / t_incalzire;
        Moving_SetPoint = T_start_proces + (procent * (T_set - T_start_proces));
    } 
    else if (sec <= (t_incalzire + t_mentinere)) {
        Moving_SetPoint = T_set;
    } 
    else if (sec <= (t_incalzire + t_mentinere + t_racire)) {
        float timp_in_racire = sec - (t_incalzire + t_mentinere);
        float ramas = 1.0 - (timp_in_racire / t_racire);
        Moving_SetPoint = T_set * ramas; 
        if(Moving_SetPoint < T_start_proces) Moving_SetPoint = T_start_proces;
    } 
    else {
       proces_activ = false; stare = MENIU_PRINCIPAL;
       analogWrite(PIN_INCALZIRE, 0); return;
    }

    float eroare = Moving_SetPoint - T_curenta;
    float dt = (now - last_pid_time) / 1000.0;
    suma_erori += eroare * dt;
    float derivativa = (eroare - eroare_anterioara) / dt;
    float output = (Kp * eroare) + (Ki * suma_erori) + (Kd * derivativa);
    if (output > 255) output = 255; if (output < 0) output = 0;
    analogWrite(PIN_INCALZIRE, (int)output);
    eroare_anterioara = eroare; last_pid_time = now;
  }
}

void verificaNavigarePrincipal() {
  if (digitalRead(BTN_OK) == LOW) { 
    delay(200); lcd.clear(); stare = EDIT_TSET;
  }
  if (digitalRead(BTN_CANCEL) == LOW) {
    delay(200); lcd.clear();
    timp_start_proces = millis(); last_pid_time = millis(); last_dht_read = 0;
    suma_erori = 0; eroare_anterioara = 0;
    
    float t = dht.readTemperature();
    if(!isnan(t)) T_curenta = t + Offset_Calibrare;
    T_start_proces = T_curenta;
    if(T_start_proces > T_set) T_start_proces = T_curenta; 

    proces_activ = true; perturbatie_activa = false;
    stare = RULARE_PROCES;
  }
}

void verificaNavigareRulare() {
  if (digitalRead(BTN_CANCEL) == LOW) { 
     delay(200); proces_activ = false; analogWrite(PIN_INCALZIRE, 0);
     stare = MENIU_PRINCIPAL; lcd.clear();
  }
  if (digitalRead(BTN_OK) == LOW) { 
     delay(200); perturbatie_activa = !perturbatie_activa;
  }
}

void gestioneazaEditare(String titlu, float &valoare, float pas, StareSistem urmatoareaStare) {
  lcd.setCursor(0,0); lcd.print(titlu);
  lcd.setCursor(0,1); lcd.print(valoare, 1); lcd.print("   [OK]=Next");
  if (digitalRead(BTN_PLUS) == LOW) { valoare += pas; delay(150); }
  if (digitalRead(BTN_MINUS) == LOW) { valoare -= pas; delay(150); }
  if (digitalRead(BTN_OK) == LOW) {
    delay(200); if(urmatoareaStare == MENIU_PRINCIPAL) salvareEEPROM();
    stare = urmatoareaStare; lcd.clear();
  }
}

void gestioneazaEditareInt(String titlu, int &valoare, int pas, StareSistem urmatoareaStare) {
  lcd.setCursor(0,0); lcd.print(titlu);
  lcd.setCursor(0,1); lcd.print(valoare); lcd.print(" sec  ");
  if (digitalRead(BTN_PLUS) == LOW) { valoare += pas; delay(150); }
  if (digitalRead(BTN_MINUS) == LOW) { valoare -= pas; delay(150); }
  if(valoare < 0) valoare = 0;
  if (digitalRead(BTN_OK) == LOW) {
    delay(200); if(urmatoareaStare == MENIU_PRINCIPAL) salvareEEPROM();
    stare = urmatoareaStare; lcd.clear();
  }
}

void afisarePrincipala() {
  lcd.setCursor(0,0); lcd.print("T:"); lcd.print(T_curenta, 1); lcd.print(" Set:"); lcd.print((int)T_set);
  lcd.setCursor(0,1); lcd.print("Ready [Start]");
}

void afisareRulare() {
  float sec = (millis() - timp_start_proces) / 1000.0;
  lcd.setCursor(0,0); 
  lcd.print("T:"); lcd.print(T_curenta, 1); 
  if(perturbatie_activa) lcd.print("!"); else lcd.print(" ");
  lcd.setCursor(9,0); lcd.print("S:"); lcd.print((int)Moving_SetPoint); lcd.print(" ");
  lcd.setCursor(0,1);
  if (sec <= t_incalzire) { lcd.print("Inc: "); lcd.print(t_incalzire - (int)sec); lcd.print("s "); } 
  else if (sec <= (t_incalzire + t_mentinere)) { lcd.print("Men: "); lcd.print((t_incalzire + t_mentinere) - (int)sec); lcd.print("s "); } 
  else { lcd.print("Rac: "); lcd.print((t_incalzire + t_mentinere + t_racire) - (int)sec); lcd.print("s "); }
}