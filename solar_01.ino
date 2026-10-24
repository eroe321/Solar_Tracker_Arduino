
// ============= Verwendete Bibliotheken hinzufügen ================

#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>

// ============= Komponenten für Konfiguration benennen ================

Servo Servomotor;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_INA219 ina219;

// ============= KONFIGURATIONSVARIABLEN ================

int schritt = 2; // Je niedriger, desto feiner wird adjustiert.
int toleranz = 50; // Je höher, desto weniger wird adjustiert.
int energiesparen_ms = 5000; // Je höher, desto energiesparender wird das System betrieben. Grenze liegt jedoch bei 120000.
int lichtwertgrenze = 150; // Je höher, desto schneller schaltet System runter bei zu wenig Licht.
int widerstand = 100; // Unser Lastwiderstand in Ohm für die Strommessung.
const float hysterese = 0.3; // Je höher, desto weniger wird ajustiert durch Stromwertänderungen.
float standby_mA = 0.5; // Je höher, desto eher wird in den Stand By Modus geschaltet um Adjustierung zu verhindern.

// ============= Variablen und Pins benennen ================

int ldr_rechts = A0;
int ldr_links = A1;
float busvolt = 0;
float shuntvolt = 0;
float current_mA = 0;
float loadvolt = 0;
int winkel = 90;

// ============= Suchmodus Variablen ================

int suchmodus = 0;
int servoPos = 5;
int besterWinkel = 90;
float maxstrom = 0.0;
unsigned long letzteBewegung = 0;
int suchSchritt = 2;
float strom = 0;

// ============= Adjustierung über Strom Variablen ================

int servoSchritt = 2;
int suchRichtung = 1;
float stromRef = 0.0;
unsigned long letzteRegelung = 0;
const unsigned long regelIntervall = 300;

// ============= Funktion für durchschnittliche Ausgaben ================

int readAverage(int pin) {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delay(2);  // Kleiner delay um ungewollte Schwingvorgänge zu verhindern.
  }
  return total / 10;
}


void setup()

{
// Serial.begin(9600);
lcd.init(); // Lcd starten.
lcd.backlight(); // oder lcd.noBacklight(); schaltet die Beleuchtung aus).
Servomotor.attach(9); // Pin 9 steuert den Servo.
Servomotor.write(winkel);
lcd.setCursor(0, 0);
lcd.print("System startet ...");
ina219.begin();
ina219.setCalibration_16V_400mA(); // Klein gewählte Kalibrierung für Solar Panel.
delay(2000); // System-Start bemerkbar machen.
lcd.clear();
}

void loop()

{

// ============= Variablen für Ldr-Widerstände ================

int ldrl = readAverage(ldr_links);
int ldrr = readAverage(ldr_rechts);
int avgLicht = (ldrl + ldrr) / 2;
int dldr = ldrl - ldrr;

// ============= Ausgabe des INA219-Sensor ================

float shuntvolt = ina219.getShuntVoltage_mV();
float busvolt = ina219.getBusVoltage_V();
float current_mA = ina219.getCurrent_mA();
float loadvolt = busvolt + (shuntvolt / 1000);

float leistung_mW = loadvolt * current_mA;

// ============= Suchmodus Aktivierung ================

if (current_mA < standby_mA && suchmodus == 0) {
  suchmodus += 1;
  servoPos = 5;
  maxstrom = 0.0;
}

// ============= Suchmodus Funktion ================

if (suchmodus == 1) {

  if (millis() - letzteBewegung > 60) {
    letzteBewegung = millis();

    Servomotor.write(servoPos);
    strom = ina219.getCurrent_mA();

    if (strom > maxstrom) {
      maxstrom = strom;
      besterWinkel = servoPos;
    }

    servoPos += suchSchritt;

    if (servoPos >= 175) {
      suchmodus += 1;
      Servomotor.write(besterWinkel);
    }
  }

  return;
}

// ============= Standby Modus bei zu dunklem Tageslicht / Keine Sonne ================

if (avgLicht < lichtwertgrenze || current_mA < standby_mA) {
    lcd.setCursor(14, 0);
    lcd.print("SB");
    delay(energiesparen_ms); // System soll hier Energiesparend sein und deshalb nicht durchgehend aktualisieren.
    lcd.clear();
    return;
  }

// ============= Adjustierung der Winkelsausrichtung ================

if (abs(dldr) > toleranz) {
  if (ldrr > ldrl && winkel < 175) {
    winkel += schritt;
  }
  else if (ldrr > ldrl && winkel > 5) {
    winkel -= schritt;
  }
  Servomotor.write(winkel);
}

// ============= Adjustierung der Winkelsausrichtung durch Stromwertänderung ================

if (millis() - letzteRegelung > regelIntervall) {
  letzteRegelung = millis();

  float stromNeu = ina219.getCurrent_mA();

  if (stromRef == 0.0) {
  stromRef = stromNeu;   // Referenz sauber setzen
  return;                // KEINE Servo-Bewegung im ersten Zyklus
}

// filtern
stromNeu = 0.8 * stromRef + 0.2 * stromNeu;

  if (stromNeu > stromRef + hysterese) {

    stromRef = stromNeu;
  }
  else if (stromNeu < stromRef - hysterese) {

    suchRichtung = -suchRichtung;
    servoPos += servoSchritt * suchRichtung;
    servoPos = constrain(servoPos, 5, 175);
    Servomotor.write(servoPos);

    stromRef = stromNeu;
  }

}

// ============= Serielle sowie über LCD-Display Ausgabe ================

lcd.setCursor(0, 0); 
lcd.print(leistung_mW); 
lcd.setCursor(6, 0); 
lcd.print("mW"); 
lcd.setCursor(0, 1); 
lcd.print(current_mA); 
lcd.setCursor(6, 1); 
lcd.print("mA"); 
//Serial.println("Shuntvolt: === Busvolt: === Current_mA: === Loadvolt:"); 
//Serial.println(shuntvolt); Serial.println(busvolt); Serial.println(current_mA); Serial.println(loadvolt); 
delay(2); // Kleiner Delay für saubere Messung. 

}