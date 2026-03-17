#!/usr/bin/env python3
"""
Monitor ESP32-S3 interaction with SimHub
- Captures debug output on COM12 (UART0 at 115200) via external debugger
- Shows what the device is doing before/during/after resets
- Helps diagnose why device keeps resetting when SimHub connects
- SimHub uses COM11 (USB CDC) at 19200 for protocol
"""

import serial
import time
import sys
from datetime import datetime

def log_msg(msg, level="INFO"):
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] [{level}] {msg}")

def monitor_debug_uart():
    """Monitor debug output from ESP32-S3 UART0 on COM12"""
    
    debug_port = "COM12"
    debug_baud = 115200
    
    try:
        log_msg(f"Opening {debug_port} at {debug_baud} baud...", "INFO")
        ser = serial.Serial(
            port=debug_port,
            baudrate=debug_baud,
            timeout=1,
            rtscts=False,
            dsrdtr=False
        )
        
        log_msg(f"✓ Connected to {debug_port}", "INFO")
        log_msg("Listening for debug output. Press Ctrl+C to exit.", "INFO")
        log_msg("=" * 80, "INFO")
        
        buffer = ""
        reset_count = 0
        last_line_time = time.time()
        
        while True:
            try:
                if ser.in_waiting:
                    chunk = ser.read(ser.in_waiting)
                    text = chunk.decode('utf-8', errors='replace')
                    buffer += text
                    last_line_time = time.time()
                    
                    # Process complete lines
                    while '\n' in buffer or '\r' in buffer:
                        # Split on \r\n, \n, or \r
                        if '\r\n' in buffer:
                            line, buffer = buffer.split('\r\n', 1)
                        elif '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                        elif '\r' in buffer:
                            line, buffer = buffer.split('\r', 1)
                        else:
                            break
                        
                        line = line.strip()
                        if not line:
                            continue
                        
                        # Highlight important events
                        if "rst:" in line and "USB_UART_CHIP_RESET" in line:
                            reset_count += 1
                            log_msg(f"⚠️  RESET #{reset_count}: {line}", "RESET")
                        elif "boot:" in line:
                            log_msg(line, "BOOT")
                        elif "[DBG]" in line:
                            log_msg(line, "DBG")
                        elif "waiting for download" in line:
                            log_msg("⚠️  Device in download mode (waiting for firmware)", "WARN")
                        elif any(x in line for x in ["initialized", "setup", "connected"]):
                            log_msg(line, "INIT")
                        else:
                            log_msg(line, "OUT")
                
                # Check for timeout (no data for 3 seconds = device likely reset/offline)
                if time.time() - last_line_time > 3:
                    if buffer:  # Flush remaining buffer
                        log_msg(f"[BUFFER] {buffer}", "WARN")
                        buffer = ""
                    
            except KeyboardInterrupt:
                log_msg("User interrupted", "STOP")
                break
            except Exception as e:
                log_msg(f"Error reading serial: {e}", "ERROR")
                time.sleep(0.1)
    
    except serial.SerialException as e:
        log_msg(f"❌ Failed to open {debug_port}: {e}", "ERROR")
        log_msg("Make sure ZXACC debugger is connected and COM12 is correct", "ERROR")
        sys.exit(1)
    finally:
        try:
            ser.close()
            log_msg("Serial port closed", "INFO")
        except:
            pass

if __name__ == "__main__":
    print("\n" + "=" * 80)
    print("ESP32-S3 SimHub Interaction Monitor")
    print("=" * 80)
    print("\nThis monitors debug output on COM12 to see what's happening")
    print("when SimHub communicates with the device.")
    print("\nIn another terminal, open SimHub and try to connect to COM11 at 19200 baud.")
    print("\n" + "=" * 80 + "\n")
    
    monitor_debug_uart()
