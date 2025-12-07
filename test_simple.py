#!/usr/bin/env python3
import serial
import time

PORT = "COM11"
BAUD = 115200

print(f"Conectando em {PORT} @ {BAUD} baud...")
ser = serial.Serial(PORT, BAUD, timeout=5)
print("✓ Conectado. Aguardando dados por 10 segundos...")
print("(Enviando Ctrl+C para sair)\n")

try:
    start = time.time()
    while time.time() - start < 10:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            print(f"[{time.strftime('%H:%M:%S')}] RX ({len(data)} bytes): {data.hex()} | {data}")
        time.sleep(0.1)
except KeyboardInterrupt:
    print("\n\nInterrompido pelo usuário")
finally:
    ser.close()
    print("✓ Porta fechada")
