// Enhanced Dispenser with Progress Reporting and Status
#define STEP_PIN 6
#define DIR_PIN 5

const float steps_per_rev = 360.0 / 1.8;  // 200 steps for 1.8° motor
const float ml_per_rev = 0.5;             // Adjust for your syringe
const float steps_per_ml = steps_per_rev / ml_per_rev;

// Status tracking variables
enum DeviceStatus {
  IDLE,
  DISPENSING,
  CANCELLED,
  ERROR
};

DeviceStatus current_status = IDLE;
bool cancel_requested = false;
float current_volume = 0.0;
float current_rate = 0.0;
float progress_percent = 0.0;
unsigned long last_status_update = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 100; // Update every 100ms

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, HIGH);  // Default direction
  Serial.begin(9600);
  
  // Send initial status
  send_status();
  Serial.println("Ready for DISPENSE:<vol_ml>,<rate_ml_per_min> or CANCEL");
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("DISPENSE:")) {
      handle_dispense_command(command);
    } else if (command == "CANCEL") {
      handle_cancel_command();
    } else if (command == "STATUS") {
      send_status();
    } else {
      Serial.println("ERROR: Unknown command. Use DISPENSE:<vol>,<rate>, CANCEL, or STATUS");
    }
  }
  
  // Send periodic status updates during dispensing
  if (current_status == DISPENSING && 
      (millis() - last_status_update) >= STATUS_UPDATE_INTERVAL) {
    send_progress_update();
    last_status_update = millis();
  }
}

void handle_dispense_command(String command) {
  if (current_status == DISPENSING) {
    Serial.println("ERROR: Already dispensing. Send CANCEL first.");
    return;
  }
  
  int commaIndex = command.indexOf(',');
  if (commaIndex > 9) {
    float volume = command.substring(9, commaIndex).toFloat();
    float rate = command.substring(commaIndex + 1).toFloat();
    
    if (volume <= 0 || rate <= 0) {
      Serial.println("ERROR: Volume and rate must be positive");
      current_status = ERROR;
      send_status();
      return;
    }
    
    current_volume = volume;
    current_rate = rate;
    current_status = DISPENSING;
    cancel_requested = false;
    progress_percent = 0.0;
    
    Serial.print("DISPENSE_START: ");
    Serial.print(volume);
    Serial.print(" mL at ");
    Serial.print(rate);
    Serial.println(" mL/min");
    
    digitalWrite(DIR_PIN, HIGH);
    send_status();
    
    dispense_volume(volume, rate);
    
    if (cancel_requested) {
      current_status = CANCELLED;
      Serial.println("DISPENSE_CANCELLED");
    } else {
      current_status = IDLE;
      Serial.println("DISPENSE_COMPLETE");
    }
    
    progress_percent = 0.0;
    send_status();
    
  } else {
    Serial.println("ERROR: Invalid DISPENSE format. Use DISPENSE:volume,rate");
    current_status = ERROR;
    send_status();
  }
}

void handle_cancel_command() {
  if (current_status == DISPENSING) {
    cancel_requested = true;
    Serial.println("CANCEL_REQUESTED");
  } else {
    Serial.println("INFO: No active dispensing to cancel");
  }
}

void dispense_volume(float volume_ml, float rate_ml_per_min) {
  long total_steps = volume_ml * steps_per_ml;
  float delay_us = (60.0 * 1000000.0) / (rate_ml_per_min * steps_per_ml);
  
  for (long i = 0; i < total_steps; i++) {
    if (cancel_requested) {
      break;
    }
    
    // Update progress
    progress_percent = ((float)i / (float)total_steps) * 100.0;
    
    // Execute step
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds((int)(delay_us / 2));
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds((int)(delay_us / 2));
  }
  
  // Final progress update
  if (!cancel_requested) {
    progress_percent = 100.0;
  }
}

void send_status() {
  Serial.print("STATUS: ");
  
  switch (current_status) {
    case IDLE:
      Serial.println("IDLE");
      break;
    case DISPENSING:
      Serial.print("DISPENSING - ");
      Serial.print(current_volume);
      Serial.print("mL @ ");
      Serial.print(current_rate);
      Serial.print("mL/min - ");
      Serial.print(progress_percent, 1);
      Serial.println("%");
      break;
    case CANCELLED:
      Serial.println("CANCELLED");
      break;
    case ERROR:
      Serial.println("ERROR");
      break;
  }
}

void send_progress_update() {
  Serial.print("PROGRESS: ");
  Serial.print(progress_percent, 1);
  Serial.print("% - ");
  Serial.print(current_volume * (progress_percent / 100.0), 2);
  Serial.print("/");
  Serial.print(current_volume);
  Serial.println("mL");
}