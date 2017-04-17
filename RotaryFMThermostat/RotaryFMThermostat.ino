
#include "LowPower.h"
// encoder libs
#include <ClickEncoder.h>
#include <TimerOne.h>
// RFM Libs
#include <RFM69.h>
#include <RFM69_ATC.h>
// OLED
#include <OLED_I2C_128x64_Monochrome.h>
#include <Wire.h>

#include "ValvesMenu.h"

// peripheral
#define BUTTON_PIN 3
#define BUTTON_INT digitalPinToInterrupt(BUTTON_PIN)
#define TIMOUT_TRSHLD 5000
#define SERIAL_BAUD   115200

// RFM69
#define NODEID        3
#define GATEWAYID     1
#define NETWORKID     100
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network
byte ackCount = 0;
uint32_t packetCount = 0;
const char* COMMAND_GET_VALVES = "V";
const char* END_OF_TRANSMISSION = "EOT";

// encoder
ClickEncoder *encoder;
volatile bool active = true;
unsigned long oldMillis;
int16_t last, value;

byte DEBUG = 0;
const byte DEBUG_RADIO = 0x01;
const byte DEBUG_ROTATION = 0x02;

// OLED
byte DISPLAY_MODE = 0;
const byte DISPLAY_MODE_ON = 0x01;
const byte DISPLAY_MODE_UPDATE = 0x02;
const byte DISPLAY_MODE_CLEAR = 0x04;

#define FILLARRAY(a,n) a[0]=n, memcpy( ((char*)a)+sizeof(a[0]), a, sizeof(a)-sizeof(a[0]) );

byte MODE = 0;
const byte MODE_REQUEST_VALVES = 0x01;
const byte MODE_GET_VALVES = 0x02;
const byte MODE_HAS_VALVES = 0x04;
const byte MODE_SHOW_VALVE = 0x08;

// Menu
ValvesMenu *menu = null;

void setup() {

  //DEBUG |= DEBUG_RADIO;
  DEBUG |= DEBUG_ROTATION;

  Serial.begin(SERIAL_BAUD);
  delay(10);

  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  encoder = new ClickEncoder(A1, A0, BUTTON_PIN, 4);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif

  Timer1.initialize(1000);
  Serial.print("Observing button on Pin "); Serial.print(BUTTON_PIN);
  Serial.print(" (Interrupt "); Serial.print(BUTTON_INT); Serial.println(")");

  // OLED
  lcd.initialize();
  lcd.rotateDisplay180();

  activate();
}

void loop() {

  if (active) {

    if (!(DISPLAY_MODE & DISPLAY_MODE_ON)) {
      DISPLAY_MODE |= DISPLAY_MODE_ON;
      menu = new ValvesMenu();
      writeDisplay();
      lcd.setDisplayOn();
    }

    handleRotation();
    handleClick();

    if (MODE & MODE_REQUEST_VALVES) {
      requestValves();
    }

    receive();

    if (DISPLAY_MODE & DISPLAY_MODE_UPDATE) {
      writeDisplay();
    }

    if (timedOut()) powerDown();
  }
}

/**
   activate
*/
void activate() {
  detachBtn();
  attachEncoder();
  resetValue();
  resetMillis();
  active = true;
  MODE |= MODE_REQUEST_VALVES;

  Serial.println("Active...");
}

