#ifndef BLANK_KEY_H
#define BLANK_KEY_H

#include "../helpers.h"

class AbstractKey {
    public:
        AbstractKey () {}
        AbstractKey (OneWire* ibutton, byte iButtonPin)
            : _ibutton(ibutton), _iButtonPin(iButtonPin)
        {}

        virtual bool write(byte *keyID) = 0;
    protected:
        OneWire* _ibutton;
        byte _iButtonPin;

        void BurnByte(byte data){
            for (byte n_bit = 0; n_bit < 8; n_bit++){ 
                _ibutton->write_bit(data & 1);  

                // даем время на прошивку каждого бита до 10 мс
                delay(5);

                data = data >> 1;
            }

            pinMode(_iButtonPin, INPUT);
        }
};

class Key_RW1990_1_2_TM01 : public AbstractKey {
    public:
        bool write(byte *keyID){
            byte rwCmd = getRwCommand();
            byte rwFlag = getRwFlag();

            _ibutton->reset();
            _ibutton->write(rwCmd);               // send 0xD1 - флаг записи
            _ibutton->write_bit(rwFlag);          // записываем значение флага записи = 1 - разрешить запись

            delay(10);

            pinMode(_iButtonPin, INPUT);

            _ibutton->reset();
            _ibutton->write(0xD5);        // команда на запись

            for (byte i = 0; i < 8; i++){
                BurnByte(prepareByteToBurn(keyID[i]));

                Serial.print('*');
            } 

            _ibutton->write(rwCmd);                     // send 0xD1 - флаг записи
            _ibutton->write_bit(!rwFlag);               // записываем значение флага записи = 1 - отключаем запись

            delay(10);

            pinMode(_iButtonPin, INPUT);

            // проверяем корректность записи
            if (!isDataSuccessfulyWrited(keyID)){
                Serial.println(" The key copy faild");

                return false;
            }

            Serial.println(" The key has copied successesfully");

            // переводим ключ из формата dallas
            // if (keyType == keyMetacom || keyType == keyCyfral){
            //     _ibutton->reset();

            //     if (keyType == keyCyfral)
            //     _ibutton->write(INIT_CYFRAL);       // send 0xCA - флаг финализации Cyfral
            //     else
            //     _ibutton->write(INIT_METACOM);      // send 0xCA - флаг финализации metacom

            //     // записываем значение флага финализации = 1 - перевезти формат
            //     _ibutton->write_bit(1);

            //     delay(10);

            //     pinMode(_iButtonPin, INPUT);
            // }

            delay(500);

            return true;
        }

        bool isDataSuccessfulyWrited(byte* keyID) {
            byte buff[8];

            if (!_ibutton->reset())
                return false;

            _ibutton->write(READ_ROM_CODE);
            _ibutton->read_bytes(buff, 8);

            // сравниваем код для записи с тем, что уже записано в ключе.
            return isBuffersEquals(keyID, buff, 8);
        }

    protected:
        virtual byte getRwCommand() = 0;
        virtual byte getRwFlag() = 0;
        virtual byte prepareByteToBurn(byte b) = 0;
};

class Key_RW1990_1 : Key_RW1990_1_2_TM01 {
    protected:
        byte getRwCommand() {
            return 0xD1;
        }

        byte getRwFlag() {
            return 0;
        }

        byte prepareByteToBurn(byte b) {
            return ~b;
        }
};

class Key_RW1990_2 : Key_RW1990_1_2_TM01 {
    protected:
        byte getRwCommand() {
            return 0x1D;
        }

        byte getRwFlag() {
            return 1;
        }

        byte prepareByteToBurn(byte b) {
            return b;
        }
};

class Key_TM01 : Key_RW1990_1_2_TM01 {
    protected:
        byte getRwCommand() {
            return 0xC1;
        }

        byte getRwFlag() {
            return 1;
        }

        byte prepareByteToBurn(byte b) {
            return b;
        }
};

#endif