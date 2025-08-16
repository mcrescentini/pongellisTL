// ---- Config ----
const byte PIN_R2  = 2;   // rosso
const byte PIN_Y3  = 3;   // giallo
const byte PIN_G4  = 4;   // verde
const byte PIN_R5  = 5;   // rosso
const byte PIN_Y6  = 6;   // giallo
const byte PIN_G7  = 7;   // verde
const byte PIN_R8  = 8;   // rosso
const byte PIN_Y9  = 9;   // giallo
const byte PIN_G10 = 10;  // verde

const byte BUTTON_PIN = 12; // pulsante verso GND (INPUT_PULLUP)

// tempi
const unsigned long TERNA_5S_MS = 5000; // 5 s
const unsigned long FASE_1S_MS  = 1000; // 1 s
const unsigned long BLINK_MS    = 500;  // lampeggio modalità 2
const unsigned long DEBOUNCE_MS = 30;

// Array con tutti i pin LED
const byte ALL_PINS[] = {
  PIN_R2, PIN_Y3, PIN_G4, PIN_R5, PIN_Y6, PIN_G7, PIN_R8, PIN_Y9, PIN_G10
};
const byte N_ALL = sizeof(ALL_PINS) / sizeof(ALL_PINS[0]);

// ---- Stato ----
// mode: 0=spento | 1=loop richiesto | 2=lampeggio 9,3,6
byte mode = 0;

enum Phase : byte {
  PHASE_A = 0,   // 10,4,5 per 5s
  PHASE_B,       // 10,4,6 per 1s (5 OFF, 6 ON)
  PHASE_C,       // 8,7,2 per 5s
  PHASE_D        // 9,3,7 per 1s (8,2 OFF, 7 resta ON)
};
Phase phase = PHASE_A;

unsigned long phaseDeadline = 0;
bool blinkState = false;
unsigned long lastBlink = 0;

// Debounce
bool lastReading = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounce = 0;

// ---- Helper ----
void allOff() {
  for (byte i = 0; i < N_ALL; i++) {
    digitalWrite(ALL_PINS[i], LOW);
  }
}

void onlyTheseOn(const byte pinsOn[], byte countOn) {
  allOff();
  for (byte i = 0; i < countOn; i++) {
    digitalWrite(pinsOn[i], HIGH);
  }
}

// ---- Setup ----
void setup() {
  for (byte i = 0; i < N_ALL; i++) {
    pinMode(ALL_PINS[i], OUTPUT);
  }
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  allOff(); // all’avvio tutto spento
}

// ---- Loop ----
void loop() {
  // --- Debounce pulsante ---
  bool reading = digitalRead(BUTTON_PIN); // HIGH=riposo, LOW=premuto
  if (reading != lastReading) {
    lastDebounce = millis();
  }
  if (millis() - lastDebounce > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) { // click
        // 0 -> 1 -> 2 -> 0 ...
        mode++;
        if (mode > 2) mode = 0;

        // reset stato per la nuova modalità
        if (mode == 0) {
          allOff();
        } else if (mode == 1) {
          // avvio loop: 10,4,5 per 5s
          phase = PHASE_A;
          phaseDeadline = millis() + TERNA_5S_MS;
          const byte onA[] = {PIN_G10, PIN_G4, PIN_R5};
          onlyTheseOn(onA, 3);
        } else if (mode == 2) {
          // lampeggio 9,3,6
          allOff();
          blinkState = false;
          lastBlink = millis();
        }
      }
    }
  }
  lastReading = reading;

  // --- Logica modalità ---
  if (mode == 1) {
    // avanzamento fasi non bloccante
    if ((long)(millis() - phaseDeadline) >= 0) {
      switch (phase) {
        case PHASE_A: {
          // Passa a PHASE_B: 10,4,6 per 1s (5 OFF, 6 ON)
          const byte onB[] = {PIN_G10, PIN_G4, PIN_Y6};
          onlyTheseOn(onB, 3);
          phase = PHASE_B;
          phaseDeadline = millis() + FASE_1S_MS;
        } break;

        case PHASE_B: {
          // Passa a PHASE_C: 8,7,2 per 5s (10,6,4 OFF)
          const byte onC[] = {PIN_R8, PIN_G7, PIN_R2};
          onlyTheseOn(onC, 3);
          phase = PHASE_C;
          phaseDeadline = millis() + TERNA_5S_MS;
        } break;

        case PHASE_C: {
          // Passa a PHASE_D: 9,3,7 per 1s (8,2 OFF; 7 resta ON)
          const byte onD[] = {PIN_Y9, PIN_Y3, PIN_G7};
          onlyTheseOn(onD, 3);
          phase = PHASE_D;
          phaseDeadline = millis() + FASE_1S_MS;
        } break;

        case PHASE_D: {
          // Spegne 9,3,7 e ricomincia da PHASE_A: 10,4,5 per 5s
          const byte onA[] = {PIN_G10, PIN_G4, PIN_R5};
          onlyTheseOn(onA, 3);
          phase = PHASE_A;
          phaseDeadline = millis() + TERNA_5S_MS;
        } break;
      }
    }
  }
  else if (mode == 2) {
    // lampeggio 9,3,6
    if (millis() - lastBlink >= BLINK_MS) {
      lastBlink = millis();
      blinkState = !blinkState;

      // lascia tutti gli altri spenti
      digitalWrite(PIN_R2,  LOW);
      digitalWrite(PIN_G4,  LOW);
      digitalWrite(PIN_R5,  LOW);
      digitalWrite(PIN_G7,  LOW);
      digitalWrite(PIN_R8,  LOW);
      digitalWrite(PIN_G10, LOW);

      digitalWrite(PIN_Y9, blinkState ? HIGH : LOW);
      digitalWrite(PIN_Y3, blinkState ? HIGH : LOW);
      digitalWrite(PIN_Y6, blinkState ? HIGH : LOW);
    }
  }
  // mode 0: non fa nulla (resta tutto spento)
}
