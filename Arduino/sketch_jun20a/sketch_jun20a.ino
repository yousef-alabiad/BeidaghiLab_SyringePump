#define STEP_PIN 6
#define DIR_PIN 5

const float steps_per_rev = 360.0 / 1.8;  // 200 steps for 1.8° motor
const float ml_per_rev = 0.5;             // Adjust for your syringe
const float steps_per_ml = steps_per_rev / ml_per_rev;

bool cancel_requested = false;

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, HIGH);  // Default direction
  Serial.begin(9600);
  Serial.println("Ready for DISPENSE:<vol_ml>,<rate_ml_per_min> or CANCEL");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("DISPENSE:")) {
      int commaIndex = command.indexOf(',');
      if (commaIndex > 9) {
        float volume = command.substring(9, commaIndex).toFloat();
        float rate = command.substring(commaIndex + 1).toFloat();
        Serial.print("Dispensing ");
        Serial.print(volume);
        Serial.print(" mL at ");
        Serial.print(rate);
        Serial.println(" mL/min");
        cancel_requested = false;
        digitalWrite(DIR_PIN, HIGH);
        dispense_volume(volume, rate);
        Serial.println("Done.");
      } else {
        Serial.println("Invalid DISPENSE format. Use DISPENSE:volume,rate");
      }
    } else if (command == "CANCEL") {
      cancel_requested = true;
      Serial.println("Dispense canceled.");
    } else {
      Serial.println("Unknown command.");
    }
  }
}

void dispense_volume(float volume_ml, float rate_ml_per_min) {
  long total_steps = volume_ml * steps_per_ml;
  float delay_us = (60.0 * 1000000.0) / (rate_ml_per_min * steps_per_ml);

  for (long i = 0; i < total_steps; i++) {
    if (cancel_requested) break;
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds((int)(delay_us / 2));
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds((int)(delay_us / 2));
  }
}
