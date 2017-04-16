#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>           //included with Arduino IDE install (www.arduino.cc)

#define NODEID        1    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define SERIAL_BAUD   115200

RFM69_ATC radio;

const int SERIAL_BUFFER_SIZE = 200;
char serial_buffer[SERIAL_BUFFER_SIZE];

const char* COMMAND_GET_VALVES = "V";
const char* END_OF_TRANSMISSION = "EOT";

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
}

byte ackCount=0;
uint32_t packetCount = 0;
void loop() {

  if (read_serial()) {
    parse_serial();
    
  }

  if (radio.receiveDone())
  {
    Serial.print(radio.SENDERID, DEC);Serial.print(" ");
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




bool read_serial()
{
  static byte index;

  while (Serial.available())
  {
    char c = Serial.read();

    if (c >= 32 && index < SERIAL_BUFFER_SIZE)
    {
      serial_buffer[index++] = c;
    }
    else if (c == '\n' && index > 0)
    {
      serial_buffer[index] = '\0';
      index = 0;
      return true;
    }
  }
  return false;
}

void parse_serial() {
  int node = atoi(strtok(serial_buffer, "|"));
  char *cmd = strtok(NULL, "|");
  char *payload = strtok(NULL, "\n");

  Serial.print(F("node = ")); Serial.println(node);
  Serial.print(F("cmd = ")); Serial.println(cmd);
  Serial.print(F("payload = ")); Serial.println(payload);

  if (strcmp(cmd, COMMAND_GET_VALVES) == 0) {
    send_valves(payload, node);
  }
}

void send_valves(char* valves, int node) {
    Serial.print("Send Valves to node ");Serial.println(node);
    char *valve = strtok(valves, "|");
    const char *eol = "\n";
    const char *term = "\0";
    radio.sendWithRetry(node, COMMAND_GET_VALVES, strlen(COMMAND_GET_VALVES));
    while (valve != NULL) {
      Serial.println(valve);
      radio.sendWithRetry(node, valve, strlen(valve));
      valve = strtok(NULL, "|");
    }
    radio.sendWithRetry(node, END_OF_TRANSMISSION, strlen(END_OF_TRANSMISSION));
}

