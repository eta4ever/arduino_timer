#include "LiquidCrystal.h"
#include "String.h"
#include "DS1307.h"
#include "Wire.h"
#include "EEPROM.h"

// самописный модуль с каналом таймера
#include "timer.h"

// линии, которые использует жкд
// управление подсветкой у нас на 10 выходе,
// туда даем 0-255 (PWM)
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int adcKeyIn  = 0;

// кнопки
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
#define btnBZZZ   -1 // дребезззг


// чтение нажатия кнопки c AI0
int buttonPressed()
{
  adcKeyIn = analogRead(0);
  
  if (adcKeyIn > 1000) return btnNONE; // кнопка не нажата
  
  if (adcKeyIn < 50)   return btnRIGHT;  
  if (adcKeyIn < 195)  return btnUP; 
  if (adcKeyIn < 380)  return btnDOWN; 
  if (adcKeyIn < 555)  return btnLEFT; 
  if (adcKeyIn < 790)  return btnSELECT; 
  
  return btnNONE; // если ничего не подошло
}

int buttonPressedFilter() // фильтруем нажатие. "Правильное" - длительностью minT-maxT мс, иначен считаем btnNONE
{

  unsigned long time1;
  int check1;
  int check2;
  int minT = 10; // миллисекунды
  int maxT = 300;
  int result = btnNONE;

  check1 = buttonPressed();
  
  if (check1 == btnNONE) { return btnNONE; } // если ничего не нажато, выходим. А то будет лишний обсчет времени, который в итоге выливается в пропуск "не попавших" нажатий

  time1 = millis(); //фиксируем счетчик времени

  while ( (time1 + minT) > millis()) //если в течение minT кнопка отпущена, считаем дребезгом, выдаем btnNONE
    {
      check2 = buttonPressed();
      if (check2 == btnNONE) { result = btnBZZZ; break;}
    }

  if (result != btnBZZZ)  { // если не было дребезга, проверяем второй временной интервал
    
    while ( (time1 + maxT) > millis()) { // если в течение minT - maxT кнопка отпущена, выдаем ее, иначе btnNONE

      check2 = buttonPressed();
      if (check2 == btnNONE) { result = check1; break;}

    }
  
  } else {result = btnNONE; } // если был дребезг
  
  return result;

}

String tohhmm (unsigned char hh, unsigned char mm){ // выдает строку hh:mm, при необходимости с ведущими нулями

  String result = "";
  
  if (hh < 10) { result = "0" + String(hh); } else { result = String(hh); }
  result += ":";

  if (mm < 10) { result += "0" + String(mm); } else { result += String(mm); }

  return result;

}

