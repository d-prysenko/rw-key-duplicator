#ifndef BUTTON_H
#define BUTTON_H

class Button {
    public:
        Button() {}
        Button(byte pin)
            : _btnPin(pin)
        {
            pinMode(_btnPin, INPUT_PULLUP);
        }

        bool isPressed() {
            bool currentBtnState = digitalRead(_btnPin);
            bool isClicked;

            if (_prevBtnState == HIGH && currentBtnState == LOW)
                isClicked = true;
            else
                isClicked = false;

            _prevBtnState = currentBtnState;

            return isClicked;
        }
    private:
        byte _btnPin = 0;
        bool _prevBtnState = LOW;
};

#endif