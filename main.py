import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial
import serial.tools.list_ports
import threading
import time
import queue

class ArduinoDispenserGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Arduino Dispenser Control")
        self.root.geometry("600x500")
        
        # Serial connection
        self.serial_connection = None
        self.is_connected = False
        self.is_dispensing = False
        
        # Queue for thread-safe GUI updates
        self.message_queue = queue.Queue()
        
        # Create GUI elements
        self.create_widgets()
        
        # Start message processing
        self.process_messages()
        
        # Refresh COM ports on startup
        self.refresh_ports()
    
    def create_widgets(self):
        # Connection Frame
        conn_frame = ttk.LabelFrame(self.root, text="Connection", padding=10)
        conn_frame.pack(fill="x", padx=10, pady=5)
        
        ttk.Label(conn_frame, text="COM Port:").grid(row=0, column=0, sticky="w")
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var, 
                                       state="readonly", width=15)
        self.port_combo.grid(row=0, column=1, padx=5)
        
        self.refresh_btn = ttk.Button(conn_frame, text="Refresh", 
                                      command=self.refresh_ports)
        self.refresh_btn.grid(row=0, column=2, padx=5)
        
        self.connect_btn = ttk.Button(conn_frame, text="Connect", 
                                      command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=3, padx=5)
        
        self.status_label = ttk.Label(conn_frame, text="Status: Disconnected", 
                                      foreground="red")
        self.status_label.grid(row=1, column=0, columnspan=4, sticky="w", pady=5)
        
        # Control Frame
        control_frame = ttk.LabelFrame(self.root, text="Dispenser Control", padding=10)
        control_frame.pack(fill="x", padx=10, pady=5)
        
        ttk.Label(control_frame, text="Volume (mL):").grid(row=0, column=0, sticky="w")
        self.volume_var = tk.StringVar(value="5.0")
        volume_entry = ttk.Entry(control_frame, textvariable=self.volume_var, width=10)
        volume_entry.grid(row=0, column=1, padx=5)
        
        ttk.Label(control_frame, text="Rate (mL/min):").grid(row=0, column=2, sticky="w", padx=(20,0))
        self.rate_var = tk.StringVar(value="10.0")
        rate_entry = ttk.Entry(control_frame, textvariable=self.rate_var, width=10)
        rate_entry.grid(row=0, column=3, padx=5)
        
        self.dispense_btn = ttk.Button(control_frame, text="Start Dispense", 
                                       command=self.start_dispense, state="disabled")
        self.dispense_btn.grid(row=1, column=0, pady=10)
        
        self.cancel_btn = ttk.Button(control_frame, text="Cancel", 
                                     command=self.cancel_dispense, state="disabled")
        self.cancel_btn.grid(row=1, column=1, padx=5, pady=10)
        
        self.status_btn = ttk.Button(control_frame, text="Get Status", 
                                     command=self.get_status, state="disabled")
        self.status_btn.grid(row=1, column=2, padx=5, pady=10)
        
        # Progress Frame
        progress_frame = ttk.LabelFrame(self.root, text="Progress", padding=10)
        progress_frame.pack(fill="x", padx=10, pady=5)
        
        self.progress_var = tk.StringVar(value="Ready")
        progress_label = ttk.Label(progress_frame, textvariable=self.progress_var)
        progress_label.pack(anchor="w")
        
        self.progress_bar = ttk.Progressbar(progress_frame, length=400, mode='determinate')
        self.progress_bar.pack(fill="x", pady=5)
        
        # Log Frame
        log_frame = ttk.LabelFrame(self.root, text="Communication Log", padding=10)
        log_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=12, width=70)
        self.log_text.pack(fill="both", expand=True)
        
        # Clear log button
        clear_btn = ttk.Button(log_frame, text="Clear Log", command=self.clear_log)
        clear_btn.pack(anchor="e", pady=5)
    
    def refresh_ports(self):
        """Refresh the list of available COM ports"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.set(ports[0])
    
    def toggle_connection(self):
        """Connect or disconnect from the Arduino"""
        if not self.is_connected:
            self.connect_to_arduino()
        else:
            self.disconnect_from_arduino()
    
    def connect_to_arduino(self):
        """Establish serial connection to Arduino"""
        port = self.port_var.get()
        if not port:
            messagebox.showerror("Error", "Please select a COM port")
            return
        
        try:
            self.serial_connection = serial.Serial(port, 9600, timeout=1)
            time.sleep(2)  # Wait for Arduino to initialize
            
            self.is_connected = True
            self.connect_btn.config(text="Disconnect")
            self.status_label.config(text=f"Status: Connected to {port}", foreground="green")
            
            # Enable control buttons
            self.dispense_btn.config(state="normal")
            self.status_btn.config(state="normal")
            
            # Start reading thread
            self.reading_thread = threading.Thread(target=self.read_serial, daemon=True)
            self.reading_thread.start()
            
            self.log_message(f"Connected to {port}")
            
        except Exception as e:
            messagebox.showerror("Connection Error", f"Failed to connect: {str(e)}")
            self.log_message(f"Connection failed: {str(e)}")
    
    def disconnect_from_arduino(self):
        """Close serial connection"""
        if self.serial_connection:
            self.serial_connection.close()
            self.serial_connection = None
        
        self.is_connected = False
        self.is_dispensing = False
        self.connect_btn.config(text="Connect")
        self.status_label.config(text="Status: Disconnected", foreground="red")
        
        # Disable control buttons
        self.dispense_btn.config(state="disabled")
        self.cancel_btn.config(state="disabled")
        self.status_btn.config(state="disabled")
        
        # Reset progress
        self.progress_var.set("Ready")
        self.progress_bar['value'] = 0
        
        self.log_message("Disconnected")
    
    def start_dispense(self):
        """Send dispense command to Arduino"""
        if not self.is_connected:
            return
        
        try:
            volume = float(self.volume_var.get())
            rate = float(self.rate_var.get())
            
            if volume <= 0 or rate <= 0:
                messagebox.showerror("Invalid Input", "Volume and rate must be positive numbers")
                return
            
            command = f"DISPENSE:{volume},{rate}\n"
            self.serial_connection.write(command.encode())
            
            self.is_dispensing = True
            self.dispense_btn.config(state="disabled")
            self.cancel_btn.config(state="normal")
            
            self.log_message(f"Sent: {command.strip()}")
            
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter valid numbers for volume and rate")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to send command: {str(e)}")
    
    def cancel_dispense(self):
        """Send cancel command to Arduino"""
        if not self.is_connected:
            return
        
        try:
            self.serial_connection.write(b"CANCEL\n")
            self.log_message("Sent: CANCEL")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to send cancel: {str(e)}")
    
    def get_status(self):
        """Request status from Arduino"""
        if not self.is_connected:
            return
        
        try:
            self.serial_connection.write(b"STATUS\n")
            self.log_message("Sent: STATUS")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to get status: {str(e)}")
    
    def read_serial(self):
        """Read messages from Arduino in separate thread"""
        while self.is_connected and self.serial_connection:
            try:
                if self.serial_connection.in_waiting:
                    message = self.serial_connection.readline().decode().strip()
                    if message:
                        self.message_queue.put(message)
                time.sleep(0.1)
            except Exception as e:
                self.message_queue.put(f"Read error: {str(e)}")
                break
    
    def process_messages(self):
        """Process messages from Arduino (runs in main thread)"""
        try:
            while True:
                message = self.message_queue.get_nowait()
                self.handle_arduino_message(message)
        except queue.Empty:
            pass
        
        # Schedule next check
        self.root.after(100, self.process_messages)
    
    def handle_arduino_message(self, message):
        """Handle incoming messages from Arduino"""
        self.log_message(f"Received: {message}")
        
        if message.startswith("STATUS:"):
            status = message[7:].strip()
            if "DISPENSING" in status:
                self.progress_var.set(f"Dispensing: {status}")
                if not self.is_dispensing:
                    self.is_dispensing = True
                    self.dispense_btn.config(state="disabled")
                    self.cancel_btn.config(state="normal")
            else:
                self.progress_var.set(f"Status: {status}")
                if self.is_dispensing and status in ["IDLE", "CANCELLED", "ERROR"]:
                    self.is_dispensing = False
                    self.dispense_btn.config(state="normal")
                    self.cancel_btn.config(state="disabled")
                    self.progress_bar['value'] = 0
        
        elif message.startswith("PROGRESS:"):
            # Extract progress percentage
            try:
                progress_part = message.split()[1]  # Get "XX.X%"
                progress_value = float(progress_part.replace('%', ''))
                self.progress_bar['value'] = progress_value
                self.progress_var.set(f"Progress: {message[9:]}")
            except:
                self.progress_var.set(f"Progress: {message[9:]}")
        
        elif message in ["DISPENSE_COMPLETE", "DISPENSE_CANCELLED"]:
            self.is_dispensing = False
            self.dispense_btn.config(state="normal")
            self.cancel_btn.config(state="disabled")
            self.progress_bar['value'] = 0 if "CANCELLED" in message else 100
            self.progress_var.set(message.replace("_", " ").title())
    
    def log_message(self, message):
        """Add message to log with timestamp"""
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        self.log_text.insert(tk.END, log_entry)
        self.log_text.see(tk.END)
    
    def clear_log(self):
        """Clear the communication log"""
        self.log_text.delete(1.0, tk.END)

def main():
    root = tk.Tk()
    app = ArduinoDispenserGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()