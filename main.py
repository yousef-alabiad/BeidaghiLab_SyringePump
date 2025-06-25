#!/usr/bin/env python3
"""
Arduino Syringe-Pump Manager - Main Entry Point
===============================================

This is the main entry point for the Arduino Syringe-Pump Manager application.
It creates the main window and starts the PumpManager.

Features:
- Multi-pump management interface
- Individual pump control windows
- Real-time status monitoring
- Enhanced progress tracking

Author: Beidaghi Lab
Version: 2.0
"""

import tkinter as tk
from pump_manager import PumpManager

def main():
    """Main entry point for the application"""
    # Create the root window
    root = tk.Tk()
    
    # Create and start the pump manager
    app = PumpManager(root)
    
    # Start the main event loop
    root.mainloop()

if __name__ == "__main__":
    main() 