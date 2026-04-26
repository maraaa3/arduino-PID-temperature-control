# Arduino PID Temperature Control System

Sistem avansat de control al temperaturii folosind algoritm PID cu senzor DHT11, interfață LCD și navigare cu butoane.

## 📋 Descrierea Proiectului

Acest proiect implementează un **sistem de control PID complet** pentru reglarea automată a temperaturii. Sistemul poate:
- 🌡️ Citi temperatura în timp real din senzor DHT11
- 📊 Aplica algoritm PID pentru control precis
- 📈 Executa un profil de temperatură personalizat (încălzire → menținere → răcire)
- 🎮 Oferi o interfață LCD pentru vizualizare și configurare
- 💾 Stoca parametri în EEPROM pentru persistență

## 🔧 Cerințe Hardware

| Component | Model | Pin Arduino |
|-----------|-------|-------------|
| Placa Arduino | Uno/Mega/Nano | - |
| Senzor Temperatură | DHT11 | A0 |
| Element Încălzire | Rezistor/Halogen | 3 (PWM) |
| Ecran LCD | 16x2 I2C (0x27) | SDA/SCL |
| Buton OK | Generic | 6 |
| Buton CANCEL/START | Generic | 7 |
| Buton PLUS | Generic | 8 |
| Buton MINUS | Generic | 9 |

## 📚 Biblioteci Necesare

Instalează din Arduino IDE → Sketch → Include Library → Manage Libraries:

- Wire (built-in)
- LiquidCrystal_I2C (by Frank de Brabander)
- EEPROM (built-in)
- DHT (by Adafruit)

## ⚙️ Caracteristici Principale

### 1. Algoritm PID
Output = Kp × Error + Ki × ∑Error + Kd × dError/dt

- **Kp (80.0)**: Proporțional - răspuns imediat la eroare
- **Ki (0.5)**: Integral - elimină erori în regim stabil
- **Kd (1.0)**: Derivativ - reduce oscilații

### 2. Profil de Temperatură
Timp: 0s → 30s → 50s → 80s
Profil: Răcire → ÎNCĂLZIRE → MENȚINERE → RĂCIRE
Temp: T_start → T_set → T_start

### 3. Stări Sistem
- MENIU_PRINCIPAL: Afișare temperatură și meniu
- EDIT_TSET: Editare temperatură țintă
- EDIT_KP/KI/KD: Ajustare parametri PID
- EDIT_T_INC/T_MEN/T_RAC: Ajustare timpi profil
- RULARE_PROCES: Execuție proces cu control PID

### 4. Mod Perturbație
Simulează tulburări în sistem pentru testare robustețe

### 5. Persistență EEPROM
Toți parametrii se salvează automat

## 🚀 Ghid Rapid

### Meniu Principal
T:XX.X Set:XX
Ready [Start]

Butoane:
- OK: Intră în meniu configurare
- CANCEL/START: Pornește procesul

### Configurare Parametri
Flow: Temp → Kp → Ki → Kd → Timp Inc → Timp Men → Timp Rac

### Rulare Proces
T:38.5 S:45
Inc: 12s

## 📊 Valori Implicite

T_set = 40.0°C - Temperatură țintă
Kp = 80.0 - Proporțional
Ki = 0.5 - Integral
Kd = 1.0 - Derivativ
t_incalzire = 30s - Timp încălzire
t_mentinere = 20s - Timp menținere
t_racire = 30s - Timp răcire

## 🔍 Troubleshooting

| Problemă | Cauză | Soluție |
|----------|-------|---------|
| DHT nu citește | Conexiune pin | Verifica pinul A0 și VCC/GND |
| LCD negru | Adresa I2C | Rulează scanner I2C |
| Nu se încălzește | PWM disabled | Verifica pin 3 |
| Oscilații mari | Kp prea mare | Scade Kp |

## ⚠️ Observații Importante

1. DHT11 - Citire lentă (1 citire/secundă max)
2. PID Loop - 100ms interval pentru stabilitate
3. PWM Pini - 3, 5, 6, 9, 10, 11 pe Arduino Uno
4. Calibrare - DHT11 poate avea ±2°C eroare

## 🔐 Siguranță

- ⚠️ Testați cu sarcini mici înainte
- ⚠️ Folosiți releu/MOSFET pentru putere mare
- ⚠️ Verificați limitele de temperatură
- ⚠️ Monitorizați permanent sistemul

---

Versiune: 1.0
Autor: Mara Broscăţan (@maraaa3)
Data: 2026-04-26