// настройка канала таймера. Передаем функции СЦЫЛКУ на объект. Иначе, что логично, она его не меняет.
void channelSetup(TimerChannel &channel){

	unsigned char editPositions[] = {3, 4, 6, 7, 11, 12, 14, 15}; // допустимые позиции курсора
  unsigned char cursorPosIndex = 0; // индекс начальной позиция курсора
  bool setupComplete = false; // флаг завершения настройки
  int buttonTemp = btnNONE; // для хранения нажатой кнопки

  unsigned char timeDigits[] = {  // временное хранилище времени по разрядам. Индексы совпалают с индексами позиции курсора!
    channel.getOnTimehh() / 10,
    channel.getOnTimehh() % 10,
    channel.getOnTimemm() / 10,
    channel.getOnTimemm() % 10,
    channel.getOffTimehh() / 10,
    channel.getOffTimehh() % 10,
    channel.getOffTimemm() / 10,
    channel.getOffTimemm() % 10
  };

  unsigned char topValue = 9; // верхний предел изменяемой величины

  lcd.clear();
	lcd.setCursor(0,0);

  // строка вида #1 10:00 - 22:00
  String row1 = "#" + String(channel.getNumber()) + " " + tohhmm(channel.getOnTimehh(), channel.getOnTimemm()); 
  row1 += " - " + tohhmm(channel.getOffTimehh(), channel.getOffTimemm()); 
  lcd.print(row1);

  lcd.setCursor(editPositions[cursorPosIndex], 0); lcd.cursor(); // поместить курсор в начало редактируемой строки, включить курсор-подчеркушку

  while (!setupComplete){ // двигаем курсор, выход по нажатию Select

    buttonTemp = buttonPressedFilter();

    switch (buttonTemp) {

      case btnLEFT: 
        if (cursorPosIndex > 0) { cursorPosIndex--; } else { cursorPosIndex = 7; }; break; // курсор влево, зацикленно

      case btnRIGHT: 
        if (cursorPosIndex < 7) { cursorPosIndex++; } else { cursorPosIndex = 0; }; break; // курсор вправо, зацикленно

      case btnUP:   // кнопка увеличения 
      case btnDOWN: // кнопка уменьшения

        topValue = 9; // в основном у нас верхний предел 9. Но есть нюансы.

        if ((cursorPosIndex == 0) || (cursorPosIndex == 4)) { topValue = 2; } // первая цифра часов не больше 2

        if (
            ((cursorPosIndex == 1) && (timeDigits[0] == 2)) ||
            ((cursorPosIndex == 5) && (timeDigits[4] == 2))
           ) { topValue = 3; } // если изменяется вторая цифра часов, а первая 2, то вторая не больше 3

        if ((cursorPosIndex == 2) || (cursorPosIndex == 6)) { topValue = 5; } // первая цифра минут не больше 5

        // если нажато вверх и изменяемая цифра в максимуме, обнулить ее, иначе увеличить
        if (buttonTemp == btnUP) 
          { if (timeDigits[cursorPosIndex] == topValue) { timeDigits[cursorPosIndex] = 0; } else { timeDigits[cursorPosIndex]++; }}

        // если нажато вниз и изменяемая цифра в нуле, в максимум ее, иначе увеньшить
        if (buttonTemp == btnDOWN) 
          { if (timeDigits[cursorPosIndex] == 0) { timeDigits[cursorPosIndex] = topValue; } else { timeDigits[cursorPosIndex]--; }}    

        // и отдельное условие: если вторая цифра часов >3, а первая после этого выставлена в двойку, сбрасываем вторую в 3.
        // печатаем эту тройку и возвращаем курсор на место

        if ( (timeDigits[0] == 2) && (timeDigits[1] > 3) ) {

          timeDigits[1] = 3; 
          lcd.setCursor(editPositions[1], 0); 
          lcd.print(3); 
          lcd.setCursor(editPositions[cursorPosIndex], 0);
        }

        if ( (timeDigits[4] == 2) && (timeDigits[5] > 3) ) {

          timeDigits[5] = 3; 
          lcd.setCursor(editPositions[5], 0); 
          lcd.print(3);
          lcd.setCursor(editPositions[cursorPosIndex], 0);
        }

        // выводим измененную цифру
        lcd.print(timeDigits[cursorPosIndex]);
        break;

      case btnSELECT: 
        
        // настройка канала окончена
        setupComplete = true;

        // записываем установленные значения в объект канала
        channel.setOnTime( timeDigits[0] * 10 + timeDigits[1], timeDigits[2] * 10 + timeDigits[3]);
        channel.setOffTime( timeDigits[4] * 10 + timeDigits[5], timeDigits[6] * 10 + timeDigits[7]);

        // записываем установки таймера во флеш
        EEPROM.write(channel.getNumber()*4, channel.getOnTimehh());
        EEPROM.write(channel.getNumber()*4+1, channel.getOnTimemm());
        EEPROM.write(channel.getNumber()*4+2, channel.getOffTimehh());
        EEPROM.write(channel.getNumber()*4+3, channel.getOffTimemm());

        break;

    }

    lcd.setCursor(editPositions[cursorPosIndex], 0);

  }
}

