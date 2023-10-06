#include <OneWire.h>
#include "helpers.h"
#include "button.h"
#include "mode.h"
#include "Keys/BlankKey.h"
#include "Keys/Key_RW1990_1_2_TM01.h"
#include "Keys/Key_TM2004.h"

#define iButtonPin 5      // Линия data ibutton
#define BtnPin 7          // Кнопка переключения режима чтение/запись

OneWire ibutton (iButtonPin);
byte keyID[8];                            // ID ключа для записи
Button modeSwitchBtn(BtnPin);

enum emRWType {TM01, RW1990_1, RW1990_2, TM2004};                            // тип болванки
enum emkeyType {keyUnknown, keyDallas, keyTM2004, keyCyfral, keyMetacom};    // тип оригинального ключа  
emkeyType keyType;

emRWType getRWtype();
bool isRW1990_1();
bool isRW1990_2();
bool isTM2004();

bool write2iBtn(byte* blankKeyData);

bool tryToReadCyfral();
bool readCyfral(byte* buf, byte CyfralPin);
void ACsetOn();
unsigned long pulseAComp(bool pulse, unsigned long timeOut = 20000);
bool validateCyfralByte(byte testByte);

bool tryToSwitchMode(bool isWritingMode);

bool parseKeyData(byte* buf);

// ROM–section, containing a 6–byte device–unique serial number, a one–byte family code, and a CRC verification byte
// The lower seven bits of the family code indicate the device type; the most significant bit of the family code is used to flag customer–specific versions

void setup() {
  Serial.begin(115200);
}


void loop() {
  byte keyDataBuffer[8];
  static Mode mode;
  static bool isKeySuccessfulRead = false;

  if (Serial.read() == 't' || modeSwitchBtn.isPressed()) {
    mode.tryToSwitchMode(isKeySuccessfulRead);
  }

  // запускаем поиск cyfral
  if (mode.isReading() && tryToReadCyfral()) {
    isKeySuccessfulRead = true;
  }

  // пробуем прочитать dallas
  if (!ibutton.search(keyDataBuffer)) {
    ibutton.reset_search();

    delay(200);

    return;
  }

  if (mode.isReading())
    isKeySuccessfulRead = parseKeyData(keyDataBuffer);
  else
    write2iBtn(keyDataBuffer);

  delay(300);
}

//*************** dallas **************
bool parseKeyData(byte* buf){
  printHex(buf, 8);

  // копируем прочтенный код в keyID
  for (byte i = 0; i < 8; i++) {
    keyID[i] = buf[i];
  }

  // это ключ формата dallas
  if (buf[0] == DALLAS_FAMILY_CODE) {
    keyType = keyDallas;

    if (getRWtype() == TM2004) {
      keyType = keyTM2004;
    }

    if (OneWire::crc8(buf, 7) != buf[7]) {
      Serial.println("CRC is not valid!");
      return false;
    }

    return true;
  }

  if ((buf[0] >> 4) == 0x0E)
    Serial.println(" Type: unknown family dallas. May be cyfral in dallas key.");
  else
    Serial.println(" Type: unknown family dallas");

  keyType = keyUnknown;
  
  // хз, а вдруг прокатит ;D
  return true;
}

emRWType getRWtype(){    
  // TM01 это неизвестный тип болванки, делается попытка записи TM-01 без финализации для dallas или c финализацией под cyfral или metacom
  // RW1990_1 - dallas-совместимые RW-1990, RW-1990.1, ТМ-08, ТМ-08v2 
  // RW1990_2 - dallas-совместимая RW-1990.2
  // TM2004 - dallas-совместимая TM2004 в доп. памятью 1кб

  // пробуем определить RW-1990.1
  if (isRW1990_1()){
    Serial.println(" Type: dallas RW-1990.1 ");

    return RW1990_1;
  }

  // пробуем определить RW-1990.2
  if (isRW1990_2()){
    Serial.println(" Type: dallas RW-1990.2 ");
 
    return RW1990_2;
  }

  // пробуем определить TM-2004
  if (isTM2004()) {
    Serial.println(" Type: dallas TM2004");

    return TM2004;
  }

  ibutton.reset();

  Serial.println(" Type: dallas unknown, trying TM-01! ");

  // это неизвестный тип DS1990, нужно перебирать алгоритмы записи (TM-01)
  return TM01;
}


bool isRW1990_1() {
  ibutton.reset();
  ibutton.write(0xD1);  // проуем снять флаг записи для RW-1990.1
  ibutton.write_bit(1); // записываем значение флага записи = 1 - отключаем запись

  delay(10);

  pinMode(iButtonPin, INPUT);

  ibutton.reset();
  ibutton.write(0xB5); // send 0xB5 - запрос на чтение флага записи

  return ibutton.read() == 0xFE;
}

bool isRW1990_2() {
  ibutton.reset();
  ibutton.write(0x1D);  // проуем установить флаг записи для RW-1990.2 
  ibutton.write_bit(1); // записываем значение флага записи = 1 - включаем запись

  delay(10);

  pinMode(iButtonPin, INPUT);

  ibutton.reset();
  ibutton.write(0x1E);  // send 0x1E - запрос на чтение флага записи

  if (ibutton.read() != 0xFE)
    return false;
  
  ibutton.reset();
  ibutton.write(0x1D);  // возвращаем оратно запрет записи для RW-1990.2
  ibutton.write_bit(0); // записываем значение флага записи = 0 - выключаем запись

  delay(10);

  pinMode(iButtonPin, INPUT);

  return true;
}

