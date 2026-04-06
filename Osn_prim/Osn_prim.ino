#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ===================== NRF =====================
RF24 radio(9, 10);   
const byte address[6] = "00001";

struct Data {
  int a;
};

// ===================== BUZZER =====================
#define BUZZ_PIN 4

unsigned long lastBeep = 0;
bool buzzState = false;

// ===================== DISPLAY 74HC595 =====================
const int dataPin  = 7;
const int clockPin = 8;
const int latchPin = 6;

const int digPins[4] = {3, 5, A1, A2};


const byte LEFT_NUM_DIG   = 0;
const byte RIGHT_NUM_DIG  = 1;
const byte LEFT_BAR_DIG   = 3;
const byte RIGHT_BAR_DIG  = 2;


const byte SEG_A  = 5;
const byte SEG_B  = 0;
const byte SEG_C  = 3;
const byte SEG_D  = 1;
const byte SEG_E  = 2;
const byte SEG_F  = 6;
const byte SEG_G  = 4;
const byte SEG_DP = 7;


const byte BAR_G1 = 5;
const byte BAR_G2 = 0;
const byte BAR_G3 = 3;

const byte BAR_Y1 = 1;
const byte BAR_Y2 = 2;

const byte BAR_R1 = 6;
const byte BAR_R2 = 4;

// ===================== GLOBAL DATA =====================
int currentDistanceCm = 3333;   
bool testDone = false;          

unsigned long lastRadioTime = 0;
bool radioConnected = false;

const unsigned long RADIO_LOST_MS = 1000;         
const unsigned long DISPLAY_OFF_MS = 180000UL;    

// ===================== DISPLAY FUNCTIONS =====================
void send595(byte value) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, value);
  digitalWrite(latchPin, HIGH);
}

void allDigitsOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(digPins[i], LOW);   
  }
}

void showRaw(byte dig, byte segMask) {
  allDigitsOff();
  send595(segMask);
  digitalWrite(digPins[dig], HIGH);
  delay(2);
  digitalWrite(digPins[dig], LOW);
}

void blankDisplay() {
  allDigitsOff();
  send595(0xFF);
}

byte makeMask(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool dp = false) {
  byte v = 0xFF;

  if (a)  v &= ~(1 << SEG_A);
  if (b)  v &= ~(1 << SEG_B);
  if (c)  v &= ~(1 << SEG_C);
  if (d)  v &= ~(1 << SEG_D);
  if (e)  v &= ~(1 << SEG_E);
  if (f)  v &= ~(1 << SEG_F);
  if (g)  v &= ~(1 << SEG_G);
  if (dp) v &= ~(1 << SEG_DP);

  return v;
}

byte digitCode(byte n, bool dp = false) {
  switch (n) {
    case 0: return makeMask(1,1,1,1,1,1,0,dp);
    case 1: return makeMask(0,1,1,0,0,0,0,dp);
    case 2: return makeMask(1,1,0,1,1,0,1,dp);
    case 3: return makeMask(1,1,1,1,0,0,1,dp);
    case 4: return makeMask(0,1,1,0,0,1,1,dp);
    case 5: return makeMask(1,0,1,1,0,1,1,dp);
    case 6: return makeMask(1,0,1,1,1,1,1,dp);
    case 7: return makeMask(1,1,1,0,0,0,0,dp);
    case 8: return makeMask(1,1,1,1,1,1,1,dp);
    case 9: return makeMask(1,1,1,1,0,1,1,dp);
  }
  return 0xFF;
}

byte barMaskByDistance(int cm) {
  byte v = 0xFF;

  
  if (cm == 1212) {
    v &= ~(1 << BAR_R1);
    v &= ~(1 << BAR_R2);
    return v;
  }

  
  if (cm >= 160) {
    return v;
  }

  
  if (cm > 100) {
    v &= ~(1 << BAR_G1);
    v &= ~(1 << BAR_G2);
    v &= ~(1 << BAR_G3);
  }
  
  else if (cm > 60) {
    v &= ~(1 << BAR_Y1);
    v &= ~(1 << BAR_Y2);
  }
  
  else {
    v &= ~(1 << BAR_R1);
    v &= ~(1 << BAR_R2);
  }

  return v;
}

