# Arduino Syringe-Pump Manager

## Files

- `pump_manager.py` - Main manager interface
- `pump_window.py` - Individual pump control
- `main_v2_main.py` - Entry point (deleted)

## Run

```bash
python pump_manager.py
```

## Structure

```
pump_manager.py
    └── pump_window.py
```

## Features

- Multi-pump management
- Real-time progress tracking
- Serial communication with Arduino
- Enhanced GUI with detailed progress display

## How It Works

### Pump Manager
- Creates and manages multiple pump windows
- Tracks pump status in a treeview
- Handles system-wide logging
- Coordinates pump events and callbacks

### Pump Window
- Connects to Arduino via serial port
- Sends commands: `DISPENSE:<volume>,<rate>`, `CANCEL`, `STATUS`
- Receives real-time updates from Arduino
- Displays progress with volume, time, and speed information

### Arduino Communication
- Uses AccelStepper library for smooth motor control
- Sends detailed progress updates every 50ms
- Provides status: IDLE, DISPENSING, CANCELLED, ERROR
- Calculates remaining time and current speed

### Key Functions
- `add_pump()` - Create new pump window
- `connect_to_arduino()` - Establish serial connection
- `start_dispense()` - Send dispense command
- `handle_arduino_message()` - Process Arduino responses
- `send_comprehensive_update()` - Real-time progress data 