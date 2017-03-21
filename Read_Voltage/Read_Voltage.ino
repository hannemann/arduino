
#define READVCC_IN      1
#define SERIAL_BAUD   9600
#define BUTTON_INT      1 //user button on interrupt 1 (D3)
#define BUTTON_PIN      3 //user button on interrupt 1 (D3)
float vout = 0.0;
float vin = 0.0;
float raw_voltage = 0.0;
float R1 = 46230.0;
float R2 = 4757.31;


void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(READVCC_IN, INPUT);
}

void loop() {
  /*
  Serial.println( readVcc(), DEC );
  Serial.println( analogRead(READVCC_IN), DEC );
  */
  raw_voltage = analogRead(READVCC_IN);
  vout = (raw_voltage * readVcc() / 1000) / 1024.0;
  //Serial.println(vout, DEC);
  vin = vout / (R2/(R1+R2));

  String _payload;

  _payload = String(vin, DEC);

  char payload[_payload.length()+1];
  _payload.toCharArray(payload, sizeof(payload));

  Serial.println(payload);
  delay(100);
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