void showMeters10WithBars(int distanceCm) {
  if (distanceCm == 2222 || distanceCm == 3333 || distanceCm < 0) {
    blankDisplay();
    return;
  }

  
  if (distanceCm == 1212) {
    showRaw(LEFT_NUM_DIG,  digitCode(0, true));
    showRaw(RIGHT_NUM_DIG, digitCode(0, false));
    showRaw(LEFT_BAR_DIG,  barMaskByDistance(1212));
    showRaw(RIGHT_BAR_DIG, barMaskByDistance(1212));
    return;
  }

  
  if (distanceCm >= 160) {
    blankDisplay();
    return;
  }

  int value10 = round(distanceCm / 10.0);   // 123 см -> 12 -> 1.2

  if (value10 < 0)  value10 = 0;
  if (value10 > 99) value10 = 99;

  int leftDigit  = value10 / 10;
  int rightDigit = value10 % 10;

  showRaw(LEFT_NUM_DIG,  digitCode(leftDigit, true));
  showRaw(RIGHT_NUM_DIG, digitCode(rightDigit, false));
  showRaw(LEFT_BAR_DIG,  barMaskByDistance(distanceCm));
  showRaw(RIGHT_BAR_DIG, barMaskByDistance(distanceCm));
}

// ===================== BUZZER =====================
void updateBuzzer(int dist) {
  if (dist == 1212 || (dist >= 0 && dist < 60)) {
    digitalWrite(BUZZ_PIN, HIGH);
    buzzState = true;
  }
  
  else if (dist >= 60 && dist < 100) {
    if (millis() - lastBeep > (buzzState ? 30 : 120)) {
      lastBeep = millis();
      buzzState = !buzzState;
      digitalWrite(BUZZ_PIN, buzzState);
    }
  }
  
  else if (dist >= 100 && dist < 160) {
    if (millis() - lastBeep > (buzzState ? 40 : 300)) {
      lastBeep = millis();
      buzzState = !buzzState;
      digitalWrite(BUZZ_PIN, buzzState);
    }
  }
  
  else {
    digitalWrite(BUZZ_PIN, LOW);
    buzzState = false;
  }
}

// ===================== RADIO =====================
void readRadio() {
  while (radio.available()) {
    Data data;
    radio.read(&data, sizeof(data));

    currentDistanceCm = data.a;
    lastRadioTime = millis();
    radioConnected = true;

    Serial.print("Received a = ");
    Serial.println(currentDistanceCm);
  }

  
  if (millis() - lastRadioTime > RADIO_LOST_MS) {
    radioConnected = false;
  }
}

// ===================== START SHOW =====================
const byte segOrder[8] = {
  SEG_A, SEG_B, SEG_C, SEG_D,
  SEG_E, SEG_F, SEG_G, SEG_DP
};

byte oneSeg(byte segBit) {
  byte v = 0xFF;
  v &= ~(1 << segBit);
  return v;
}

void startupSegmentShow() {
  for (int s = 0; s < 8; s++) {
    unsigned long t0 = millis();

    while (millis() - t0 < 80) {
      for (int d = 0; d < 4; d++) {
        showRaw(d, oneSeg(segOrder[s]));
      }
    }
  }

  blankDisplay();
  delay(1500);
}

// ===================== NO RADIO ANIMATION =====================
const byte circleSegs[6] = {
  SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F
};

void showNoRadioAnimation() {
  static int step = 0;
  static unsigned long lastStep = 0;

  if (millis() - lastStep > 80) {
    lastStep = millis();
    step++;
    if (step >= 6) step = 0;
  }

  byte mask = oneSeg(circleSegs[step]);

  showRaw(LEFT_NUM_DIG, mask);
  showRaw(RIGHT_NUM_DIG, mask);
}

void setup() {
  Serial.begin(9600);

  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(digPins[i], OUTPUT);
  }

  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(BUZZ_PIN, LOW);

  radio.begin();
  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.setRetries(5, 15);
  radio.setCRCLength(RF24_CRC_16);
  radio.openReadingPipe(1, address);
  radio.startListening();

  blankDisplay();
  Serial.println("RX + Display ready");

  lastRadioTime = millis();
}

void loop() {
  if (!testDone) {
    startupSegmentShow();
    testDone = true;
    lastRadioTime = millis(); 
  }

  readRadio();

  if (!radioConnected) {
    digitalWrite(BUZZ_PIN, LOW);
    buzzState = false;

    if (millis() - lastRadioTime >= DISPLAY_OFF_MS) {
      blankDisplay();
    } else {
      showNoRadioAnimation();
    }

    return;
  }

  updateBuzzer(currentDistanceCm);
  showMeters10WithBars(currentDistanceCm);
}