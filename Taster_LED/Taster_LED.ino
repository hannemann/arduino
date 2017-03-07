const int PIN_INT = 2;
const int PIN_OUT = 9;
const int INT = 0;
const int bri = 10;
volatile unsigned long mil=0, wait=20;

//#define USE_INTERRUPT

void setup() {
  Serial.begin(115200);
  digitalWrite(13, LOW);
  pinMode(PIN_INT, INPUT_PULLUP);
  pinMode(PIN_OUT, OUTPUT);

  #ifdef USE_INTERRUPT 
  attachInterrupt(INT, isr, CHANGE);
  #endif
}

void loop() {

  #ifdef USE_INTERRUPT
  #else
  int state = digitalRead(PIN_INT);

  //int state = HIGH;
  Serial.println(state);

  if (state != HIGH) {
    analogWrite(PIN_OUT, bri);
  } else {
    analogWrite(PIN_OUT, 0);
  }

  delay(30);
  #endif

}

void isr() {
  
  int state = digitalRead(PIN_INT);

  if((millis() - mil) > wait) { 
  
    Serial.println("Interrupt");
  
    if (LOW == state) {
      analogWrite(PIN_OUT, bri);
    } else {
      analogWrite(PIN_OUT, 0);
    }
    mil = millis();
  }

}
