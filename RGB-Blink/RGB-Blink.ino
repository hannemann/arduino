int rled = 3;
int gled = 5;
int bled = 6;

void setup() {
  pinMode(rled, OUTPUT);
  pinMode(gled, OUTPUT);
  pinMode(bled, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(rled, HIGH);
  delay(200);
  digitalWrite(rled, LOW);
  delay(200);
  digitalWrite(gled, HIGH);
  delay(200);
  digitalWrite(gled, LOW);
  delay(200);
  digitalWrite(bled, HIGH);
  delay(200);
  digitalWrite(bled, LOW);
  delay(1000);
}
