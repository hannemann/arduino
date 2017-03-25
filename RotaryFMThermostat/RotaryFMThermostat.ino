//#define WITH_LCD 1

#include <ClickEncoder.h>
#include <TimerOne.h>

#ifdef WITH_LCD
#include <LiquidCrystal.h>

#define LCD_RS       8
#define LCD_RW       9
#define LCD_EN      10
#define LCD_D4       4
#define LCD_D5       5
#define LCD_D6       6
#define LCD_D7       7

#define LCD_CHARS   20
#define LCD_LINES    4

LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#endif

// begin 
#define btnPin 2
#define TIMOUT_TRSHLD 5000

volatile bool active = false;
bool btnPressed = false;
int16_t oldValue;
unsigned long oldMillis;
int btnInt = digitalPinToInterrupt(btnPin);

ClickEncoder *encoder;
int16_t last, value;

void timerIsr() {
  encoder->service();
}

void buttonIsr() {
  if (!active) {
    activate(); 
  }
}

#ifdef WITH_LCD
void displayAccelerationStatus() {
  lcd.setCursor(0, 1);  
  lcd.print("Acceleration ");
  lcd.print(encoder->getAccelerationEnabled() ? "on " : "off");
}
#endif

void setup() {
  Serial.begin(115200);
  encoder = new ClickEncoder(A1, A0, btnPin, 4);

#ifdef WITH_LCD
  lcd.begin(LCD_CHARS, LCD_LINES);
  lcd.clear();
  displayAccelerationStatus();
#endif

  Timer1.initialize(1000);
  Timer1.stop();
  //Timer1.attachInterrupt(timerIsr); 
  attachBtn();

  Serial.println(btnPin);
  Serial.println(btnInt);
  Serial.println(pinMode(btnPin) == INPUT_PULLUP ? "PULLUP" : "OTHER");
  
  resetMillis();
  
  last = -1;
}

void loop() {

  if (active) {
  
    value += encoder->getValue();
    
    if (value != last) {
      last = value;
      Serial.print("Encoder Value: ");
      Serial.println(value);
  #ifdef WITH_LCD
      lcd.setCursor(0, 0);
      lcd.print("         ");
      lcd.setCursor(0, 0);
      lcd.print(value);
  #endif
    }
    
    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) {
      Serial.print("Button: ");
      btnPressed = true;
      #define VERBOSECASE(label) case label: Serial.println(#label); break;
      switch (b) {
        VERBOSECASE(ClickEncoder::Pressed);
        VERBOSECASE(ClickEncoder::Held)
        VERBOSECASE(ClickEncoder::Released)
        VERBOSECASE(ClickEncoder::Clicked)
        case ClickEncoder::DoubleClicked:
            Serial.println("ClickEncoder::DoubleClicked");
            encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
            Serial.print("  Acceleration is ");
            Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
  #ifdef WITH_LCD
            displayAccelerationStatus();
  #endif
          break;
      }
    }    
  }

  if (value != oldValue) {
    resetMillis();
    oldValue = value;
  }

  if (btnPressed) {
    resetMillis();
    btnPressed = false;
  }

  if (active && timedOut()) {
    deactivate();
  }
}

/**
 * activate
 */
void activate() {
  detachBtn();
  attachEncoder();
  resetValue();
  active = true;
  resetMillis();
  Serial.print("Awake after ");
  Serial.println(oldMillis);
}

/**
 * deactivate
 */
void deactivate() {
  attachBtn();
  detachEncoder();
  active = false;
  Serial.println("Sleep...");
}

/**
 * attach button interrupt
 */
void attachBtn() {
  attachInterrupt(btnInt, buttonIsr, FALLING);
}

/**
 * detach button interrupt
 */
void detachBtn() {
  detachInterrupt(btnInt);
}

/**
 * attach encoder timer interrupt
 */
void attachEncoder() {
  Timer1.start();
  Timer1.attachInterrupt(timerIsr);
}

/**
 * detach encoder timer interrupt
 */
void detachEncoder() {
  Timer1.stop();
  Timer1.detachInterrupt();
}

/**
 * reset stored millis
 */
void resetMillis() {
  oldMillis = millis();
}

/**
 * determine if encoder wasnt active
 */
bool timedOut() {
  return millis() - oldMillis > TIMOUT_TRSHLD;
}

/**
 * reset encoder value
 */
void resetValue() {
  value = 0;
  oldValue = value;
}

// helper
int pinMode(uint8_t pin)
{
  if (pin >= NUM_DIGITAL_PINS) return (-1);

  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint8_t *reg = portModeRegister(port);
  if (*reg & bit) return (OUTPUT);

  volatile uint8_t *out = portOutputRegister(port);
  return ((*out & bit) ? INPUT_PULLUP : INPUT);
}

