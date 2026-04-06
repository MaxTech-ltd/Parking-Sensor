#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <NewPing.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#define TRIGGER_PIN  6
#define ECHO_PIN     7
#define MAX_DISTANCE 210   

#define MIN_RANGE 20
#define MAX_RANGE 210

#define HYST 3
#define BAD_LIMIT 3         
#define GOOD_LIMIT 2        

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

float filtered = -1;

uint8_t badCount = 0;
uint8_t goodCount = 0;

bool outOfRange = false;

unsigned long stableTimer = 0;
int stablePrev = -1;
bool stableNow = false;

// --- СОН логика ---
bool sleepModeActive = false;   
uint16_t oorSeconds = 0;        

RF24 radio(9, 10);  
const byte address[6] = "00001";

struct Data {
  int a;
};

// -------------------- WDT будильник --------------------
ISR(WDT_vect) {
}

void setupWDT_1s() {
  MCUSR &= ~(1 << WDRF);
  WDTCSR = (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1);
}

void enterSleep_1s() {
  radio.powerDown();

  setupWDT_1s();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  Serial.flush();     
  sleep_cpu();        
  sleep_disable();

  wdt_disable();

  // -------- ВОССТАНОВЛЕНИЕ ПОСЛЕ СНА --------
  SPI.begin();
  delay(10);

  radio.begin();
  delay(50);          

  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.setRetries(5, 15);
  radio.setCRCLength(RF24_CRC_16);

  radio.openWritingPipe(address);
  radio.stopListening();

  delay(20);          
}
// ------------------------------------------------------

void setup() {
  Serial.begin(9600);

  SPI.begin();

  radio.begin();

  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.setRetries(5, 15);
  radio.setCRCLength(RF24_CRC_16);

  radio.openWritingPipe(address);
  radio.stopListening();

  wdt_disable();   

  Serial.println("TX ready");
}

void loop() {
  //----------------------------------------------JSN-----------------------------------

  unsigned int cm = sonar.ping_cm();

  int OUT = 2222;

  bool isBad = (cm == 0) || (cm < MIN_RANGE) || (cm > MAX_RANGE);

  if (isBad) {
    badCount++;
    goodCount = 0;

    if (badCount >= BAD_LIMIT) {
      outOfRange = true;
      Serial.println("Ping: OUT OF RANGE");
      radio.write(&OUT, sizeof(OUT));
    }

    if (outOfRange) {
      if (oorSeconds < 65000) oorSeconds++;
      if (oorSeconds >= 480) {           
        sleepModeActive = true;
      }
    }
    // ----------------------------------------------------------------------

    if (sleepModeActive) {
      radio.write(&OUT, sizeof(OUT));
      delay(5);
      bool ok = radio.write(&OUT, sizeof(OUT));

      Serial.println(ok ? "OUT sent OK" : "OUT sent FAIL");
      delay(3);

      enterSleep_1s();
    }

    delay(60);
    return;
  }

  goodCount++;
  badCount = 0;

  outOfRange = false;
  sleepModeActive = false;
  oorSeconds = 0;

  if (filtered < 0) filtered = cm;
  else if (abs((int)cm - (int)filtered) > 15) filtered = cm;
  else filtered = filtered * 0.7 + cm * 0.3;

  if (stablePrev < 0) {                
    stablePrev = (int)filtered;
    stableTimer = millis();
    stableNow = false;
  }

  if (abs((int)filtered - stablePrev) > 2) {   
    stablePrev = (int)filtered;
    stableTimer = millis();
    stableNow = false;
  }

  if (millis() - stableTimer >= 4000) {        
    stableNow = true;
  }

  int enter_data = (int)filtered;

  //-------------------------NRF---------------------------------------------------------

  Data data;

  bool warning = (cm < 40);
  int WARR = 1212;

  if (warning) {
    Serial.println("Very Closer");
    delay(50);
    radio.write(&WARR, sizeof(WARR));
    return;
  }

  int SIL = 3333;

  if (stableNow) {
    radio.write(&SIL, sizeof(SIL));
    return;
  }

  data.a = enter_data;

  bool ok = radio.write(&data, sizeof(data));

  Serial.print("Send data ");
  Serial.print(data.a);
  Serial.print("  status=");
  Serial.println(ok ? "OK" : "FAIL");

  delay(20);
}