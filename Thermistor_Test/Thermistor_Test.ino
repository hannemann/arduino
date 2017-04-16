

#include <SmoothThermistor.h>
// serial resistor: the small, the higher the measured temperature
SmoothThermistor smoothThermistor(A2, ADC_SIZE_10_BIT, 10000, 8500);
#define SERIAL_BAUD   9600

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(smoothThermistor.temperature());
  delay(500);
}
