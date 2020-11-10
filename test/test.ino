void setup() {
  // put your setup code here, to run once:
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  pinMode(A2,INPUT);
  pinMode(2,INPUT);
  pinMode(3,INPUT);
  pinMode(4,INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("wpcurrent: ");
  Serial.println(abs(analogRead(A0)-512));
  Serial.print("lcurrent: ");
  Serial.println(abs(analogRead(A1)-512));
  Serial.print("rcurrent: ");
  Serial.println(abs(analogRead(A2)-512));
  Serial.print("2: ");
  Serial.println(pulseIn(2,1,50000));
  Serial.print("3: ");
  Serial.println(pulseIn(3,1,50000));
  Serial.print("4: ");
  Serial.println(pulseIn(4,1,50000));
  Serial.println();
  delay(100);
}
