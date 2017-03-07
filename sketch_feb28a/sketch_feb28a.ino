int rled = 3;
int gled = 5;
int bled = 6;

int rbrightness = 100;
float gbrightness = 5;
int bbrightness = 10;
int rfadeAmount = 5;
int gfadeAmount = 1;
int bfadeAmount = 5;

void setup() {
  pinMode(rled, OUTPUT);
  pinMode(gled, OUTPUT);
  pinMode(bled, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  /*
  analogWrite(rled, rbrightness);
  analogWrite(gled, gbrightness);
  analogWrite(bled, bbrightness);
  */
  // set the brightness of pin 9:
  analogWrite(rled, rbrightness);
  analogWrite(gled, gbrightness);
  analogWrite(bled, bbrightness);

  // change the brightness for next time through the loop:
  rbrightness = rbrightness + rfadeAmount;
  gbrightness = gbrightness + gfadeAmount;
  bbrightness = bbrightness + bfadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (rbrightness <= 0 || rbrightness >= 255) {
    rfadeAmount = -rfadeAmount;
  }

  // reverse the direction of the fading at the ends of the fade:
  if (gbrightness <= 0 || gbrightness >= 50) {
    gfadeAmount = -gfadeAmount;
  }

  // reverse the direction of the fading at the ends of the fade:
  if (bbrightness <= 0 || bbrightness >= 255) {
    bfadeAmount = -bfadeAmount;
  }
  // wait for 30 milliseconds to see the dimming effect
  
  delay(30);
}
