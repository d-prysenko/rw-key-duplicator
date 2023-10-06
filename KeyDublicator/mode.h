class Mode {
    public:
        void tryToSwitchMode(bool keySuccessfulRead) {
            if (_isWriting) {
                _isWriting = false;
                Serial.println("Switched to reading mode");
            } else if (!_isWriting && keySuccessfulRead == true) {
                _isWriting = true;
                Serial.println("Switched to writing mode");
            } else {
                Serial.println("Key wasn't successfuly read, can't switch to writing mode");
            }
        }

        bool isWriting() {
            return _isWriting;
        }

        bool isReading() {
            return !_isWriting;
        }

    private:
        bool _isWriting = false;
};