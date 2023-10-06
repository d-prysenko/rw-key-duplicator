#ifndef KEY_TM2004_H
#define KEY_TM2004_H

#include "BlankKey.h"

class Key_TM2004 : public AbstractKey {
    public:
        Key_TM2004 (OneWire* ibutton, byte iButtonPin)
            : AbstractKey(ibutton, iButtonPin)
        {}

        bool write(byte *keyID) {
            byte answer;

            _ibutton->reset();

            _ibutton->write(0x3C);                // команда записи ROM для TM-2004    
            _ibutton->write(0x00);
            _ibutton->write(0x00);                // передаем адрес с которого начинается запись

            for (byte i = 0; i < 8; i++){
                _ibutton->write(keyID[i]);

                answer = _ibutton->read();

                //if (OneWire::crc8(m1, 3) != answer){result = false; break;}     // crc не верный
                // испульс записи

                delayMicroseconds(600);

                _ibutton->write_bit(1);

                delay(50);

                pinMode(_iButtonPin, INPUT);

                Serial.print('*');

                //читаем записанный байт и сравниваем, с тем что должно записаться
                if (keyID[i] != _ibutton->read()) {
                    _ibutton->reset();
                    Serial.println(" The key copy faild");

                    return false;
                }
            }

            _ibutton->reset();

            Serial.println(" The key has copied successesfully");

            delay(500);

            return true;
        }
};

#endif