// настройка времени. По сути, перепиленная копия channelSetup. Да, копипаста, да и хрен бы.
void timeSetup(void)
{
    unsigned char editPositions[] = {0, 1, 3, 4, 6, 7, 9, 10, 12, 13}; // допустимые позиции курсора
    unsigned char cursorPosIndex = 0; // индекс начальной позиция курсора
    bool setupComplete = false; // флаг завершения настройки
    int buttonTemp = btnNONE; // для хранения нажатой кнопки

    unsigned char dateTimeDigits[] = {  // временное хранилище времени и даты по разрядам. Индексы совпалают с индексами позиции курсора!
      RTC.get(DS1307_HR, true) / 10, // буфер обновляем один раз, на каждое считывание не надо
      RTC.get(DS1307_HR, false) % 10,
      RTC.get(DS1307_MIN, false) / 10,
      RTC.get(DS1307_MIN, false) % 10,
      RTC.get(DS1307_DATE, false) / 10,
      RTC.get(DS1307_DATE, false) % 10,
      RTC.get(DS1307_MTH, false) / 10,
      RTC.get(DS1307_MTH, false) % 10,
      (RTC.get(DS1307_YR, false) % 100 - RTC.get(DS1307_YR, false) % 10) / 10,
      RTC.get(DS1307_YR, false) % 10,
    };

    unsigned char topValue = 9; // верхний предел изменяемой величины

    lcd.clear();
    lcd.setCursor(0,0);

    // строка вида 22:00 01.11.14
    String row1 = String(dateTimeDigits[0]) + String(dateTimeDigits[1]) + ":" + String(dateTimeDigits[2]) + String(dateTimeDigits[3]) + " ";
    row1 += String(dateTimeDigits[4]) + String(dateTimeDigits[5]) + "." + String(dateTimeDigits[6]) + String(dateTimeDigits[7]) + ".";
    row1 += String(dateTimeDigits[8]) + String(dateTimeDigits[9]);

    lcd.print(row1);

    lcd.setCursor(editPositions[cursorPosIndex], 0); lcd.cursor(); // поместить курсор в начало редактируемой строки, включить курсор-подчеркушку

    while (!setupComplete){ // двигаем курсор, выход по нажатию Select


      buttonTemp = buttonPressedFilter();

      switch (buttonTemp) {

        case btnLEFT: 
         if (cursorPosIndex > 0) { cursorPosIndex--; } else { cursorPosIndex = 9; }; break; // курсор влево, зацикленно

        case btnRIGHT: 
         if (cursorPosIndex < 9) { cursorPosIndex++; } else { cursorPosIndex = 0; }; break; // курсор вправо, зацикленно

        case btnUP:   // кнопка увеличения 
        case btnDOWN: // кнопка уменьшения

          topValue = 9; // в основном у нас верхний предел 9. Но есть нюансы. Много.

          if (cursorPosIndex == 0) { topValue = 2; } // первая цифра часов не больше 2

          if ( (cursorPosIndex == 1) && (dateTimeDigits[0] == 2) ) { topValue = 3; } // вторая цифра часов не больше 3, если первая - 2

          if (cursorPosIndex == 2) { topValue = 5; } // первая цифра минут не больше 2

          if (cursorPosIndex == 4) { topValue = 3; } // первая цифра дня не больше 3

          if ( (cursorPosIndex == 5) && (dateTimeDigits[4] == 3) ) { topValue = 1; } // вторая цифра дня не больше 1, если первая - 3

          if (cursorPosIndex == 6) { topValue = 1; } // первая цифра месяца не больше 1

          if ( (cursorPosIndex == 7) && (dateTimeDigits[6] == 1) ) { topValue = 2; } // вторая цифра месяца не больше 2, если первая - 1

          // если нажато вверх и изменяемая цифра в максимуме, обнулить ее, иначе увеличить
          if (buttonTemp == btnUP) 
            { if (dateTimeDigits[cursorPosIndex] == topValue) { dateTimeDigits[cursorPosIndex] = 0; } else { dateTimeDigits[cursorPosIndex]++; }}

          // если нажато вниз и изменяемая цифра в нуле, в максимум ее, иначе увеньшить
          if (buttonTemp == btnDOWN) 
            { if (dateTimeDigits[cursorPosIndex] == 0) { dateTimeDigits[cursorPosIndex] = topValue; } else { dateTimeDigits[cursorPosIndex]--; }}  

         // отдельное условие: если вторая цифра часов >3, а первая после этого выставлена в двойку, сбрасываем вторую в 3.
         // печатаем эту тройку и возвращаем курсор на место
         if ( (dateTimeDigits[0] == 2) && (dateTimeDigits[1] > 3) ) {

           dateTimeDigits[1] = 3; 
           lcd.setCursor(editPositions[1], 0); 
           lcd.print(3); 
           lcd.setCursor(editPositions[cursorPosIndex], 0);
         }


          // такая же коррекция для дня
         if ( (dateTimeDigits[4] == 3) && (dateTimeDigits[5] > 1) ) {

           dateTimeDigits[5] = 1; 
           lcd.setCursor(editPositions[5], 0); 
           lcd.print(1); 
           lcd.setCursor(editPositions[cursorPosIndex], 0);
         }

          // такая же коррекция для месяца
         if ( (dateTimeDigits[6] == 1) && (dateTimeDigits[7] > 2) ) {

            dateTimeDigits[7] = 2; 
            lcd.setCursor(editPositions[7], 0); 
            lcd.print(2); 
            lcd.setCursor(editPositions[cursorPosIndex], 0);
          }

         // выводим измененную цифру
          lcd.print(dateTimeDigits[cursorPosIndex]);
          break;

        case btnSELECT: 
        
         // настройка канала окончена
          setupComplete = true;

          // записываем установленные значения в RTC
         RTC.set(DS1307_HR, dateTimeDigits[0] * 10 + dateTimeDigits[1]);
         RTC.set(DS1307_MIN, dateTimeDigits[2] * 10 + dateTimeDigits[3]);
         RTC.set(DS1307_SEC, 0);
         RTC.set(DS1307_DATE, dateTimeDigits[4] * 10 + dateTimeDigits[5]);
         RTC.set(DS1307_MTH, dateTimeDigits[6] * 10 + dateTimeDigits[7]);
         RTC.set(DS1307_YR, dateTimeDigits[8] * 10 + dateTimeDigits[9]);

         break;    
      }

      lcd.setCursor(editPositions[cursorPosIndex], 0);

    }
}


