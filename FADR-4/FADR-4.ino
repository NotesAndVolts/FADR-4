/******************************
   FADR-4 v1.0.0
   for Teensy LC (www.pjrc.com)
   by Notes and Volts
   www.notesandvolts.com
 ******************************/

/**************************************
   ** Upload Settings **
   Board: "Teensy LC"
   USB Type: "MIDI"
   CPU Speed: "48 Mhz"
 **************************************/

#include <LedControl.h>
#include <EEPROM.h>

#define EEPROM_KEY 200
#define LED_LEVEL 2

#define EDIT_BUTTON 12

LedControl mydisplay = LedControl(4, 2, 3, 1);

byte memStart = 2; // Start of Faders in EEPROM
byte bank = 0;
int oldValue[8];
int pins[] = {A0, A1, A2, A3, A4, A5, A6, A7};
byte cc[][8] = {
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8},
  {1, 2, 3, 4, 5, 6, 7, 8}
};
byte mChan[] = {1, 2, 3, 4, 5, 6, 7, 8};
byte val = 0;

void setup() {
  pinMode(EDIT_BUTTON, INPUT_PULLUP); //Edit Button
  mydisplay.setIntensity(0, LED_LEVEL); // 15 = brightest
  mydisplay.shutdown(0, false);  // turns on display
  mydisplay.setChar(0, 0, 8, true);
  mydisplay.setChar(0, 1, 8, true);
  mydisplay.setDigit(0, 2, 8, true);

  initRom();
  readRom();
  if (digitalRead(EDIT_BUTTON) == LOW) {
    mydisplay.setDigit(0, 0, 1, true); // Version 1.0.0
    mydisplay.setDigit(0, 1, 0, true);
    mydisplay.setDigit(0, 2, 0, false);
    delay(8000);
  }
  if (digitalRead(EDIT_BUTTON) == LOW) {
    showRom();
  }
  delay(2000);
  faderReset();
}

void loop() {
  usbMIDI.read();
  switch (checkButton()) {
    case 1:
      displayMode();
      break;
    case 2:
      channelEdit();
      break;
  }
  for (int x = 0; x < 4; x++) {
    val = getFaderValue(x);
    if (val < 255) {
      usbMIDI.sendControlChange(cc[bank][x], (val), mChan[0]);
      threeDigit(val);
    }
  }
}

void faderReset() {
  for (int x = 0; x < 4; x++) {
    val = getFaderValue(x);
    if (val < 255) {
    }
  }
  mydisplay.setChar(0, 0, '-', false);
  mydisplay.setChar(0, 1, '-', false);
  mydisplay.setChar(0, 2, '-', false);
}

void channelEdit() {
  int channelVal = mChan[0];
  chanDigit(channelVal);

  while (checkButton() != 1) {
    usbMIDI.read();
    for (int x = 0; x < 4; x++) {
      val = getFaderValue(x);
      if (val < 255) {
        channelVal = (val >> 3) + 1;
        chanDigit(channelVal);
      }
    }
  }
  mChan[0] = channelVal;
  if (mChan[0] != EEPROM.read(1)) EEPROM.write(1, mChan[0]);
  mydisplay.setIntensity(0, 0);
  delay(100);
  mydisplay.setIntensity(0, LED_LEVEL);
  delay(100);
  mydisplay.setIntensity(0, 0);
  delay(100);
  mydisplay.setIntensity(0, LED_LEVEL);
  delay(100);

  faderReset();
}

void displayMode() {
  unsigned long buttonMillis;
  unsigned long currentMillis;
  unsigned long delayMillis;
  byte faderSelected = 1;
  bool toggle = true;
  byte buttonStat = 0;

  buttonMillis = millis();
  delayMillis = buttonMillis;//TEST

  while (1) {
    usbMIDI.read();
    currentMillis = millis();
    buttonStat = checkButton();
    if (buttonStat == 1) {
      faderSelected++;
      if (faderSelected > 4) faderSelected = 1;
      mydisplay.setChar(0, 0, 'F', false);
      mydisplay.setDigit(0, 1, faderSelected, false);
      mydisplay.setChar(0, 2, ' ', false);
      buttonMillis = millis();
      delayMillis = buttonMillis;
      currentMillis = buttonMillis;
      toggle = false;
    }

    else if (buttonStat == 2) {
      threeDigit(cc[bank][faderSelected - 1]);
      faderEdit(faderSelected);
      buttonMillis = millis();
      delayMillis = buttonMillis;
      currentMillis = buttonMillis;
      toggle = false;
    }

    if (currentMillis - delayMillis >= 1000) {
      if (toggle == true) {
        mydisplay.setChar(0, 0, 'F', false);
        mydisplay.setDigit(0, 1, faderSelected, false);
        mydisplay.setChar(0, 2, ' ', false);
      }
      else if (toggle == false) {
        threeDigit(cc[bank][faderSelected - 1]);
      }
      toggle = !toggle;
      delayMillis = millis();
    }

    if (currentMillis - buttonMillis > 8000) {
      faderReset();
      return;
    }
  }
}

