#ifndef BLANK_KEY_H
#define BLANK_KEY_H

#include "../helpers.h"

// TODO: rename this

class AbstractKey {
    public:
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


#endif