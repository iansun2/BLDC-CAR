void setup() {
  // put your setup code here, to run once:
  pinMode(9,OUTPUT);
  pinMode(10,OUTPUT);
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  TCCR1B = TCCR1B & 0xF8 | 1;
  Serial.begin(2000000);
  //analogWrite(9,250);
  //analogWrite(10,50);
}

void loop() {
  // put your main code here, to run repeatedly:
    int i = analogRead(A0);
    i = map(i,0,1023,0,255);
    analogWrite(9,i);
    Serial.print("l: ");
    Serial.println(i);
    i = analogRead(A1);
    i = map(i,0,1023,0,255);
    analogWrite(10,i);
    Serial.print("r: ");
    Serial.println(i);
  
}