// отображение дежурного экрана, отображательный кусок от timeSetup.
void defaultScreen(void)
{
  unsigned char dateTimeDigits[] = {  // временное хранилище времени и даты по разрядам. Индексы совпалают с индексами позиции курсора!
      RTC.get(DS1307_HR, true) / 10, // буфер обновляем один раз, на каждое считывание не надо
      RTC.get(DS1307_HR, false) % 10,
      RTC.get(DS1307_MIN, false) / 10,
      RTC.get(DS1307_MIN, false) % 10,
      RTC.get(DS1307_DATE, false) / 10,
      RTC.get(DS1307_DATE, false) % 10,
      RTC.get(DS1307_MTH, false) / 10,
      RTC.get(DS1307_MTH, false) % 10,
      RTC.get(DS1307_YR, false) % 100 - RTC.get(DS1307_YR, false) % 10,
      RTC.get(DS1307_YR, false) % 10,
    };

    unsigned char topValue = 9; // верхний предел изменяемой величины

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.noCursor(); // отключаем курсор-подчеркушку

    // строка вида 22:00 01.11.14
    String row1 = String(dateTimeDigits[0]) + String(dateTimeDigits[1]) + ":" + String(dateTimeDigits[2]) + String(dateTimeDigits[3]) + " ";
    row1 += String(dateTimeDigits[4]) + String(dateTimeDigits[5]) + "." + String(dateTimeDigits[6]) + String(dateTimeDigits[7]) + ".";
    //row1 += String(dateTimeDigits[8]) + String(dateTimeDigits[9]);
    row1 += String(RTC.get(DS1307_YR, false));

    lcd.print(row1);
}

