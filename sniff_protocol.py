#!/usr/bin/env python3
"""
Sniff the protocol - monitor what real SimHub sends vs what our test sends
This helps us understand the exact byte sequence SimHub uses
"""

import serial
import time

PORT = "COM11"
BAUD_RATE = 115200

def hex_dump(data, label=""):
    """Print hex dump of data"""
    if label:
        print(f"\n{label}")
    hex_str = " ".join([f"{b:02X}" for b in data])
    ascii_str = "".join([chr(b) if 32 <= b < 127 else "." for b in data])
    print(f"  Hex: {hex_str}")
    print(f"  Ascii: {ascii_str}")

def main():
    print("\n" + "="*70)
    print("Protocol Sniffer - Watch serial data flow")
    print("="*70 + "\n")
    print("Instruções:")
    print("1. Deixe este script rodando")
    print("2. Abra o SimHub e conecte com o ESP32")
    print("3. Veja os dados sendo enviados abaixo")
    print("\nPressione Ctrl+C para parar\n")
    
    try:
        ser = serial.Serial(PORT, BAUD_RATE, timeout=0.1)
        time.sleep(1)
        ser.reset_input_buffer()
        
        print("Aguardando dados...\n")
        
        buffer = bytearray()
        last_print = time.time()
        
        while True:
            if ser.in_waiting > 0:
                chunk = ser.read(ser.in_waiting)
                buffer.extend(chunk)
                
                # Print in real-time with some grouping
                if time.time() - last_print > 0.5 or len(buffer) > 100:
                    if buffer:
                        hex_dump(buffer, f"[DATA] {len(buffer)} bytes received:")
                        buffer = bytearray()
                        last_print = time.time()
            
            time.sleep(0.01)
    
    except KeyboardInterrupt:
        print("\n\n[OK] Sniffer parado")
    except Exception as e:
        print(f"[ERRO] {e}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()
