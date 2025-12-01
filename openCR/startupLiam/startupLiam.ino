#include "pitches.h"

// Nintendo-style Startup Sound
int melody[] = {
  NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6, NOTE_G5
};

int noteDurations[] = {
  8, 8, 4, 4, 2
};

void setup() {
  for (int thisNote = 0; thisNote < 5; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BDPIN_BUZZER, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    noTone(BDPIN_BUZZER);
  }
}

void loop() {
  // No repeat
}