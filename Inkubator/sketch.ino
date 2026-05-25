#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define GRZALKA_PIN 3 // PWM

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- PID Regulator ---
const float TEMP_ZADANA = 37.5; // 

// PID paramterers
float Kp = 40.0;  // Człon proporcjonalny
float Ki = 0.5;   // Człon całkujący
float Kd = 15.0;  // Człon różniczkujący

float bladSumaryczny = 0;
float ostatniBlad = 0;
unsigned long poprzedniCzas = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  lcd.init();
  lcd.backlight();
  lcd.print("Inkubator PID");
  lcd.setCursor(0, 1);
  lcd.print("Inicjalizacja...");
  delay(2000);
  lcd.clear();
  
  poprzedniCzas = millis();
}

void loop() {
  // 1. Odczyt danych z czujnika
  float tempAktualna = dht.readTemperature();
  
  // Sprawdzenie poprawności odczytu
  if (isnan(tempAktualna)) {
    lcd.setCursor(0, 0);
    lcd.print("Blad czujnika!");
    return;
  }

  // 2. OBLICZENIA PID
  unsigned long obecnyCzas = millis();
  float dt = (obecnyCzas - poprzedniCzas) / 1000.0; // czas w sekundach
  
  // Obliczenie błędu
  float blad = TEMP_ZADANA - tempAktualna;
  
  // Człon P (Proporcjonalny)
  float P = Kp * blad;
  
  // Człon I (Całkujący) z zabezpieczeniem anti-windup
  bladSumaryczny += blad * dt;
  bladSumaryczny = constrain(bladSumaryczny, -50, 50); 
  float I = Ki * bladSumaryczny;
  
  // Człon D (Różniczkujący)
  float D = Kd * ((blad - ostatniBlad) / dt);
  
  // Suma sygnału sterującego
  float sygnalSterujacy = P + I + D;
  
  // Ograniczenie sygnału do zakresu PWM
  int mocPWM = constrain(sygnalSterujacy, 0, 255);
  
  // 3. Sterowanie grzałką
  analogWrite(GRZALKA_PIN, mocPWM);
  
  // Zapisanie wartości do kolejnej pętli
  ostatniBlad = blad;
  poprzedniCzas = obecnyCzas;

  // 4. Wizualizacja na LCD
  lcd.setCursor(0, 0);
  lcd.print("T: "); lcd.print(tempAktualna, 1); lcd.print("C / "); lcd.print(TEMP_ZADANA, 1); lcd.print("C");
  
  lcd.setCursor(0, 1);
  // Procentowy wskaźnik mocy grzania
  int mocProcent = map(mocPWM, 0, 255, 0, 100);
  lcd.print("Moc grzania: "); lcd.print(mocProcent); lcd.print("%   ");

  // Logi
  Serial.print("Temp:"); Serial.print(tempAktualna);
  Serial.print(",Moc:"); Serial.println(mocProcent);

  delay(1000); // Próbkowanie co 1 sekundę
}