/**
   powerDown
*/
void powerDown() {
  attachBtn();
  detachEncoder();
  lcd.clearDisplay();
  lcd.setDisplayOff();
  MODE = 0x00;
  DISPLAY_MODE = 0x00;
  delete menu;
  active = false;
  Serial.println("Sleep...");
  Serial.flush();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void writeDisplay() {

  if (DISPLAY_MODE & DISPLAY_MODE_CLEAR) {
    DISPLAY_MODE &= ~DISPLAY_MODE_CLEAR;
    lcd.clearDisplay();
  }

  if (MODE & MODE_REQUEST_VALVES) {
    lcd.printString("...loading", 0, 0);
    Serial.print("RAM: ");
    Serial.println(freeRam());
  } else if (MODE & MODE_HAS_VALVES) {

    for (byte i = 0; i < menu->length(); i++) {
      if (menu->isCurrent(menu->item(i).index())) {
        lcd.printString(". ", 0, i);
      } else {
        lcd.printString("  ", 0, i);
      }
      lcd.printString(menu->item(i).name(), 2 , i);
    }

  } else if (MODE & MODE_SHOW_VALVE) {

    float wanted = menu->current()->wanted();
    byte x_offset = 5;
    byte x_begin = 0;

    // Header
    char *name = menu->current()->name();
    int n_length = strlen(name);

    lcd.printString("- ", x_begin, 0);
    x_offset = 2;
    lcd.printString(name, x_offset, 0);
    x_offset += n_length;
    lcd.printString(" ", x_offset, 0);
    x_offset++;
    x_offset += lcd.printNumber(menu->current()->real(), 1, x_offset, 0);
    lcd.printString(" ", x_offset, 0);
    x_offset++;
    for (x_offset; x_offset < 16; x_offset++) {
      lcd.printString("-", x_offset, 0);
    }

    // Content
    for (x_offset = 0; x_offset < 6; x_offset++) {
      lcd.printString(" ", x_offset, 3);
    }
    x_offset += lcd.printNumber(wanted, 1, x_offset, 3);
    if (wanted - (int)wanted == 0) {
      lcd.printString(".0", x_offset, 3);
      x_offset += 2;
    }
    for (x_offset; x_offset < 16; x_offset++) {
      lcd.printString(" ", x_offset, 3);
    }

    lcd.printString("----------------", 0, 6);
  }

  DISPLAY_MODE &= ~DISPLAY_MODE_UPDATE;
}

/**
   attach button interrupt
*/
void attachBtn() {
  attachInterrupt(BUTTON_INT, buttonIsr, RISING);
}

/**
   detach button interrupt
*/
void detachBtn() {
  detachInterrupt(BUTTON_INT);
}

/**
   attach encoder timer interrupt
*/
void attachEncoder() {
  Timer1.start();
  Timer1.attachInterrupt(timerIsr);
}

/**
   detach encoder timer interrupt
*/
void detachEncoder() {
  Timer1.stop();
  Timer1.detachInterrupt();
}

/**
   reset stored millis
*/
void resetMillis() {
  oldMillis = millis();
}

/**
   determine if encoder wasnt active
*/
bool timedOut() {
  return millis() - oldMillis > TIMOUT_TRSHLD;
}

/**
   reset encoder value
*/
void resetValue() {
  menu->index(0);
  last = 0;
}

/**
   handle encoder rotation
*/
void handleRotation() {

  if (MODE & MODE_HAS_VALVES) {
    int last = menu->index();
    int index = (*menu) += encoder->getValue();
    if (index != last) {
      resetMillis();
      DISPLAY_MODE |= DISPLAY_MODE_UPDATE;
    }
  } else if (MODE & MODE_SHOW_VALVE) {
    Valve * current = menu->current();
    float last = current->wanted();
    float wanted = (*current) += encoder->getValue();
    if (wanted != last) {
      resetMillis();
      DISPLAY_MODE |= DISPLAY_MODE_UPDATE;
    }
  }
}

/**
   handle encoder click
*/
void handleClick() {

  if (MODE & MODE_HAS_VALVES) {
    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) {
      resetMillis();
      if (b == ClickEncoder::Clicked) {
        MODE &= ~MODE_HAS_VALVES;
        MODE |= MODE_SHOW_VALVE;
        DISPLAY_MODE |= DISPLAY_MODE_CLEAR;
        DISPLAY_MODE |= DISPLAY_MODE_UPDATE;
      }
    }
  }
}

void timerIsr() {
  encoder->service();
}

void buttonIsr() {
  if (!active) {
    activate();
  }
}

void requestValves() {
  Serial.println("Requesting Valves");
  String _payload;
  _payload = "G/V";
  char payload[_payload.length() + 1];
  _payload.toCharArray(payload, sizeof(payload));
  if (radio.sendWithRetry(GATEWAYID, payload, sizeof(payload))) {
    Serial.print("ok!");
  }
  MODE &= ~MODE_REQUEST_VALVES;
}

// Radio
void receive() {

  if (radio.receiveDone())
  {
    resetMillis();
    if (DEBUG & DEBUG_RADIO) {
      Serial.print("Data received: ");
      for (byte i = 0; i < radio.DATALEN; i++) {
        char c = (char)radio.DATA[i];
        Serial.print(c);
      }
      Serial.println();
    }
    if (radio.DATALEN == 1) {
      if (*COMMAND_GET_VALVES == (char)radio.DATA[0]) {
        Serial.println("Fetch valves...");
        MODE |= MODE_GET_VALVES;
      }
    } else {

      char payload[61];
      for (byte i = 0; i < radio.DATALEN; i++) {
        char c = (char)radio.DATA[i];
        payload[i] = c;
      }
      payload[radio.DATALEN] = '\0';

      if (strcmp(payload, END_OF_TRANSMISSION) == 0) {
        if (DEBUG & DEBUG_RADIO) {
          Serial.println("EOT");
        }
        if (MODE & MODE_GET_VALVES) {
          MODE &= ~MODE_GET_VALVES;
          MODE |= MODE_HAS_VALVES;
          DISPLAY_MODE |= DISPLAY_MODE_UPDATE;
          DISPLAY_MODE |= DISPLAY_MODE_CLEAR;
        }
      }

      if (MODE & MODE_GET_VALVES) {
        parseValve(payload);
      }
    }

    if (radio.ACKRequested())
    {
      radio.sendACK();
    }
  }
}

void parseValve(char* payload) {
  int addr = atoi(strtok(payload, "/"));
  char *name = strtok(NULL, "/");
  float wanted = atof(strtok(NULL, "/"));
  float real = atof(strtok(NULL, "/"));
  if (DEBUG & DEBUG_RADIO) {
    Serial.print("Name: ");
    Serial.println(name);
  }
  menu->addItem(name, addr, wanted, real);
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
