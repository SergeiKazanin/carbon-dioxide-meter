#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_PM25AQI.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(8, 9); // RX,TX 
SoftwareSerial pmSerial(14, 16); // RX,TX 
byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; //Read CO2 concentration
byte cmd1[9] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86}; //ABC logic off
byte cmd2[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78}; //ZERO POINT CALIBRATION
unsigned char response[9];
byte cfg[7] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70}; // set passive mode
byte trg[7] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71}; // passive mode read
int g1, b;
int flag = 0;
int i;
unsigned int ppm, maxPpm, hours = 0;
uint32_t myTimer1, myTimer2, myTimer3;

Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
PM25_AQI_Data data;

void setup()
{
  lcd.init();
  lcd.backlight();
  b = 1;
  lcd.setCursor(0, 0);
  mySerial.begin(9600);
  pmSerial.begin(9600);
  aqi.begin_UART(&pmSerial);
  delay(100);
  pmSerial.listen();
  pmSerial.write(cfg, 7);
  mySerial.listen();
  mySerial.write(cmd1, 9);
  pinMode(10, INPUT_PULLUP); //button
  digitalWrite(10, HIGH);
  lcd.print(" CO2 and PM2.5");
  lcd.setCursor(0, 1);
  lcd.print("    meter");
}

void loop()
{
  if (millis() - myTimer1 >= 10000) {
    myTimer1 = millis();
    pmSerial.listen();
    pmSerial.write(trg, 7);
    aqi.read(&data);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PM2.5 ");
    lcd.print(data.pm25_env);
    lcd.setCursor(0, 1);
    lcd.print("CO2 ");
    mySerial.listen();
    mySerial.write(cmd, 9);
    memset(response, 0, 9);
    mySerial.readBytes(response, 9);

    byte crc = 0;
    for (i = 1; i < 8; i++) crc += response[i];
    crc = 255 - crc;
    crc++;
    if ( !(response[8] == crc) ) {
      lcd.print("CRC err");
    } else {
      unsigned int responseHigh = (unsigned int) response[2];
      unsigned int responseLow = (unsigned int) response[3];
      ppm = (256 * responseHigh) + responseLow;

      lcd.print(ppm);
      if (maxPpm < ppm) maxPpm = ppm;
      lcd.setCursor(9, 1);
      lcd.print(maxPpm);
      lcd.setCursor(14, 1);
      lcd.print(hours);
    }
  }
  if (millis() - myTimer3 >= 3600000) {
    myTimer3 = millis();
    hours++;
    if (hours > 24) {
      hours = 0;
      maxPpm = 0;
    }
  }


  if (millis() - myTimer2 >= 100) {
    myTimer2 = millis();
    if (digitalRead(10) == LOW && flag == 0) {
      flag = 1;
      if (b == 0) {
        lcd.backlight();
        b = 1;
      } else {
        lcd.noBacklight();
        b = 0;
      }
    }
    if (digitalRead(10) == LOW && flag == 1) {
      g1++;
      myTimer1 = millis();
      if (g1 > 10 && g1 < 100) {
        lcd.clear();
        lcd.print(g1 * 0.1f);
      }
      if (g1 == 100) {
        lcd.clear();
        lcd.print("ZERO CALIBRATION");
        mySerial.write(cmd2, 9);
      }
    }

    if (digitalRead(10) == HIGH) {
      flag = 0;
      g1 = 0;
    }
  }
}
