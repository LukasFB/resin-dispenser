#include <Q2HX711.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

// setup scale
const byte hx711Data = 2;
const byte hx711Clock = 3;
Q2HX711 hx711(hx711Data, hx711Clock);
float calibrationWeight = 9.415; // << insert the weight of your test object here
float avg_size = 4.0;
float y1 = calibrationWeight;
long x0 = 10700632.0; // << insert values from scale calibration here
long x1 = 10564794.0; // << insert values from scale calibration here
float weightOffset = 0.0;

int btn1 = 10;
int btn2 = 13;
int btn3 = 12;
int btn4 = 11;

int isPressed1 = 0;
int isPressed2 = 0;
int isPressed3 = 0;
int isPressed4 = 0;

int m1Enable = 9;
int m2Enable = 8;
int m1Dir = 7;
int m2Dir = 6;
int m1Step = 5;
int m2Step = 4;

float target = 3.0;
float ratio = 0.6;
float fillEarlyStop = 0.0;

String mode = "main";

int m1Dispense = HIGH;
int m2Dispense = LOW;
int m1Retract = LOW;
int m2Retract = HIGH;

float stepDelay = 1400;
float finalA = 0;
float finalB = 0;

float scaleTara = 0;

void setup() {

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Starting ...");

  Serial.begin(9600);
  Serial.write("Start ...");

  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(btn3, INPUT_PULLUP);
  pinMode(btn4, INPUT_PULLUP);

  // stepper setup
  pinMode(m1Enable, OUTPUT);
  pinMode(m1Dir, OUTPUT);
  pinMode(m1Step, OUTPUT);
  pinMode(m2Enable, OUTPUT);
  pinMode(m2Dir, OUTPUT);
  pinMode(m2Step, OUTPUT);

  digitalWrite(m1Enable, HIGH);
  digitalWrite(m2Enable, HIGH);

  //scaleCalibration(); // << uncomment this once to calibrate the scale
  switchToMode("main");
}

// how much to rotate based on the difference between current and target weight
int getStepsForDiff(float diff) {
  if (diff > 2) return 350;
  if (diff > 1) return 150;
  if (diff > 0.75) return 40;
  if (diff > 0.4) return 15;
  return 5;
}

