// Enhanced Dispenser with Progress Reporting and Status using AccelStepper
#include <AccelStepper.h>

#define STEP_PIN 6
#define DIR_PIN 5

// Create stepper motor instance
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

const float steps_per_rev = 360.0 / 1.8;  // 200 steps for 1.8° motor
const float ml_per_rev = 0.5;             // Adjust for your syringe
const float steps_per_ml = steps_per_rev / ml_per_rev;

// Status tracking variables

// Loop:
// check status idle, dispensing, cancelled,
// id dispensing, dispence the amount of m




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
const unsigned long STATUS_UPDATE_INTERVAL = 50; // Update every 50ms for smoother updates

// Dispensing variables
long target_position = 0;
long start_position = 0;
unsigned long dispense_start_time = 0;
float dispensed_volume = 0.0;

void setup() {
  Serial.begin(9600);
  
  // Configure stepper motor
  stepper.setMaxSpeed(1000);  // steps per second
  stepper.setAcceleration(500);  // steps per second^2
  stepper.setCurrentPosition(0);
  
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
  
  // Handle stepper motor movement (non-blocking)
  if (current_status == DISPENSING) {
    if (cancel_requested) {
      // Stop motor immediately
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());
      current_status = CANCELLED;
      Serial.println("DISPENSE_CANCELLED");
      progress_percent = 0.0;
      dispensed_volume = 0.0;
      send_status();
      return;
    }
    
    // Check if movement is complete
    if (stepper.distanceToGo() == 0) {
      current_status = IDLE;
      Serial.println("DISPENSE_COMPLETE");
      progress_percent = 100.0;
      dispensed_volume = current_volume;
      send_status();
      return;
    }
    
    // Run stepper motor (non-blocking)
    stepper.run();
    
    // Update progress and dispensed volume
    long current_pos = stepper.currentPosition();
    long total_distance = target_position - start_position;
    if (total_distance > 0) {
      progress_percent = ((float)(current_pos - start_position) / (float)total_distance) * 100.0;
      dispensed_volume = current_volume * (progress_percent / 100.0);
    }
  }
  
  // Send periodic status updates during dispensing
  if (current_status == DISPENSING && 
      (millis() - last_status_update) >= STATUS_UPDATE_INTERVAL) {
    send_comprehensive_update();
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
    dispensed_volume = 0.0;
    dispense_start_time = millis();
    
    Serial.print("DISPENSE_START: ");
    Serial.print(volume);
    Serial.print(" mL at ");
    Serial.print(rate);
    Serial.println(" mL/min");
    
    // Calculate movement parameters
    long steps_to_move = volume * steps_per_ml;
    float speed_steps_per_sec = (rate / 60.0) * steps_per_ml;  // Convert mL/min to steps/sec
    
    // Set motor parameters
    stepper.setMaxSpeed(speed_steps_per_sec);
    stepper.setAcceleration(speed_steps_per_sec * 2);  // 2x speed for acceleration
    
    // Set target position
    start_position = stepper.currentPosition();
    target_position = start_position + steps_to_move;
    stepper.moveTo(target_position);
    
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

void send_comprehensive_update() {
  // Calculate time information
  unsigned long elapsed_time = millis() - dispense_start_time;
  float elapsed_minutes = elapsed_time / 60000.0;
  float remaining_volume = current_volume - dispensed_volume;
  float estimated_remaining_time = 0;
  
  if (current_rate > 0) {
    estimated_remaining_time = remaining_volume / current_rate; // in minutes
  }
  
  // Calculate current motor speed
  float current_speed = stepper.speed(); // steps per second
  float current_speed_ml_min = (current_speed / steps_per_ml) * 60.0; // convert to mL/min
  
  // Send detailed progress update
  Serial.print("PROGRESS_DETAILED: ");
  Serial.print(progress_percent, 1);
  Serial.print("%,");
  Serial.print(dispensed_volume, 2);
  Serial.print("mL,");
  Serial.print(remaining_volume, 2);
  Serial.print("mL,");
  Serial.print(elapsed_minutes, 1);
  Serial.print("min,");
  Serial.print(estimated_remaining_time, 1);
  Serial.print("min,");
  Serial.print(current_speed_ml_min, 1);
  Serial.print("mL/min,");
  Serial.print(stepper.currentPosition());
  Serial.print("steps,");
  Serial.print(stepper.distanceToGo());
  Serial.println("steps_remaining");
  
  // Also send the simple progress update for backward compatibility
  send_progress_update();
}

void send_progress_update() {
  Serial.print("PROGRESS: ");
  Serial.print(progress_percent, 1);
  Serial.print("% - ");
  Serial.print(dispensed_volume, 2);
  Serial.print("/");
  Serial.print(current_volume);
  Serial.println("mL");
}