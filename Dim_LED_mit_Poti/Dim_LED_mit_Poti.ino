const int POT_IN = 5;
const int OUT = 9;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(POT_IN, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, 0);
}

void loop() {

  int cur = analogRead(POT_IN);
  Serial.println(cur);

  analogWrite(OUT, cur/4);

  delay(60);
}
