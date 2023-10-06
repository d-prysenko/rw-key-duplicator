
#define READ_ROM_CODE 0x33

#define DALLAS_FAMILY_CODE 0x01

#define INIT_CYFRAL 0xCA
#define INIT_METACOM 0xCB

void printHex(byte* buf, byte length) {
  for (byte i = 0; i < length; i++) {
    Serial.print(buf[i], HEX);
    Serial.print(":");
  }
}

bool isBuffersEquals(byte* buf1, byte* buf2, byte length) {
  for (byte i = 0; i < length; i++)
    if (buf1[i] != buf2[i])
      return false;

  return true;
}