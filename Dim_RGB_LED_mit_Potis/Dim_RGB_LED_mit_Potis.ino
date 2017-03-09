const int R_IN = 0;
const int G_IN = 1;
const int B_IN = 2;
const int R_OUT = 9;
const int G_OUT = 10;
const int B_OUT = 11;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(R_IN, INPUT);
  pinMode(G_IN, INPUT);
  pinMode(B_IN, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, 0);
}

void loop() {

  int r = analogRead(R_IN);
  int g = analogRead(G_IN);
  int b = analogRead(B_IN);
  Serial.print(r/4);
  Serial.print(" ");
  Serial.print(g/4);
  Serial.print(" ");
  Serial.print(b/4);
  Serial.println();

  analogWrite(R_OUT, r/4);
  analogWrite(G_OUT, g/4);
  analogWrite(B_OUT, b/4);

  delay(30);
}
