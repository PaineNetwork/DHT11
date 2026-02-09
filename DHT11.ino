#include "DHT.h"  
#include "LiquidCrystal.h" 
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);    // chan ket noi LCD voi arduino     
const int DHTPIN = 4;      // chan vao cua cam bien DHT11
const int DHTTYPE = DHT11;  

DHT dht(DHTPIN, DHTTYPE);
void setup() {

  dht.begin(); 
  lcd.begin(16,2);// dinh nghia loai LCD duoc dung
        
}

void loop() {
 int h = dht.readHumidity();    
  int t = dht.readTemperature(); // o day minh dung ham so nguyen, cac ban co the dung ham float de sai so thap
lcd.setCursor(0,0);
lcd.print("NHIET DO :");
lcd.setCursor(10,0);
lcd.print(t);// in ra nhiet do
lcd.setCursor(2,1);
lcd.print(t);
lcd.setCursor(13,0);
lcd.print("C");
lcd.setCursor(0,1);
lcd.print("DO AM    :");
lcd.setCursor(10,1);
lcd.print(h);
lcd.setCursor(13,1);
lcd.print("%");         
  delay(100);                     
}