void faderEdit(byte fader) {
  unsigned long delayMillis;
  bool toggle = true;
  byte temp = cc[0][fader - 1];

  delayMillis = millis();
  while (checkButton() == 2) {
    usbMIDI.read();
    if (millis() - delayMillis >= 100) {
      if (toggle == true) mydisplay.setIntensity(0, 0);
      else mydisplay.setIntensity(0, LED_LEVEL);
      delayMillis = millis();
      toggle = !toggle;
    }
    for (int x = 0; x < 4; x++) {
      val = getFaderValue(x);
      if (val < 255) {
        threeDigit(val);
        temp = val;
        cc[0][fader - 1] = val;
      }
    }
  }
  if (temp != EEPROM.read((fader - 1) + memStart)) EEPROM.write((fader - 1) + memStart, temp); //Write new CC to EEProm
  mydisplay.setIntensity(0, LED_LEVEL);
  return;
}

void threeDigit(int number) {
  int first = number / 100;
  int secon = number % 100 / 10;
  int third = number % 10;
  mydisplay.setDigit(0, 2, third, false);
  mydisplay.setDigit(0, 1, secon, false);
  mydisplay.setDigit(0, 0, first, false);
}

void chanDigit(int number) {
  int secon = number % 100 / 10;
  int third = number % 10;
  mydisplay.setDigit(0, 2, third, false);
  mydisplay.setDigit(0, 1, secon, false);
  mydisplay.setChar(0, 0, 'C', false);

}

int getFaderValue(int pin) {
  int value = analogRead(pins[pin]);
  int tmp = (oldValue[pin] - value);
  if (tmp >= 8 || tmp <= -8) {
    value = analogRead(pins[pin]);
    tmp = (oldValue[pin] - value);
    if ((tmp >= 8) || (tmp <= -8)) {
      if (value == 8) oldValue[pin] = value + 1;// Zero Fix
      else oldValue[pin] = value;
      if (value < 8) return 0; //test
      else return (value >> 3); //test
    }
  }
  return 255;
}

void editBank() {
  bank++;
  if (bank > 7) bank = 0;
  mydisplay.clearDisplay(0);
  mydisplay.setChar(0, 0, 'B', false);
  mydisplay.setChar(0, 2, (bank + 1), false);
  delay(500);
}

void readRom() {
  mChan[0] = EEPROM.read(1); //Read Midi Channel

  for (int bank = 0; bank < 8; bank++) {
    for (int fader = 0; fader < 8; fader++) {
      cc[bank][fader] = EEPROM.read((bank * 8) + (fader + memStart));
    }
  }
}

void initRom() {
  byte key = 102; // First CC Number

  if (EEPROM.read(0) != EEPROM_KEY) {
    EEPROM.write(0, EEPROM_KEY); // Key

    EEPROM.write(1, 1); // Set Midi channel to 1

    for (int bank = 0; bank < 8; bank++) {
      for (int fader = 0; fader < 8; fader++) {
        EEPROM.write((bank * 8) + (fader + memStart), key);
        key++;
      }
    }
    mydisplay.setChar(0, 0, ' ', true);
    mydisplay.setChar(0, 1, ' ', true);
    mydisplay.setChar(0, 2, ' ', true);
    delay(1000);
  }
}

byte checkButton() {
  const int debounce = 50;
  const int longPress = 2000;
  static byte buttonState = 0;
  static unsigned long buttonMillis;
  static unsigned long currentMillis;

  switch (buttonState) {
    case 0: // Nothing
      if (digitalRead(EDIT_BUTTON) == LOW) {
        buttonState = 1;
        buttonMillis = millis();
      }
      return 0;
      break;
    case 1: // Test
      currentMillis = millis();
      if (currentMillis - buttonMillis > debounce) {
        if (digitalRead(EDIT_BUTTON) == LOW) buttonState = 2;
        else buttonState = 0;
      }
      return 0;
      break;
    case 2: // Valid
      currentMillis = millis();
      if (digitalRead(EDIT_BUTTON) == LOW) {
        if (currentMillis - buttonMillis > longPress) {
          buttonState = 3;
          return 2;
        }
      }
      else {
        buttonState = 0;
        return 1;
      }
      return 0;
      break;
    case 3: // Long
      if (digitalRead(EDIT_BUTTON) == HIGH) {
        buttonState = 0;
        return 0;
      }
      else return 2;
      break;
    default:
      buttonState = 0;
      return 0;
      break;
  }
}

void showRom() {
  for (int i = 0; i < 6; i++) {
    threeDigit(EEPROM.read(i));
    delay(2000);
  }
}