void loop() {

  checkButtons();

  if (mode == "main") {
    lcd.setCursor(8, 0);
    lcd.print(target);
    lcd.setCursor(0, 1);
    lcd.print(target / (1 + ratio));
    lcd.print(" A");
    lcd.setCursor(8, 1);
    lcd.print(target / (1 + ratio) * ratio);
    lcd.print(" B");
  }

  if (mode == "dispenseA") {

    float current = getScaleReading();
    lcd.setCursor(0, 1);
    lcd.print(current);
    float targetA = target / (1 + ratio);

    if (current > targetA) {
      // reverse motors for a bit, anti drip function
      digitalWrite(m1Dir, m1Retract);
      for (int i = 0; i < 50; i++) {
        digitalWrite(m1Step, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(m1Step, LOW);
        delayMicroseconds(stepDelay);
      }
      delay(250);
      finalA = getScaleReading();
      delay(1000);
      switchToMode("dispenseB");
    } else {
      int stepToDo = getStepsForDiff(targetA - current);
      digitalWrite(m1Dir, m1Dispense);
      for (int i = 0; i < stepToDo; i++) {
        digitalWrite(m1Step, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(m1Step, LOW);
        delayMicroseconds(stepDelay);
      }
      delay(100);
    }
  }

  if (mode == "dispenseB") {
    float current = getScaleReading();
    lcd.setCursor(0, 1);
    lcd.print(current);
    float targetB = target / (1 + ratio) * ratio;

    if (current > targetB) {
      // reverse motors for a bit, anti drip function
      digitalWrite(m2Dir, m2Retract);
      for (int i = 0; i < 50; i++) {
        digitalWrite(m2Step, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(m2Step, LOW);
        delayMicroseconds(stepDelay);
      }
      delay(250);
      finalB = getScaleReading();
      delay(1000);
      switchToMode("result");
    } else {
      int stepToDo = getStepsForDiff(targetB - current);
      digitalWrite(m2Dir, m2Dispense);
      for (int i = 0; i < stepToDo; i++) {
        digitalWrite(m2Step, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(m2Step, LOW);
        delayMicroseconds(stepDelay);
      }
      delay(100);
    }

  }
}

void switchToMode(String newMode) {
  mode = newMode;
  lcd.setCursor(0, 0);
  lcd.clear();
  if (mode == "main") {
    // HIGH on able pins = disable motors
    digitalWrite(m1Enable, HIGH);
    digitalWrite(m2Enable, HIGH);
    lcd.print("Target: ");
  }
  if (mode == "dispenseA") {
    digitalWrite(m1Enable, LOW);
    lcd.setCursor(0, 0);
    lcd.print("Filling Part A");
    lcd.setCursor(8, 1);
    lcd.print(" / ");
    lcd.print(target / (1 + ratio));
    delay(250);
    scaleTara = getScaleReading();
  }
  if (mode == "dispenseB") {
    digitalWrite(m1Enable, HIGH);
    digitalWrite(m2Enable, LOW);
    lcd.setCursor(0, 0);
    lcd.print("Filling Part B");
    lcd.setCursor(8, 1);
    lcd.print(" / ");
    lcd.print(target / (1 + ratio) * ratio);
    delay(250);
    scaleTara += getScaleReading();
  }
  if (mode == "result") {
    scaleTara = 0;
    digitalWrite(m1Enable, HIGH);
    digitalWrite(m2Enable, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Complete");

    lcd.setCursor(0, 0);
    lcd.print(finalA);
    lcd.setCursor(8, 0);
    lcd.print(finalB);

    float realRatio = finalB / finalA;
    float percentage = (realRatio / ratio * 100) - 100;
    lcd.setCursor(0, 0);
    lcd.print("Error ");
    lcd.print(percentage);
    lcd.print(" %");

    delay(5000);
    switchToMode("main");
  }
}

float getScaleReading() {
  // averaging reading
  long reading = 0;
  for (int jj = 0; jj < int(avg_size); jj++) {
    reading += hx711.read();
  }
  reading /= long(avg_size);
  // calculating mass based on calibration and linear fit
  float ratio_1 = (float) (reading - x0);
  float ratio_2 = (float) (x1 - x0);
  float ratio = ratio_1 / ratio_2;
  float mass = y1 * ratio;
  return mass + weightOffset - scaleTara;
}

void checkButtons() {
  if (mode == "main") {
    // increase amount
    if (isPressed1 == 0) {
      if (digitalRead(btn1) == LOW) {
        isPressed1 = 1;
        target += 1;
      }
    } else {
      if (digitalRead(btn1) == HIGH) {
        isPressed1 = 0;
      }
    }

    // decrease amount
    if (isPressed2 == 0) {
      if (digitalRead(btn2) == LOW) {
        isPressed2 = 1;
        if (target > 1) {
          target -= 1;
        }
      }
    } else {
      if (digitalRead(btn2) == HIGH) {
        isPressed2 = 0;
      }
    }

    // start dispensing
    if (digitalRead(btn4) == LOW) {
      switchToMode("dispenseA");
    }
  }
}


void scaleCalibration() {
  delay(1000); // allow load cell and hx711 to settle
  // tare procedure
  x0 = 0L;
  x1 = 0L;
  for (int ii = 0; ii < int(avg_size); ii++) {
    delay(10);
    x0 += hx711.read();
  }
  x0 /= long(avg_size);
  Serial.println("Add calibration weight");
  delay(5000);
  // calibration procedure (mass should be added equal to y1)
  int ii = 1;
  while (true) {
    int r = hx711.read();
    if (r < 5000) {
      Serial.println("no weight ... " + String(r));
      delay(1000);
    } else {
      Serial.println("Calibration mass detected");
      ii++;
      delay(2000);
      for (int jj = 0; jj < int(avg_size); jj++) {
        x1 += hx711.read();
      }
      x1 /= long(avg_size);
      break;
    }
  }
  Serial.println("Calibration Complete");
  Serial.println("x0: ");
  Serial.println(x0);
  Serial.println("x1: ");
  Serial.println(x1);
}
