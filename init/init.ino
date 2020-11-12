#include <Servo.h>
Servo ESC;
void setup() {
  // put your setup code here, to run once:
  ESC.attach(9);
  ESC.writeMicroseconds(2000);
  delay(3000);
  ESC.writeMicroseconds(1000);
  delay(5000);
  ESC.writeMicroseconds(1200);
}

void loop() {
  // put your main code here, to run repeatedly:

}
