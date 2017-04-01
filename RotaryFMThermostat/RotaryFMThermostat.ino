
#include "LowPower.h"
// encoder libs
#include <ClickEncoder.h>
#include <TimerOne.h>
// RFM Libs
#include <RFM69.h>
#include <RFM69_ATC.h>

// peripheral 
#define BUTTON_PIN 3
#define BUTTON_INT digitalPinToInterrupt(BUTTON_PIN)
#define TIMOUT_TRSHLD 5000
#define SERIAL_BAUD   115200

// RFM69
#define NODEID        1
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

// encoder
ClickEncoder *encoder;
volatile bool active = true;
unsigned long oldMillis;
int16_t last, value;

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
  activate();
}

void loop() {

  if (active) {
    handleRotation();
    handleClick();

    receive();
    
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
  active = true;
  Serial.println("Active...");
}

/**
 * powerDown
 */
void powerDown() {
  attachBtn();
  detachEncoder();
  active = false;
  Serial.println("Sleep...");
  Serial.flush();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
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
  value = 0;
  last = value;
}

/**
 * handle encoder rotation
 */
void handleRotation() {

  value += encoder->getValue();
  
  if (value != last) {
    last = value;
    resetMillis();
    Serial.print("Encoder Value: ");
    Serial.println(value);
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

// Radio
void receive() {
  
  if (radio.receiveDone())
  {
    Serial.print("#[");
    Serial.print(++packetCount);
    Serial.print(']');
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    if (promiscuousMode)
    {
      Serial.print("to [");Serial.print(radio.TARGETID, DEC);Serial.print("] ");
    }
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
    
    if (radio.ACKRequested())
    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();
      Serial.print(" - ACK sent.");

      // When a node requests an ACK, respond to the ACK
      // and also send a packet requesting an ACK (every 3rd one only)
      // This way both TX/RX NODE functions are tested on 1 end at the GATEWAY
      if (ackCount++%3==0)
      {
        Serial.print(" Pinging node ");
        Serial.print(theNodeID);
        Serial.print(" - ACK...");
        delay(3); //need this when sending right after reception .. ?
        if (radio.sendWithRetry(theNodeID, "ACK TEST", 8, 0))  // 0 = only 1 attempt, no retries
          Serial.print("ok!");
        else Serial.print("nothing");
      }
    }
    Serial.println();
  }
}

