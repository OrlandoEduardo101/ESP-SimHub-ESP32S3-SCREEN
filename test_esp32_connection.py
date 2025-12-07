#!/usr/bin/env python3
import serial
import time
import sys

PORT = "COM11"
BAUD = 115200

def test_connection():
    try:
        print(f"Conectando em {PORT} @ {BAUD} baud...")
        ser = serial.Serial(PORT, BAUD, timeout=2)
        time.sleep(1)  # Aguarda inicialização do ESP32
        
        print("✓ Porta aberta com sucesso")
        print(f"✓ Porta: {ser.port}")
        print(f"✓ Baud rate: {ser.baudrate}")
        
        # Envia comando HELLO (0x03 0x01)
        print("\n[1] Enviando comando HELLO (0x03 0x01)...")
        ser.write(bytes([0x03, 0x01]))
        ser.flush()
        
        # Aguarda resposta
        print("Aguardando resposta...")
        response = ser.read(20)
        
        if response:
            print(f"✓ Resposta recebida ({len(response)} bytes):")
            print(f"  Hex: {response.hex()}")
            print(f"  ASCII: {response}")
        else:
            print("✗ Nenhuma resposta recebida (timeout)")
        
        # Testa FEATURES (0x03 0x00)
        print("\n[2] Enviando comando FEATURES (0x03 0x00)...")
        ser.write(bytes([0x03, 0x00]))
        ser.flush()
        
        print("Aguardando resposta...")
        response = ser.read(20)
        
        if response:
            print(f"✓ Resposta recebida ({len(response)} bytes):")
            print(f"  Hex: {response.hex()}")
            print(f"  ASCII: {response}")
        else:
            print("✗ Nenhuma resposta recebida (timeout)")
        
        # Testa DEVICE_NAME (0x03 0x4E)
        print("\n[3] Enviando comando DEVICE_NAME (0x03 0x4E)...")
        ser.write(bytes([0x03, 0x4E]))
        ser.flush()
        
        print("Aguardando resposta...")
        response = ser.read(50)
        
        if response:
            print(f"✓ Resposta recebida ({len(response)} bytes):")
            print(f"  Hex: {response.hex()}")
            try:
                print(f"  ASCII: {response.decode('utf-8', errors='ignore')}")
            except:
                pass
        else:
            print("✗ Nenhuma resposta recebida (timeout)")
        
        ser.close()
        print("\n✓ Teste concluído")
        
    except serial.SerialException as e:
        print(f"✗ Erro na porta serial: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"✗ Erro: {e}")
        sys.exit(1)

if __name__ == "__main__":
    test_connection()
