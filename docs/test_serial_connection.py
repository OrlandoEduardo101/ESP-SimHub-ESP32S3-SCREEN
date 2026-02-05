#!/usr/bin/env python3
"""
Script para testar a comunicação serial com o ESP32-S3 WT32-SC01 Plus
Simula o handshake do SimHub e mostra a resposta bruta do firmware
"""

import serial
import time
import sys


# Tabela de CRC8 (mesma da biblioteca ARQSerial)
crc_table = [
    0,213,127,170,254,43,129,84,41,252,86,131,215,2,168,125,82,135,45,248,172,121,211,6,123,174,4,209,133,80,250,47,
    164,113,219,14,90,143,37,240,141,88,242,39,115,166,12,217,246,35,137,92,8,221,119,162,223,10,160,117,33,244,94,139,
    157,72,226,55,99,182,28,201,180,97,203,30,74,159,53,224,207,26,176,101,49,228,78,155,230,51,153,76,24,205,103,178,
    57,236,70,147,199,18,184,109,16,197,111,186,238,59,145,68,107,190,20,193,149,64,234,63,66,151,61,232,188,105,195,22,
    239,58,144,69,17,196,110,187,198,19,185,108,56,237,71,146,189,104,194,23,67,150,60,233,148,65,235,62,106,191,21,192,
    75,158,52,225,181,96,202,31,98,183,29,200,156,73,227,54,25,204,102,179,231,50,152,77,48,229,79,154,206,27,177,100,
    114,167,13,216,140,89,243,38,91,142,36,241,165,112,218,15,32,245,95,138,222,11,161,116,9,220,118,163,247,34,136,93,
    214,3,169,124,40,253,87,130,255,42,128,85,1,212,126,171,132,81,251,46,122,175,5,208,173,120,210,7,83,134,44,249
]


def update_crc(current, value):
    return crc_table[current ^ value]


def build_arq_packet(packet_id, data_bytes):
    """Monta um pacote ARQ: 0x01 0x01 <id> <len> <data...> <crc>"""
    length = len(data_bytes)
    crc = 0
    crc = update_crc(crc, packet_id)
    crc = update_crc(crc, length)
    for b in data_bytes:
        crc = update_crc(crc, b)
    packet = bytes([0x01, 0x01, packet_id, length]) + bytes(data_bytes) + bytes([crc])
    return packet


def read_with_timeout(ser, nbytes, label, timeout_s=1.0):
    ser.timeout = timeout_s
    data = ser.read(nbytes)
    print(f"    {label} ({len(data)} bytes): {data.hex()} | {data}")
    return data


def test_connection(port='COM11', baudrate=19200):
    print(f"[*] Conectando em {port} a {baudrate} bauds...")

    try:
        ser = serial.Serial(port, baudrate, timeout=1, write_timeout=None, rtscts=False, dsrdtr=False)
        # Avoid toggling DTR/RTS which resets the ESP32-S3 USB/JTAG
        ser.dtr = False
        ser.rts = False
        time.sleep(1.0)
        print("[✓] Porta aberta com sucesso!")

        # Limpar buffer de entrada
        ser.reset_input_buffer()
        time.sleep(0.1)

        # ---------- Teste 1: Hello ----------
        print("\n[*] Teste 1: Hello (packet 0, data=[0x03,0x31])")
        pkt = build_arq_packet(0, [0x03, 0x31])
        ser.write(pkt)
        ser.flush()
        # Espera ACK (0x03 <id>)
        ack = read_with_timeout(ser, 2, "ACK")
        # Espera resposta (max 8 bytes)
        resp = read_with_timeout(ser, 8, "RESP")

        # ---------- Teste 2: Features ----------
        print("\n[*] Teste 2: Features (packet 1, data=[0x03,0x30])")
        pkt = build_arq_packet(1, [0x03, 0x30])
        ser.write(pkt)
        ser.flush()
        ack = read_with_timeout(ser, 2, "ACK")
        resp = read_with_timeout(ser, 8, "RESP")

        # ---------- Teste 3: Device Name ----------
        print("\n[*] Teste 3: Device Name (packet 2, data=[0x03,0x4E])")
        pkt = build_arq_packet(2, [0x03, 0x4E])
        ser.write(pkt)
        ser.flush()
        ack = read_with_timeout(ser, 2, "ACK")
        resp = read_with_timeout(ser, 32, "RESP")

        # ---------- Teste 4: Unique ID ----------
        print("\n[*] Teste 4: Unique ID (packet 3, data=[0x03,0x49])")
        pkt = build_arq_packet(3, [0x03, 0x49])
        ser.write(pkt)
        ser.flush()
        ack = read_with_timeout(ser, 2, "ACK")
        resp = read_with_timeout(ser, 32, "RESP")

        ser.close()
        print("\n[✓] Testes concluídos!")

    except serial.SerialException as e:
        print(f"[!] Erro serial: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"[!] Erro: {e}")
        sys.exit(1)


if __name__ == "__main__":
    test_connection()
