// Sample RFM69 sender/node sketch, with ACK and optional encryption, and Automatic Transmission Control
// Sends periodic messages of increasing length to gateway (id=1)
// It also looks for an onboard FLASH chip, if present
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash
#include <SPI.h>           //included with Arduino IDE install (www.arduino.cc)
#include <LowPower.h>       
#include <SmoothThermistor.h>

//*********************************************************************************************
// *********** IMPORTANT SETTINGS - YOU MUST CHANGE/ONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
//This part of the code simply sets up the parameters we want the chip to use
// these parameters allow you to have multiple networks, channels, and encryption keys
#define NETWORKID     100  //the same on all nodes that talk to each other
#define RECEIVER      1    //unique ID of the gateway/receiver
#define SENDER        2    // you could for example, have multiple senders
#define NODEID        SENDER  //change to "SENDER" if this is the sender node (the one with the button)
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
//*********************************************************************************************
#define SERIAL_BAUD   9600

//This part defines the LED pin and button pin
#define BUTTON_INT      1 //user button on interrupt 1 (D3)
#define BUTTON_PIN      3 //user button on interrupt 1 (D3)

#define DEBUG false

RFM69 radio;

SmoothThermistor smoothThermistor(A2, ADC_SIZE_10_BIT, 10000, 10160);
#define THERMISTOR_POWER 4

#define READVCC_IN      1
#define READVCC_SWITCH  5
float vout = 0.0;
float vin = 0.0;
float raw_voltage = 0.0;
float R1 = 479600.0;
float R2 = 46230.0;
bod_t BOD = BOD_OFF;

// the setup contains the start-up procedure and some useful serial data
void setup() {
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  Serial.flush();
  pinMode(THERMISTOR_POWER, OUTPUT);
  pinMode(READVCC_SWITCH, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(THERMISTOR_POWER, LOW);
  digitalWrite(READVCC_SWITCH, LOW);
  digitalWrite(BUTTON_PIN, LOW);
  attachInterrupt(BUTTON_INT, handleButton, RISING);
}


//******** THIS IS INTERRUPT BASED DEBOUNCING FOR BUTTON ATTACHED TO D3 (INTERRUPT 1)
#define FLAG_INTERRUPT 0x01
volatile int mainEventFlags = 0;
boolean buttonPressed = false;
void handleButton()
{
  mainEventFlags |= FLAG_INTERRUPT;
}

int is_minute = 12;

void loop() {
  //******** THIS IS INTERRUPT BASED DEBOUNCING FOR BUTTON ATTACHED TO D3 (INTERRUPT 1)
  if (mainEventFlags & FLAG_INTERRUPT)
  {
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD);
    mainEventFlags &= ~FLAG_INTERRUPT;
    if (digitalRead(BUTTON_PIN)) {
      buttonPressed=true;
      digitalWrite(BUTTON_PIN, LOW);
    }
  }

  if (buttonPressed || is_minute == 12)
  {
    is_minute = 0;
    if (DEBUG == true) {
      Serial.println("Button pressed!");
    }
    buttonPressed = false;
    
    String _payload;
    _payload = String(getVoltage(), DEC);
    _payload += "/";
    _payload += getTemperature();

    // turn brown out detection on if voltage drops below 4 Volts
    BOD = vin > 4 ? BOD_OFF : BOD_ON;
    //_payload += BOD == BOD_ON ? " BAT_WARN" : " BAT_OK";
  
    char payload[_payload.length()+1];
    _payload.toCharArray(payload, sizeof(payload));
    
    if (radio.sendWithRetry(RECEIVER, payload, sizeof(payload))) {
      //check if receiver wanted an ACK
      if (radio.ACKRequested())
      {
        radio.sendACK();
        if (DEBUG == true) {
          Serial.print(" - ACK sent");
        }
      }
    }
  }
  is_minute ++;
  radio.sleep();
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD); //sleep Arduino in low power mode (to save battery)
}

float getTemperature() {
    
    digitalWrite(THERMISTOR_POWER, HIGH);
    float t = smoothThermistor.temperature();
    digitalWrite(THERMISTOR_POWER, LOW);
    return t;
}

float getVoltage() {

    digitalWrite(READVCC_SWITCH, HIGH);
    delay(10);
    raw_voltage = analogRead(READVCC_IN);
    vout = (raw_voltage * readVcc() / 1000) / 1024.0;
    digitalWrite(READVCC_SWITCH, LOW);
    vin = vout / (R2/(R1+R2));

    return vin;
}

long readVcc() {
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;
  return result;
}
