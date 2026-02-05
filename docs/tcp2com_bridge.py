#!/usr/bin/env python3
"""
TCP to Virtual COM Bridge for SimHub
Conecta ao ESP32-S3 via WiFi (TCP) e expõe como porta COM virtual no Windows
"""

import socket
import serial
import threading
import time
import sys

# ==========================================
# CONFIGURAÇÃO
# ==========================================
ESP32_IP = "192.168.0.223"      # IP do ESP32-S3
ESP32_PORT = 20777              # Porta TCP do SimHub
VIRTUAL_COM = "COM20"           # Porta COM virtual (ajuste conforme com0com)
BAUDRATE = 115200               # Não importa para virtual, mas SimHub espera

print("=" * 60)
print("TCP to COM Bridge for SimHub + ESP32-S3 WiFi")
print("=" * 60)
print(f"ESP32 Address: {ESP32_IP}:{ESP32_PORT}")
print(f"Virtual COM Port: {VIRTUAL_COM}")
print("=" * 60)

# ==========================================
# CONEXÃO TCP COM ESP32
# ==========================================
def connect_tcp():
    """Conecta ao ESP32 via TCP"""
    while True:
        try:
            print(f"\n[TCP] Connecting to {ESP32_IP}:{ESP32_PORT}...")
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((ESP32_IP, ESP32_PORT))
            sock.settimeout(None)  # Remove timeout após conectar
            print("[TCP] Connected!")
            return sock
        except Exception as e:
            print(f"[TCP] Connection failed: {e}")
            print("[TCP] Retrying in 3 seconds...")
            time.sleep(3)

# ==========================================
# CONEXÃO COM PORTA COM VIRTUAL
# ==========================================
def connect_serial():
    """Abre porta COM virtual"""
    while True:
        try:
            print(f"\n[COM] Opening {VIRTUAL_COM}...")
            ser = serial.Serial(VIRTUAL_COM, BAUDRATE, timeout=0.1)
            print(f"[COM] {VIRTUAL_COM} opened!")
            return ser
        except Exception as e:
            print(f"[COM] Failed to open {VIRTUAL_COM}: {e}")
            print("[COM] Retrying in 3 seconds...")
            time.sleep(3)

# ==========================================
# THREADS DE PONTE
# ==========================================
def tcp_to_com(tcp_sock, serial_port):
    """Lê dados do TCP e envia para COM"""
    print("[Bridge] TCP → COM thread started")
    buffer = bytearray()
    
    while True:
        try:
            data = tcp_sock.recv(1024)
            if not data:
                print("[TCP] Connection closed by ESP32")
                break
            
            buffer.extend(data)
            # Envia dados para COM
            serial_port.write(buffer)
            serial_port.flush()
            
            # Debug
            if len(buffer) > 0:
                print(f"[TCP→COM] {len(buffer)} bytes")
            
            buffer.clear()
            
        except socket.timeout:
            continue
        except Exception as e:
            print(f"[TCP→COM] Error: {e}")
            break

def com_to_tcp(serial_port, tcp_sock):
    """Lê dados do COM e envia para TCP"""
    print("[Bridge] COM → TCP thread started")
    
    while True:
        try:
            if serial_port.in_waiting > 0:
                data = serial_port.read(serial_port.in_waiting)
                if data:
                    tcp_sock.sendall(data)
                    print(f"[COM→TCP] {len(data)} bytes")
            else:
                time.sleep(0.01)  # Pequeno delay para não sobrecarregar CPU
                
        except Exception as e:
            print(f"[COM→TCP] Error: {e}")
            break

# ==========================================
# MAIN LOOP
# ==========================================
def main():
    while True:
        # Conecta TCP e COM
        tcp_sock = connect_tcp()
        serial_port = connect_serial()
        
        print("\n" + "=" * 60)
        print("BRIDGE ACTIVE - SimHub can now connect to", VIRTUAL_COM)
        print("Press Ctrl+C to stop")
        print("=" * 60 + "\n")
        
        # Inicia threads de ponte
        t1 = threading.Thread(target=tcp_to_com, args=(tcp_sock, serial_port), daemon=True)
        t2 = threading.Thread(target=com_to_tcp, args=(serial_port, tcp_sock), daemon=True)
        
        t1.start()
        t2.start()
        
        # Aguarda threads terminarem (conexão perdida)
        t1.join()
        t2.join()
        
        # Fecha conexões
        print("\n[Bridge] Connection lost, reconnecting...")
        try:
            tcp_sock.close()
        except:
            pass
        try:
            serial_port.close()
        except:
            pass
        
        time.sleep(2)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[Bridge] Stopped by user")
        sys.exit(0)
