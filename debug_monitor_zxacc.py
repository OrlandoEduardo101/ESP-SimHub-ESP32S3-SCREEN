#!/usr/bin/env python3
"""
Debug Monitor for WT32-SC01 Plus via ZXACC
Monitors debug logs from UART1 (GPIO 17 TXD / GPIO 18 RXD)
via ZXACC programmer while SimHub is connected to USB CDC (COM11)

Usage:
    python debug_monitor_zxacc.py --port COM5 --baud 115200
    
ZXACC Connection:
    1. Connect ZXACC to WT32-SC01 Plus Debug Interface:
       ZXACC Pin 1 (+5V)  -> WT32-SC01 Plus Pin 1 (+5V)
       ZXACC Pin 3 (TXD)  -> WT32-SC01 Plus Pin 4 (ESP_RXD / GPIO 18)
       ZXACC Pin 4 (RXD)  -> WT32-SC01 Plus Pin 3 (ESP_TXD / GPIO 17)
       ZXACC Pin 7 (GND)  -> WT32-SC01 Plus Pin 7 (GND)
    
    2. Connect USB-TTL adapter or ZXACC to your computer
    3. Find which COM port it appears as (use Device Manager)
    4. Run this script with that COM port
"""

import serial
import argparse
import time
import sys
from datetime import datetime

class DebugMonitor:
    def __init__(self, port, baud=115200, timeout=1):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None
        self.running = False
        
    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=self.timeout)
            self.running = True
            print(f"[✓] Connected to {self.port} at {self.baud} baud")
            print("[*] Waiting for debug output from WT32-SC01 Plus...")
            print("[*] Make sure ZXACC is connected to Debug Interface pins")
            print("[*] Press Ctrl+C to exit\n")
            return True
        except serial.SerialException as e:
            print(f"[✗] Failed to connect to {self.port}: {e}")
            return False
    
    def monitor(self):
        if not self.connect():
            sys.exit(1)
        
        try:
            while self.running:
                if self.ser.in_waiting > 0:
                    try:
                        line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                            print(f"[{timestamp}] {line}")
                    except Exception as e:
                        print(f"[E] Decode error: {e}")
                else:
                    time.sleep(0.01)
        except KeyboardInterrupt:
            print("\n[*] Monitoring stopped by user")
        finally:
            self.disconnect()
    
    def disconnect(self):
        if self.ser:
            self.ser.close()
            self.running = False
            print("[*] Disconnected")

def list_ports():
    """List available serial ports"""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        if ports:
            print("Available COM ports:")
            for port, desc, hwid in ports:
                print(f"  {port}: {desc}")
        else:
            print("No COM ports found")
    except Exception as e:
        print(f"Error listing ports: {e}")

def main():
    parser = argparse.ArgumentParser(
        description='Debug Monitor for WT32-SC01 Plus via ZXACC',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  python debug_monitor_zxacc.py --port COM5
  python debug_monitor_zxacc.py --port COM5 --baud 115200
  python debug_monitor_zxacc.py --list-ports
        '''
    )
    
    parser.add_argument('--port', '-p', help='COM port (e.g., COM5, /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--list-ports', action='store_true', help='List available COM ports')
    
    args = parser.parse_args()
    
    if args.list_ports:
        list_ports()
        return
    
    if not args.port:
        print("Error: --port is required")
        parser.print_help()
        sys.exit(1)
    
    monitor = DebugMonitor(args.port, args.baud)
    monitor.monitor()

if __name__ == '__main__':
    main()