bool isTM2004() {
  ibutton.reset();
  ibutton.write(READ_ROM_CODE);  // посылаем команду чтения ROM для перевода в расширенный 3-х байтовый режим

  //читаем данные ключа
  for ( byte i = 0; i < 8; i++)
    ibutton.read();

  byte readCommand[3] = {0xAA, 0,0};

  // пробуем прочитать регистр статуса для TM-2004
  ibutton.write_bytes(readCommand, 3);

  // вычисляем CRC комманды
  if (OneWire::crc8(readCommand, 3) != ibutton.read())
    return false;

  ibutton.read();
  ibutton.reset();
}

bool write2iBtn(byte* blankKeyData){
  Serial.print("The new key code is: ");

  printHex(blankKeyData, 8);

  // сравниваем код для записи с тем, что уже записано в ключе.
  // если коды совпадают, ничего писать не нужно
  if (isBuffersEquals(keyID, blankKeyData, 8)) {
    Serial.println(" it is the same key. Writing in not needed.");
    delay(500);

    return false;
  }

  // определяем тип RW-1990.1 или 1990.2 или TM-01
  emRWType rwType = getRWtype();

  Serial.print("\n Burning iButton ID: ");

  AbstractKey* blankKey = blankKeyFactory(rwType);
  bool res = blankKey->write(keyID);
  delete blankKey;

  return res;
}

AbstractKey* blankKeyFactory(emRWType rwType) {
  switch (rwType)
  {
  case emRWType::RW1990_1:
    return new Key_RW1990_1(&ibutton, iButtonPin);
  case emRWType::RW1990_2:
    return new Key_RW1990_2(&ibutton, iButtonPin);
  case emRWType::TM01:
    return new Key_TM01(&ibutton, iButtonPin);
  case emRWType::TM2004:
    return new Key_TM2004(&ibutton, iButtonPin);
  default:
    return 0;
  }
}

//************ Cyfral ***********************

bool tryToReadCyfral(){
  byte readBuffer[8];

  for (byte i = 0; i < 8; i++)
    readBuffer[i] = 0;

  if (!readCyfral(readBuffer, iButtonPin))
    return false; 

  keyType = keyCyfral;

  printHex(readBuffer, 8);

  // копируем прочтенный код в ReadID
  for (byte i = 0; i < 8; i++) {
    keyID[i] = readBuffer[i];
  }

  Serial.println(" Type: Cyfral ");

  return true;  
}

// понятия не имею, что тут происходит
bool readCyfral(byte* buf, byte CyfralPin){
  unsigned long ti;
  byte j = 0;

  digitalWrite(CyfralPin, LOW);
  pinMode(CyfralPin, OUTPUT);  //отклчаем питание от ключа

  delay(200);

  ACsetOn();    //analogComparator.setOn(0, CyfralPin); 

  pinMode(CyfralPin, INPUT_PULLUP);  // включаем пиание Cyfral

  // чиаем 36 bit
  for (byte i = 0; i < 36; i++){
    ti = pulseAComp(HIGH);
    if ((ti == 0) || (ti > 200))
      break;                      // not Cyfral
    //if ((ti > 20)&&(ti < 50)) bitClear(buf[i >> 3], 7-j);
    if ((ti > 50) && (ti < 200))
      bitSet(buf[i >> 3], 7-j);
    j++;
    if (j>7) j=0; 
  }

  // not Cyfral
  if (ti == 0)
    return false;
  if ((buf[0] >> 4) != 0b1110)
    return false;

  byte test;

  for (byte i = 1; i < 4; i++){
    test = buf[i] >> 4;

    if (!validateCyfralByte(test))
      return false;

    test = buf[i] & 0x0F;

    if (!validateCyfralByte(test))
      return false;
  }

  return true;
}

void ACsetOn(){
  ADCSRA &= ~(1<<ADEN);      // выключаем ADC
  ADCSRB |= (1<<ACME);        //включаем AC
  ADMUX = 0b00000101;        // подключаем к AC Линию A5
}

unsigned long pulseAComp(bool pulse, unsigned long timeOut){  // pulse HIGH or LOW
  bool AcompState;
  unsigned long tStart = micros();

  do {
    AcompState = ACSR & _BV(ACO);  // читаем флаг компаратора

    if (AcompState == pulse) {
      tStart = micros();

      do {
        AcompState = ACSR & _BV(ACO);  // читаем флаг компаратора
        if (AcompState != pulse)
          return (long)(micros() - tStart);  
      } while ((long)(micros() - tStart) < timeOut);

      return 0;                                                 //таймаут, импульс не вернуся оратно
    }

  } while ((long)(micros() - tStart) < timeOut);

  return 0;
}

bool validateCyfralByte(byte testByte) {
  return testByte == 0b0001 || testByte == 0b0010 || testByte == 0b0100 || testByte == 0b1000;
}

//*************************************