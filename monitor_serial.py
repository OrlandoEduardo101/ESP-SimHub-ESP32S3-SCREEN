#!/usr/bin/env python3
"""
Monitor serial output while running test
"""

import serial
import time
import threading
import sys

PORT = "COM11"
BAUD_RATE = 115200

def monitor_serial(ser):
    """Monitor serial output in background"""
    buffer = ""
    while True:
        if ser.in_waiting > 0:
            try:
                data = ser.read(1).decode('utf-8', errors='replace')
                buffer += data
                
                if data == '\n':
                    print(f"[SERIAL] {buffer.strip()}")
                    sys.stdout.flush()
                    buffer = ""
            except Exception as e:
                pass

def send_handshake(ser):
    """Send handshake commands"""
    MESSAGE_HEADER = 0x1E
    print("\n>>> Enviando handshake...\n")
    ser.write(bytes([MESSAGE_HEADER, ord('1')]))
    time.sleep(0.3)
    ser.write(bytes([MESSAGE_HEADER, ord('0')]))
    time.sleep(0.3)
    ser.write(bytes([MESSAGE_HEADER, ord('N')]))
    time.sleep(0.3)
    ser.write(bytes([MESSAGE_HEADER, ord('I')]))
    time.sleep(0.3)
    ser.write(bytes([MESSAGE_HEADER, ord('4')]))
    time.sleep(0.3)
    print(">>> Handshake enviado\n")

def send_test_frame(ser):
    """Send a single test frame"""
    MESSAGE_HEADER = 0x1E
    
    # Create simple test data with exactly 62 fields
    fields = [
        "100",  # speed
        "3", "50", "9000", "4500",
        "00:01:00.000", "00:00:59.500", "00:00:58.000", "0.500", "0.50", "False",
        "26.5", "26.5", "26.2", "26.2",
        "85", "85", "82", "82",
        "120", "120", "118", "118",
        "85", "90",
        "3", "1", "2", "0", "0", "54.0", "25",
        "1", "12", "--", "1.23", "15.0", "0.85", "00:45:00", "None", "0", "0",
        "NORMAL", "",
        "50", "0", "0", "0",
        "10", "10", "10", "10",
        "31.234", "32.456", "31.789",
        "28", "55",
        "0", "1", "0", "80", "1.2"
    ]
    
    data = ";".join(fields) + "\n"
    message = bytes([MESSAGE_HEADER, ord('P')]) + data.encode()
    
    print(f">>> Enviando frame: {len(message)} bytes")
    print(f"    Primeiros 100 chars: {message[:100]}")
    ser.write(message)

def main():
    try:
        print(f"\nConectando em {PORT} a {BAUD_RATE} baud...\n")
        ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        
        # Start monitoring thread
        monitor_thread = threading.Thread(target=monitor_serial, args=(ser,), daemon=True)
        monitor_thread.start()
        
        # Clear buffer
        ser.reset_input_buffer()
        time.sleep(0.5)
        
        print("\n=== INICIANDO TESTE COM MONITORAMENTO SERIAL ===\n")
        
        # Send handshake
        send_handshake(ser)
        time.sleep(1)
        
        # Send 5 test frames
        print(">>> Enviando 5 frames de teste...\n")
        for i in range(5):
            send_test_frame(ser)
            time.sleep(0.3)
        
        print("\n>>> Aguardando 3 segundos para processar...")
        time.sleep(3)
        
        print("\n>>> Teste concluído")
        
    except Exception as e:
        print(f"[ERRO] {e}")
        import traceback
        traceback.print_exc()
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()