// объекты каналов таймера
TimerChannel ch[4];

// для минут с rtc
int mm;

// список выходов на реле
const unsigned char relay0 = 11; 
const unsigned char relay1 = 12;
const unsigned char relay2 = 2;
const unsigned char relay3 = 3;   

void setup()
{
 //debug
 Serial.begin(9600);
 
 lcd.begin(16, 2);   // запуск жкд
 lcd.setCursor(0,0);
 analogWrite (10, 50); // установка яркости подсветки
 
// установка пинов как выходов
pinMode(relay0, OUTPUT);
pinMode(relay1, OUTPUT);
pinMode(relay2, OUTPUT);
pinMode(relay3, OUTPUT);

// инициализация каналов
 ch[0].begin(relay0, 0); 
 ch[1].begin(relay1, 1);
 ch[2].begin(relay2, 2);
 ch[3].begin(relay3, 3);

  for (unsigned char i=0; i <= 3; i++)
  {
    
    // инициализация EEPROM. По сути, будет выполнена один раз за все существование девайса
    // если в используемые ячейки записана лажа, записать туда нули
    if (EEPROM.read(i*4) > 23) {EEPROM.write(i*4, 0); };
    if (EEPROM.read(i*4+1) > 59) {EEPROM.write(i*4+1, 0); };
    if (EEPROM.read(i*4+2) > 23) {EEPROM.write(i*4+2, 0); };
    if (EEPROM.read(i*4+3) > 59) {EEPROM.write(i*4+3, 0); };

     // а потом считывание сохраненных настроек таймеров из EEPROM
    ch[i].setOnTime(EEPROM.read(i*4), EEPROM.read(i*4+1));
    ch[i].setOffTime(EEPROM.read(i*4+2), EEPROM.read(i*4+3));
  }

// отображение экрана по умолчанию
defaultScreen();

 mm = RTC.get(DS1307_MIN, true);
}
 
void loop()
{
  int buttonTemp = buttonPressedFilter();
  if (buttonTemp == btnLEFT) {channelSetup(ch[0]); defaultScreen(); }
  if (buttonTemp == btnUP) {channelSetup(ch[1]); defaultScreen(); }
  if (buttonTemp == btnDOWN) {channelSetup(ch[2]); defaultScreen(); }
  if (buttonTemp == btnRIGHT) {channelSetup(ch[3]); defaultScreen(); }

  if (buttonTemp == btnSELECT) {timeSetup(); defaultScreen(); }

  // если счетчик минут не совпадает с реальными минутами, обновить экран, выставить счетчик в реальные минуты, и заодно дернуть все таймеры
  if (RTC.get(DS1307_MIN, true) != mm) 
  {

    defaultScreen(); 
    mm = RTC.get(DS1307_MIN, false); 

    Serial.print(RTC.get(DS1307_HR, false)); Serial.print(':'); Serial.print(mm); Serial.print(' ');

    for (unsigned char i = 0; i <= 3; i++) 
    { 
      ch[i].check(RTC.get(DS1307_HR, false), mm); 

      //debug
      Serial.print(i); Serial.print(" "); Serial.print(ch[i].getState()); Serial.print(" "); Serial.print(ch[i].getOnTimehh()); Serial.print(':'); Serial.print(ch[i].getOnTimemm());
      Serial.print(' '); Serial.print(ch[i].getOffTimehh()); Serial.print(':'); Serial.print(ch[i].getOffTimemm()); Serial.print(" | ");
    }

    //debug
    Serial.println("");

  } 
}
