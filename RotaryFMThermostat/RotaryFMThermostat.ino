
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

#include "SimpleMenu.h"

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
byte ackCount=0;
uint32_t packetCount = 0;
const char* COMMAND_GET_VALVES = "V";
const char* END_OF_TRANSMISSION = "EOT";

// encoder
ClickEncoder *encoder;
volatile bool active = true;
unsigned long oldMillis;
int16_t last, value;

// OLED
bool displayOn = false;

#define FILLARRAY(a,n) a[0]=n, memcpy( ((char*)a)+sizeof(a[0]), a, sizeof(a)-sizeof(a[0]) );

bool updateMenu = false;

unsigned char MODE = 0;
const unsigned char MODE_GET_VALVES = 0x01;

const int MAX_VALVES = 20;
struct valve {
  int addr;
  char *name;
  float wanted;
  float real;
};
struct valve valves[MAX_VALVES];
int valvesCount;

// Menu
SimpleMenu *menu = null;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  encoder = new ClickEncoder(A1, A0, BUTTON_PIN, 4);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif

  Timer1.initialize(1000);
  Serial.print("Observing button on Pin ");Serial.print(BUTTON_PIN);
  Serial.print(" (Interrupt ");Serial.print(BUTTON_INT);Serial.println(")");
  
  // OLED
  lcd.initialize();
  lcd.rotateDisplay180();
  
  activate();
}

void loop() {

  if (active) {

    if (!displayOn) {
      menu = new SimpleMenu();
      writeDisplay();
      lcd.setDisplayOn();
      displayOn = true;
    }
    
    handleRotation();
    handleClick();

    receive();

    if (updateMenu) {
      writeDisplay();
    }
    
    if (timedOut()) powerDown();
  }
}

/**
 * activate
 */
void activate() {
  detachBtn();
  attachEncoder();
  resetValue();
  resetMillis();
  requestValves();
  active = true;
  
  Serial.println("Active...");
}

/**
 * powerDown
 */
void powerDown() {
  attachBtn();
  detachEncoder();
  lcd.setDisplayOff();
  delete menu;
  active = false;
  displayOn = false;
  Serial.println("Sleep...");
  Serial.flush();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
}

void writeDisplay() {

    long val = menu->index();

    lcd.printString("----------------", 0, 0);
    byte x_offset = 5;
    byte x_begin = 0;
    for (x_begin;x_begin<x_offset;x_begin++) {
      lcd.printString(".", x_begin, 3);
    }
    x_offset += lcd.printNumber(val, x_offset, 3);
    for (x_offset;x_offset<16;x_offset++) {
      lcd.printString(",", x_offset, 3);
    }
    lcd.printString("----------------", 0, 6);

    updateMenu = false;
}

/**
 * attach button interrupt
 */
void attachBtn() {
  attachInterrupt(BUTTON_INT, buttonIsr, RISING);
}

/**
 * detach button interrupt
 */
void detachBtn() {
  detachInterrupt(BUTTON_INT);
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
  menu->index(0);
  last = 0;
}

/**
 * handle encoder rotation
 */
void handleRotation() {

  int index = (*menu) += encoder->getValue();
  
  if (index != last) {
    last = index;
    resetMillis();
    Serial.print("Encoder Value: ");
    Serial.println(index);
    updateMenu = true;
  }
}

/**
 * handle encoder click
 */
void handleClick() {
  
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    resetMillis();
    Serial.print("Button: ");
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
        break;
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
  char payload[_payload.length()+1];
  _payload.toCharArray(payload, sizeof(payload));
  if (radio.sendWithRetry(GATEWAYID, payload, sizeof(payload))) {
    Serial.print("ok!");
  }
}

// Radio
void receive() {
  
  if (radio.receiveDone())
  {
    resetMillis();
    if (radio.DATALEN == 1) {
      if (*COMMAND_GET_VALVES == (char)radio.DATA[0]) {
        Serial.println("Fetch valves...");
        struct valve valves[MAX_VALVES];
        MODE |= MODE_GET_VALVES;
        valvesCount = 0;
      }
    } else {

      char payload[61];
      for (byte i = 0; i < radio.DATALEN; i++) {
        char c = (char)radio.DATA[i];
        payload[i] = c;
      }
      payload[radio.DATALEN] = '\0';

      if (strcmp(payload, END_OF_TRANSMISSION) == 0) {
        if (MODE & MODE_GET_VALVES) {
        }
        MODE = 0;
      }

      if (MODE & MODE_GET_VALVES) {
        parseValve(payload, valvesCount);
        valvesCount++;
      }
    }
    
    if (radio.ACKRequested())
    {
      radio.sendACK();
    }
  }
}

void parseValve(char* payload, int i) {
  struct valve v;
  v.addr = atoi(strtok(payload, "/"));
  v.name = strtok(NULL, "/");
  v.wanted = atof(strtok(NULL, "/"));
  v.real = atof(strtok(NULL, "/"));
  valves[i] = v;
  Serial.print("Name: ");
  Serial.println(valves[i].name);
}

