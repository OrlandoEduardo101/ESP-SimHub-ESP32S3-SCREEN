#!/usr/bin/env python3
"""
SimHub Simulator para ESP32-S3 WT32-SC01 Plus Dashboard
========================================================
Emula a comunicação do SimHub com o ESP32:
  1. Handshake completo (ARQ + comandos SimHub)
  2. Loop de telemetria de corrida por 3 minutos

Uso:
    python3 scripts/simhub_simulator.py
    python3 scripts/simhub_simulator.py --port /dev/cu.usbmodem1101
    python3 scripts/simhub_simulator.py --port /dev/cu.usbmodem1101 --duration 300 --hz 20

Requisitos:
    pip3 install pyserial
"""

import serial
import time
import math
import glob
import argparse
import sys
import struct
from typing import Optional

# ──────────────────────────────────────────────────────────────────────────────
# CRC-8 table (idêntica à de ArqSerial.h)
# ──────────────────────────────────────────────────────────────────────────────
CRC8_TABLE = [
    0,213,127,170,254,43,129,84,41,252,86,131,215,2,168,125,82,135,45,248,
    172,121,211,6,123,174,4,209,133,80,250,47,164,113,219,14,90,143,37,240,
    141,88,242,39,115,166,12,217,246,35,137,92,8,221,119,162,223,10,160,117,
    33,244,94,139,157,72,226,55,99,182,28,201,180,97,203,30,74,159,53,224,
    207,26,176,101,49,228,78,155,230,51,153,76,24,205,103,178,57,236,70,147,
    199,18,184,109,16,197,111,186,238,59,145,68,107,190,20,193,149,64,234,63,
    66,151,61,232,188,105,195,22,239,58,144,69,17,196,110,187,198,19,185,108,
    56,237,71,146,189,104,194,23,67,150,60,233,148,65,235,62,106,191,21,192,
    75,158,52,225,181,96,202,31,98,183,29,200,156,73,227,54,25,204,102,179,
    231,50,152,77,48,229,79,154,206,27,177,100,114,167,13,216,140,89,243,38,
    91,142,36,241,165,112,218,15,32,245,95,138,222,11,161,116,9,220,118,163,
    247,34,136,93,214,3,169,124,40,253,87,130,255,42,128,85,1,212,126,171,
    132,81,251,46,122,175,5,208,173,120,210,7,83,134,44,249
]

def crc8(data: bytes) -> int:
    crc = 0
    for b in data:
        crc = CRC8_TABLE[crc ^ b]
    return crc


# ──────────────────────────────────────────────────────────────────────────────
# ARQ Protocol Layer
# ──────────────────────────────────────────────────────────────────────────────
class ARQProtocol:
    """
    Implementa o protocolo ARQ (Automatic Repeat reQuest) do SimHub.

    Pacote enviado (SimHub → ESP32):
        [0x01][0x01][packetID][length][payload...][CRC8]

    Respostas do ESP32 ao receber payload válido:
        [0x03][packetID]        → ACK
        [0x04][lastID][reason]  → NACK

    Respostas inline enquanto o ESP32 processa comandos:
        [0x08][byte]            → Write single byte
        [0x06][len][data][0x20] → Print string
        [0x07][len][data][0x20] → Debug print
        [0x09][type][len][data] → Custom packet
    """

    MAX_PAYLOAD = 32  # ARQ max payload size
    VERSION = ord('j')  # Firmware VERSION = 'j'

    def __init__(self, ser: serial.Serial):
        self.ser = ser
        self._packet_id = 0   # começa em 0, vai até 127, depois volta a 0
        self._debug = False

    def _next_id(self) -> int:
        """Retorna o próximo packet ID (0-127 ciclicamente)."""
        id_ = self._packet_id
        self._packet_id = 0 if self._packet_id >= 127 else self._packet_id + 1
        return id_

    def _build_packet(self, packet_id: int, payload: bytes) -> bytes:
        """Monta um pacote ARQ com CRC."""
        assert len(payload) <= self.MAX_PAYLOAD, f"Payload too large: {len(payload)}"
        crc_input = bytes([packet_id, len(payload)]) + payload
        crc = crc8(crc_input)
        return bytes([0x01, 0x01, packet_id, len(payload)]) + payload + bytes([crc])

    def _read_until_quiet(self, timeout: float) -> bytes:
        """
        Lê do serial até que a porta fique silenciosa por quiet_threshold
        segundos ou até atingir o timeout total.

        Importante: o quiet_threshold só é aplicado DEPOIS de receber pelo
        menos 1 byte.  Isso evita que latência variável do USB CDC (que pode
        demorar 50-200ms para entregar o primeiro byte no macOS) cause um
        retorno prematuro com dados vazios.
        """
        raw = b""
        deadline = time.time() + timeout
        last_rx = time.time()
        quiet_threshold = max(0.05, timeout / 4)
        got_data = False
        while time.time() < deadline:
            n = self.ser.in_waiting
            if n:
                raw += self.ser.read(n)
                last_rx = time.time()
                got_data = True
            else:
                # Só aplica silêncio após ter recebido algo
                if got_data and (time.time() - last_rx) >= quiet_threshold:
                    break
                time.sleep(0.002)
        return raw

    def _decode_response(self, raw: bytes) -> bytes:
        """Interpreta o stream de resposta do ESP32 e retorna bytes de dados úteis."""
        decoded = []
        i = 0
        while i < len(raw):
            b = raw[i]
            if b == 0x03 and i + 1 < len(raw):
                # ACK [0x03][id]
                if self._debug:
                    print(f"    [ARQ] ACK id={raw[i+1]}")
                i += 2
            elif b == 0x04 and i + 2 < len(raw):
                # NACK [0x04][lastID][reason]
                if self._debug:
                    print(f"    [ARQ] NACK lastID={raw[i+1]} reason={raw[i+2]:#04x}")
                i += 3
            elif b == 0x08 and i + 1 < len(raw):
                # Write single byte [0x08][byte]
                decoded.append(raw[i+1])
                i += 2
            elif b == 0x06 and i + 1 < len(raw):
                # Print string [0x06][len][data...][0x20]
                str_len = raw[i+1]
                end = i + 2 + str_len
                if end <= len(raw):
                    decoded.extend(raw[i+2:end])
                    i = end + 1  # skip 0x20 terminator
                else:
                    i += 1
            elif b == 0x07 and i + 1 < len(raw):
                # Debug print — skip
                str_len = raw[i+1]
                i += 2 + str_len + 1
            elif b == 0x09 and i + 2 < len(raw):
                # Custom packet — skip
                pkt_len = raw[i+2] if i + 2 < len(raw) else 0
                i += 3 + pkt_len
            else:
                i += 1
        return bytes(decoded)

    def send_command(self, command_bytes: bytes, drain_timeout: float = 0.4) -> bytes:
        """
        Envia um comando via ARQ (geralmente 1 pacote para handshake).
        Divide o payload em chunks de MAX_PAYLOAD, envia todos de uma vez
        sem esperar ACK entre eles — os ACKs chegam enquanto o próximo
        pacote já está a caminho, mantendo o pipeline fluindo.
        Depois faz um único drain que captura ACKs + bytes de resposta juntos.
        """
        chunks = [command_bytes[i:i+self.MAX_PAYLOAD]
                  for i in range(0, len(command_bytes), self.MAX_PAYLOAD)]

        # Envia todos os chunks em rajada
        for chunk in chunks:
            pid = self._next_id()
            pkt = self._build_packet(pid, chunk)
            self.ser.write(pkt)
        self.ser.flush()

        # Lê tudo que chegar até ficar quieto
        raw = self._read_until_quiet(drain_timeout)
        if self._debug and raw:
            print(f"    [ARQ] resp raw={raw.hex()}")
        return self._decode_response(raw)

    def stream_telemetry(self, command_bytes: bytes) -> None:
        """
        Envia telemetria com pacing por ACK para evitar overflow do
        DataBuffer (RingBuf<uint8_t, 32>) no firmware.

        Problema do burst send:
          O firmware tem ProcessIncomingData() que processa TODOS os pacotes
          disponíveis no serial de uma vez (while StreamAvailable > 0).
          Cada pacote faz push() de até 32 bytes no DataBuffer de 32 bytes.
          Se múltiplos pacotes chegam de uma vez, push() retorna false e
          os dados são SILENCIOSAMENTE DESCARTADOS.

        Solução:
          Enviar um pacote, esperar pelo ACK real (byte 0x03 seguido de
          packetID), e só então enviar o próximo. Antes de cada envio,
          drenamos qualquer lixo (debug output do ESP32) do buffer.
          O ACK chega em ~2-5ms via USB CDC.

        O timeout de 400ms por byte em read() é respeitado porque cada
        chamada a read() reinicia seu timer de 400ms, e o próximo pacote
        chega dentro de ~5ms após o ACK.
        """
        chunks = [command_bytes[i:i+self.MAX_PAYLOAD]
                  for i in range(0, len(command_bytes), self.MAX_PAYLOAD)]

        for chunk in chunks:
            pid = self._next_id()
            pkt = self._build_packet(pid, chunk)

            # Drena dados residuais (debug output do ESP32) antes de enviar
            if self.ser.in_waiting:
                self.ser.read(self.ser.in_waiting)

            self.ser.write(pkt)
            self.ser.flush()

            # Espera ACK real: procura bytes [0x03][pid] na resposta.
            # O ESP32 pode misturar debug output (Serial.println) no mesmo
            # stream USB CDC, então precisamos procurar o ACK nos bytes.
            ack_deadline = time.time() + 0.03  # 30ms max
            ack_found = False
            while time.time() < ack_deadline:
                if self.ser.in_waiting >= 2:
                    data = self.ser.read(self.ser.in_waiting)
                    # Procura padrão ACK [0x03][pid] nos bytes recebidos
                    for i in range(len(data) - 1):
                        if data[i] == 0x03 and data[i+1] == pid:
                            ack_found = True
                            break
                    if ack_found:
                        break
                time.sleep(0.001)

            # Se não encontrou ACK em 30ms, espera um pouco mais para
            # dar tempo ao ESP32 de consumir o DataBuffer
            if not ack_found:
                time.sleep(0.005)

        # Drena resposta final (0x15 ACK do Command_CustomProtocolData)
        time.sleep(0.005)
        if self.ser.in_waiting:
            self.ser.read(self.ser.in_waiting)

    def reset_packet_id(self):
        """Reinicia o packet ID para 0 (útil no início do handshake)."""
        self._packet_id = 0

    def send_command_force(self, command_bytes: bytes, drain_timeout: float = 0.8) -> bytes:
        """
        Envia um comando usando packet ID 255 (aceito incondicionalmente
        pelo ESP32 independente do estado da sequência ARQ).
        Útil para retry quando o estado de sequência pode estar dessincronizado.
        """
        chunks = [command_bytes[i:i+self.MAX_PAYLOAD]
                  for i in range(0, len(command_bytes), self.MAX_PAYLOAD)]
        for chunk in chunks:
            pkt = self._build_packet(255, chunk)
            self.ser.write(pkt)
        self.ser.flush()
        raw = self._read_until_quiet(drain_timeout)
        if self._debug and raw:
            print(f"    [ARQ] resp raw={raw.hex()}")
        return self._decode_response(raw)


# ──────────────────────────────────────────────────────────────────────────────
# SimHub Handshake
# ──────────────────────────────────────────────────────────────────────────────
MSG_HEADER = 0x03

def do_handshake(arq: ARQProtocol) -> bool:
    """
    Executa o handshake completo do SimHub:
      1. Hello         → espera VERSION 'j'
      2. Expanded list → "mcutype\nkeepalive\n\n"
      3. MCU type      → [0x1E][0x98][0x01]
      4. Features      → "NIPX\n"
      5. Device Name   → "ESP-SimHubDisplay\n"
      6. Unique ID     → "<MAC>\n"
      7. Buttons count → 0x00
      8. TM1638 count  → 0x00
      9. Simple modules → 0x00
      10. RGB LEDs count → 0x00
      11. Acquisition   → 0x03

    Retorna True se bem-sucedido.
    """
    print("\n" + "═"*60)
    print("  HANDSHAKE SimHub → ESP32")
    print("═"*60)

    success = True

    # ── 1. Hello (com retry — caso touch init ainda esteja rodando) ──
    print("  [1/11] Hello (0x03 0x31)...", end=" ", flush=True)
    hello_ok = False
    hello_payload = bytes([MSG_HEADER, 0x31, 0x00])
    for attempt in range(5):
        if attempt > 0:
            print(f"retry {attempt}...", end=" ", flush=True)
            # Limpa qualquer dado pendente
            time.sleep(0.3)
            arq.ser.reset_input_buffer()
        # Na primeira tentativa usa ID normal; nas seguintes, ID 255
        # (aceito incondicionalmente pelo ESP32)
        if attempt == 0:
            resp = arq.send_command(hello_payload, drain_timeout=0.8)
        else:
            resp = arq.send_command_force(hello_payload, drain_timeout=0.8)
        if ARQProtocol.VERSION in resp:
            print(f"OK — VERSION='{chr(ARQProtocol.VERSION)}' (0x{ARQProtocol.VERSION:02X})")
            hello_ok = True
            # Sincroniza o packet ID: ESP32 agora tem LastValidPacket=255,
            # próximo esperado = 0
            arq.reset_packet_id()
            break
    if not hello_ok:
        print(f"FAIL — resp={resp.hex() if resp else 'empty'}")
        success = False
        # Mesmo falhando, reseta para tentar os próximos passos
        arq.reset_packet_id()
        arq.ser.reset_input_buffer()

    # ── 2. Expanded commands list ─────────────────────────────────
    print("  [2/11] Expanded commands list (X list)...", end=" ", flush=True)
    payload = bytes([MSG_HEADER, ord('X')]) + b"list\n"
    resp = arq.send_command(payload, drain_timeout=0.5)
    # Espera 0x15 ACK no stream de bytes escritos
    if resp:
        print(f"OK — resp={resp.hex()}")
    else:
        print(f"OK (no data, normal)")

    # ── 3. MCU Type ──────────────────────────────────────────────
    print("  [3/11] MCU Type (X mcutype)...", end=" ", flush=True)
    payload = bytes([MSG_HEADER, ord('X')]) + b"mcutype\n"
    resp = arq.send_command(payload, drain_timeout=0.5)
    # Espera [0x1E][0x98][0x01] (Arduino Mega signature)
    if b'\x1e\x98\x01' in resp or len(resp) >= 3:
        sig = resp[:3].hex() if len(resp) >= 3 else resp.hex()
        print(f"OK — sig={sig}")
    else:
        print(f"OK (resp={resp.hex() if resp else 'empty'})")

    # ── 4. Features ───────────────────────────────────────────────
    print("  [4/11] Features (0x30)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x30]), drain_timeout=0.5)
    features = resp.decode('ascii', errors='replace').strip()
    if features:
        print(f"OK — features='{features}'")
    else:
        print(f"OK (no data)")

    # ── 5. Device Name ────────────────────────────────────────────
    print("  [5/11] Device Name (0x4E)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x4E]), drain_timeout=0.5)
    name = resp.decode('ascii', errors='replace').strip('\x00\x15\n ')
    if name:
        print(f"OK — name='{name}'")
    else:
        print(f"OK (resp={resp.hex() if resp else 'empty'})")

    # ── 6. Unique ID ──────────────────────────────────────────────
    print("  [6/11] Unique ID (0x49)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x49]), drain_timeout=0.5)
    uid = resp.decode('ascii', errors='replace').strip('\x00\x15\n ')
    if uid:
        print(f"OK — id='{uid}'")
    else:
        print(f"OK (resp={resp.hex() if resp else 'empty'})")

    # ── 7. Buttons count ──────────────────────────────────────────
    print("  [7/11] Buttons count (0x4A)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x4A]), drain_timeout=0.3)
    print(f"OK — count={resp[0] if resp else '?'}")

    # ── 8. TM1638 count ───────────────────────────────────────────
    print("  [8/11] TM1638 count (0x32)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x32]), drain_timeout=0.3)
    print(f"OK — count={resp[0] if resp else '?'}")

    # ── 9. Simple modules count ───────────────────────────────────
    print("  [9/11] Simple modules count (0x42)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x42]), drain_timeout=0.3)
    print(f"OK — count={resp[0] if resp else '?'}")

    # ── 10. RGB LEDs count ────────────────────────────────────────
    print("  [10/11] RGB LEDs count (0x34)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x34]), drain_timeout=0.3)
    leds = resp[0] if resp else 0
    print(f"OK — count={leds}")

    # ── 11. Acquisition ───────────────────────────────────────────
    print("  [11/11] Acquisition (0x41)...", end=" ", flush=True)
    resp = arq.send_command(bytes([MSG_HEADER, 0x41]), drain_timeout=0.4)
    # Espera [0x08][0x03] → byte 0x03
    if 0x03 in resp or resp:
        print(f"OK — resp={resp.hex() if resp else 'empty'}")
    else:
        print(f"OK")

    print("═"*60)
    if success:
        print("  ✓ Handshake completo! Iniciando telemetria...\n")
    else:
        print("  ⚠  Handshake com erros (continuando mesmo assim)...\n")
    return True  # Continue even if some steps had issues


# ──────────────────────────────────────────────────────────────────────────────
# Race Telemetry Simulator
# ──────────────────────────────────────────────────────────────────────────────
class RaceSimulator:
    """
    Simula uma corrida num circuito fictício (~85s/volta).
    Gera os 62 campos do protocolo customizado do SimHub.

    Segmentos do circuito (proporção do total da volta):
      0.00–0.08  Reta principal     → aceleração máxima, 6ª marcha
      0.08–0.14  Curva 1 (chicane)  → frenagem forte, 2ª/3ª marcha
      0.14–0.22  Setor médio        → 4ª/5ª marcha
      0.22–0.30  Reta secundária    → 5ª/6ª marcha
      0.30–0.38  Setor técnico      → 2ª/3ª marcha, muitas curvas
      0.38–0.50  Reta dos fundos    → velocidade máxima, 6ª marcha
      0.50–0.58  Chicane dupla      → frenagem, 2ª marcha
      0.58–0.66  Setor médio 2      → 4ª/5ª marcha
      0.66–0.74  Curva rápida       → 5ª marcha, G lateral alto
      0.74–0.82  Frenagem final     → da 6ª para 2ª
      0.82–1.00  Saída + reta       → aceleração e voltar ao começo
    """

    LAP_DURATION = 85.0  # segundos por volta
    FUEL_PER_LAP = 2.34  # litros por volta
    TOTAL_FUEL   = 30.0  # litros iniciais (~12 voltas)

    # Sequência de alertas que vai rotacionar durante a corrida
    ALERT_SEQUENCE = [
        "NORMAL", "NORMAL", "NORMAL",
        "BLUE FLAG",
        "NORMAL", "NORMAL",
        "YELLOW FLAG",
        "NORMAL", "NORMAL",
        "PIT LIMITER",
        "NORMAL", "NORMAL", "NORMAL",
        "LOW FUEL",
        "NORMAL",
        "GREEN FLAG",
        "NORMAL", "NORMAL",
    ]

    def __init__(self, total_duration: float = 180.0):
        self.total_duration   = total_duration
        self.start_time       = time.time()
        self.last_alert_change = time.time()
        self.alert_idx        = 0
        self.alert_change_interval = 30.0  # mudar alerta a cada 30s

        # Acúmulo de estado
        self.fuel             = self.TOTAL_FUEL
        self.lap_count        = 0
        self.tyre_wear_fl     = 100  # 100% = novo
        self.tyre_wear_fr     = 100
        self.tyre_wear_rl     = 100
        self.tyre_wear_rr     = 100
        self.last_lap_time    = "00:00.000"
        self.best_lap_time    = "00:00.000"
        self.prev_position    = 0.0
        self.lap_start_time   = time.time()
        self.sector1_done     = False
        self.sector2_done     = False
        self.sector1_time     = ""   # vazio até cruzar o setor (igual SimHub real)
        self.sector2_time     = ""
        self.sector3_time     = ""

        # Popup tracking
        self.popup_message    = ""
        self.popup_until      = 0.0
        self.last_tc_level    = 3
        self.last_abs_level   = 2
        self.last_bias        = 58.0
        self.popup_cycle_time = 20.0
        self.last_popup_change = time.time()

    def _fmt_lap_time(self, seconds: float) -> str:
        """Formata segundos como mm:ss.fff"""
        if seconds <= 0:
            return "00:00.000"
        m = int(seconds // 60)
        s = seconds % 60
        ms = int((s - int(s)) * 1000)
        return f"{m:02d}:{int(s):02d}.{ms:03d}"

    def _fmt_sector_time(self, seconds: float) -> str:
        """Formata segundos como ss.fff"""
        if seconds <= 0:
            return "00.000"
        ms = int((seconds - int(seconds)) * 1000)
        return f"{int(seconds):02d}.{ms:03d}"

    def _track_profile(self, pos: float) -> dict:
        """
        Dado pos ∈ [0, 1), retorna o estado físico do carro nesse ponto:
        speed, gear, rpm_pct, brake, throttle, drs_available, etc.
        """
        p = pos % 1.0

        # Perfil do circuito: lista de (inicio, fim, velocidade_alvo, marcha, brake%)
        segments = [
            (0.00, 0.08, 290, 6, 0,   "reta principal"),
            (0.08, 0.14,  80, 2, 95,  "chicane 1"),
            (0.14, 0.22, 180, 4, 0,   "setor médio"),
            (0.22, 0.30, 250, 5, 0,   "reta secundária"),
            (0.30, 0.38, 100, 3, 70,  "setor técnico"),
            (0.38, 0.50, 280, 6, 0,   "reta dos fundos"),
            (0.50, 0.58,  60, 2, 100, "chicane dupla"),
            (0.58, 0.66, 170, 4, 0,   "setor médio 2"),
            (0.66, 0.74, 220, 5, 10,  "curva rápida"),
            (0.74, 0.82,  75, 2, 90,  "frenagem final"),
            (0.82, 1.00, 200, 5, 0,   "saída+reta"),
        ]

        # Encontra o segmento atual
        seg = segments[-1]
        for s in segments:
            if s[0] <= p < s[1]:
                seg = s
                break

        seg_start, seg_end, v_target, gear, brake_pct, _label = seg

        # Progress dentro do segmento (0→1)
        seg_len = seg_end - seg_start
        seg_progress = (p - seg_start) / seg_len if seg_len > 0 else 0.5

        # Transição suave de velocidade com sigmoid
        def smooth(t):
            return t * t * (3 - 2 * t)

        # Velocidade com variação senoidal para realismo (fator ±5%)
        noise = 1.0 + 0.03 * math.sin(p * 47.3 + 1.2)
        speed = v_target * noise

        # RPM percentage baseado na velocidade e marcha
        gear_top_speeds = {1: 80, 2: 130, 3: 180, 4: 220, 5: 260, 6: 310}
        gear_max = gear_top_speeds.get(gear, 200)
        rpm_pct = min(95, max(30, int((speed / gear_max) * 100)))

        # RPM efetivo (simula motor ~8000-13000 RPM)
        rpm = int(rpm_pct / 100 * 12000)

        # Brake
        brake = int(brake_pct * smooth(min(seg_progress * 3, 1.0))) if brake_pct > 0 else 0

        # DRS: disponível e ativo na reta principal e reta dos fundos
        drs_zones = [(0.00, 0.08), (0.38, 0.50)]
        drs_available = any(s <= p < e for s, e in drs_zones)
        drs_active = drs_available and speed > 200

        # Turbo boost (simula 0.8–1.6 Bar dependendo do RPM)
        turbo = round(0.4 + (rpm_pct / 100.0) * 1.2 + 0.1 * math.sin(p * 23), 1)

        # KERS/ERS level (0-100%, carrega em frenagem, usa em aceleração)
        kers = int(50 + 40 * math.sin(p * 2 * math.pi - 0.5))
        kers = max(0, min(100, kers))

        # Shift light: acende se RPM > 95%
        shift_light = 1 if rpm_pct >= 95 else 0

        return {
            "speed": int(speed),
            "gear": str(gear),
            "rpm_pct": rpm_pct,
            "rpm": rpm,
            "brake": brake,
            "drs_available": 1 if drs_available else 0,
            "drs_active": 1 if drs_active else 0,
            "turbo": turbo,
            "kers": kers,
            "shift_light": shift_light,
        }

    def _update_alert(self) -> str:
        """Atualiza o alerta ciclicamente."""
        now = time.time()
        if now - self.last_alert_change >= self.alert_change_interval:
            self.alert_idx = (self.alert_idx + 1) % len(self.ALERT_SEQUENCE)
            self.last_alert_change = now
        alert = self.ALERT_SEQUENCE[self.alert_idx]

        # Sobrescreve com PIT LIMITER se velocidade baixa + pit limiter ativo
        return alert

    def _update_popup(self, physical: dict) -> str:
        """Gera pop-ups temporários (mudança de TC, ABS, bias)."""
        now = time.time()

        # Expirou?
        if now > self.popup_until:
            self.popup_message = ""

        # Cicla mudanças de ajuste a cada ~20s
        if now - self.last_popup_change >= self.popup_cycle_time:
            self.last_popup_change = now
            cycle = int((now - self.start_time) // self.popup_cycle_time) % 4
            if cycle == 0:
                self.last_bias = round(self.last_bias + 0.5, 1)
                if self.last_bias > 65: self.last_bias = 55.0
                self.popup_message = f"BIAS: {self.last_bias:.1f}"
                self.popup_until = now + 1.5
            elif cycle == 1:
                self.last_tc_level = (self.last_tc_level % 8) + 1
                self.popup_message = f"TC LEVEL: {self.last_tc_level}"
                self.popup_until = now + 1.5
            elif cycle == 2:
                self.last_abs_level = (self.last_abs_level % 5) + 1
                self.popup_message = f"ABS LEVEL: {self.last_abs_level}"
                self.popup_until = now + 1.5
            else:
                pass  # sem popup

        return self.popup_message

    def generate_frame(self) -> str:
        """
        Gera uma string de telemetria no formato do protocolo SimHub
        com exatamente 62 campos separados por ';'.
        Termina com ';' após o último campo — essencial para que o
        ESP32 não faça timeout de 400ms no último ReadStringUntil(';').
        """
        now        = time.time()
        elapsed    = now - self.start_time
        lap_elapsed = now - self.lap_start_time

        # Posição na pista (0.0 a 1.0)
        position_on_track = (lap_elapsed / self.LAP_DURATION) % 1.0

        # Detecta nova volta
        if position_on_track < self.prev_position:
            self.lap_count += 1
            lap_time = now - self.lap_start_time
            self.last_lap_time = self._fmt_lap_time(lap_time)
            if self.best_lap_time == "00:00.000" or lap_time < self._parse_lap_time(self.best_lap_time):
                self.best_lap_time = self.last_lap_time
            self.lap_start_time = now
            self.fuel = max(0, self.fuel - self.FUEL_PER_LAP)
            # Desgaste dos pneus
            self.tyre_wear_fl = max(0, self.tyre_wear_fl - 3.2)
            self.tyre_wear_fr = max(0, self.tyre_wear_fr - 3.5)
            self.tyre_wear_rl = max(0, self.tyre_wear_rl - 2.8)
            self.tyre_wear_rr = max(0, self.tyre_wear_rr - 3.0)
            # Reset setores (volta nova: limpa tempos)
            self.sector1_done = False
            self.sector2_done = False
            self.sector1_time = ""
            self.sector2_time = ""
            self.sector3_time = ""

        # Setores (split a cada 1/3 da volta)
        if not self.sector1_done and position_on_track >= 0.333:
            self.sector1_time = self._fmt_sector_time(lap_elapsed * 0.333)
            self.sector1_done = True
        if not self.sector2_done and position_on_track >= 0.666:
            self.sector2_time = self._fmt_sector_time(lap_elapsed * 0.333)
            self.sector2_done = True

        self.prev_position = position_on_track

        # Estado físico
        phys = self._track_profile(position_on_track)

        # Tempo de volta corrente
        current_lap = self._fmt_lap_time(lap_elapsed)

        # Delta (simulado) — formato '0.000' igual ao SimHub real (sem sinal '+' para positivo)
        delta_sec = round(0.3 * math.sin(elapsed / 8.0), 3)
        delta_pct = round((delta_sec + 1.5) / 3.0, 2)
        delta_pct = max(0.0, min(1.0, delta_pct))

        # Combustível
        fuel_laps = round(self.fuel / self.FUEL_PER_LAP, 1)

        # Tempo de sessão restante (formato hh:mm:ss igual SimHub real)
        remaining = max(0, self.total_duration - elapsed)
        h = int(remaining // 3600)
        m = int((remaining % 3600) // 60)
        s = int(remaining % 60)
        session_time_left = f"{h:02d}:{m:02d}:{s:02d}"

        # Temperaturas (variam com posição)
        base_tyre_temp = 85 + int(20 * (phys["rpm_pct"] / 100.0))
        tfl = base_tyre_temp + 3
        tfr = base_tyre_temp + 2
        trl = base_tyre_temp - 1
        trr = base_tyre_temp

        # Pressão dos pneus (PSI)
        p_base = 28.5 + 1.0 * math.sin(elapsed / 30)
        pfl = round(p_base + 0.3, 1)
        pfr = round(p_base + 0.2, 1)
        prl = round(p_base - 0.1, 1)
        prr = round(p_base, 1)

        # Temperatura freios
        brake_temp = 200 + int(400 * (phys["brake"] / 100.0))
        bfl = brake_temp + 20
        bfr = brake_temp + 15
        brl = brake_temp - 30
        brr = brake_temp - 25

        # Motor
        oil_temp   = 110 + int(5 * math.sin(elapsed / 20))
        water_temp = 90 + int(5 * math.sin(elapsed / 15 + 1.0))

        # Bandeira
        alert      = self._update_alert()
        popup      = self._update_popup(phys)

        # Sincroniza flag com alerta
        flag_map = {
            "YELLOW FLAG": "Yellow",
            "BLUE FLAG":   "Blue",
            "GREEN FLAG":  "Green",
            "FINISHED":    "Checkered",
            "BLACK FLAG":  "Black",
        }
        current_flag = flag_map.get(alert, "None")

        # Spotter (simula carro adjacente aleatoriamente)
        spotter_left  = 1 if (int(elapsed * 3) % 20 == 7) else 0
        spotter_right = 1 if (int(elapsed * 3) % 20 == 14) else 0

        # LapInvalidated (simula ocasionalmente)
        lap_inv = "True" if (int(elapsed) % 90 == 45) else "False"

        # Setor 3 estimado
        if lap_elapsed > self.LAP_DURATION * 0.666:
            s3 = self._fmt_sector_time(lap_elapsed - self.LAP_DURATION * 0.666)
        else:
            s3 = self.sector3_time

        # Posição na corrida e adversários
        race_pos = max(1, 5 - int(elapsed / 60))  # sobe de posição com o tempo
        opponents = 20

        # Gap pode ser negativo no SimHub real (ex: -14.4 = carro à frente está 14.4s à frente)
        gap_ahead  = "--" if race_pos == 1 else str(round(1.5 - elapsed / 60 + 2.0 * math.sin(elapsed / 15), 1))
        gap_behind = str(round(0.8 + 0.3 * math.sin(elapsed / 5), 1))

        # ABS/TC ativos quando freando/acelerando
        abs_active = 1 if phys["brake"] > 60 else 0
        tc_active  = 1 if phys["rpm_pct"] > 85 and phys["brake"] == 0 else 0
        tc_cut     = 1 if tc_active and phys["rpm_pct"] > 92 else 0

        # Monta os 62 campos
        fields = [
            # Bloco 1: Telemetria básica (0-4)
            str(phys["speed"]),                         # [0]  Velocidade (Km/h)
            phys["gear"],                               # [1]  Marcha (N, R, 1-6)
            str(phys["rpm_pct"]),                       # [2]  RPM % (0-100)
            "95",                                       # [3]  RPM Redline (% barra LED)
            str(phys["rpm"]),                           # [4]  RPM Atual (valor exato)

            # Bloco 2: Cronometragem (5-10)
            current_lap,                                # [5]  tempo volta atual
            self.last_lap_time,                         # [6]  último tempo
            self.best_lap_time,                         # [7]  melhor tempo
            f"{delta_sec:.3f}",                         # [8]  delta segundos (sem '+', igual SimHub real)
            f"{delta_pct:.2f}",                         # [9]  delta barra
            lap_inv,                                    # [10] volta inválida

            # Bloco 3: Física e Pneus (11-24)
            f"{pfl:04.1f}",                             # [11] pressão FL
            f"{pfr:04.1f}",                             # [12] pressão FR
            f"{prl:04.1f}",                             # [13] pressão RL
            f"{prr:04.1f}",                             # [14] pressão RR
            str(tfl),                                   # [15] temp pneu FL
            str(tfr),                                   # [16] temp pneu FR
            str(trl),                                   # [17] temp pneu RL
            str(trr),                                   # [18] temp pneu RR
            str(bfl),                                   # [19] temp freio FL
            str(bfr),                                   # [20] temp freio FR
            str(brl),                                   # [21] temp freio RL
            str(brr),                                   # [22] temp freio RR
            str(oil_temp),                              # [23] temp óleo
            str(water_temp),                            # [24] temp água

            # Bloco 4: Eletrônica (25-31)
            str(self.last_tc_level),                    # [25] nível TC
            str(tc_active),                             # [26] TC ativo
            str(self.last_abs_level),                   # [27] nível ABS
            str(abs_active),                            # [28] ABS ativo
            str(tc_cut),                                # [29] TC corte
            f"{self.last_bias:04.1f}",                   # [30] brake bias (format '00.0')
            str(phys["brake"]),                         # [31] pedal freio

            # Bloco 5: Estratégia (32-41)
            str(race_pos),                              # [32] posição
            str(opponents),                             # [33] total pilotos
            gap_ahead,                                  # [34] Gap frente ("--" se P1)
            gap_behind,                                 # [35] Gap atrás
            f"{fuel_laps:.1f}",                         # [36] combustível (voltas)
            f"{self.FUEL_PER_LAP:.2f}",                 # [37] combustível (L/volta)
            session_time_left,                          # [38] tempo sessão restante
            current_flag,                               # [39] bandeira
            "0",                                        # [40] penalidades
            "0",                                        # [41] avisos corte

            # Bloco 6: Alertas (42-43)
            alert,                                      # [42] alerta crítico
            popup,                                      # [43] pop-up temporário

            # Bloco 7: LED data (44-47)
            str(phys["rpm_pct"]),                       # [44] RPM % (repetido)
            str(spotter_left),                          # [45] spotter esq
            str(spotter_right),                         # [46] spotter dir
            str(abs_active),                            # [47] ABS ativo (repetido)

            # Bloco 8: Desgaste e ambiente (48-61)
            str(int(self.tyre_wear_fl)),                # [48] wear FL
            str(int(self.tyre_wear_fr)),                # [49] wear FR
            str(int(self.tyre_wear_rl)),                # [50] wear RL
            str(int(self.tyre_wear_rr)),                # [51] wear RR
            self.sector1_time,                          # [52] setor 1
            self.sector2_time,                          # [53] setor 2
            s3,                                         # [54] setor 3
            "24",                                       # [55] temp ar
            "35",                                       # [56] temp pista
            str(phys["shift_light"]),                   # [57] shift light
            str(phys["drs_available"]),                 # [58] DRS disponível
            str(phys["drs_active"]),                    # [59] DRS ativo
            # KERS e Turbo: maioria dos carros no AC não tem, SimHub envia vazio.
            # Simulador mostra valores nas primeiras 2 voltas para testar display,
            # depois envia vazio (igual comportamento real AC).
            str(phys["kers"]) if self.lap_count < 2 else "",   # [60] KERS/ERS %
            str(phys["turbo"]) if self.lap_count < 2 else "",  # [61] Pressão turbo (Bar)
        ]

        assert len(fields) == 62, f"Expected 62 fields, got {len(fields)}"
        return ";".join(fields) + ";"  # Termina com ';' para flush do último campo

    def _parse_lap_time(self, t: str) -> float:
        """Converte mm:ss.fff para segundos."""
        try:
            parts = t.split(":")
            m = int(parts[0])
            s = float(parts[1])
            return m * 60 + s
        except:
            return 999.0

    def is_finished(self) -> bool:
        return time.time() - self.start_time >= self.total_duration


# ──────────────────────────────────────────────────────────────────────────────
# Telemetry sender
# ──────────────────────────────────────────────────────────────────────────────
def send_telemetry(arq: ARQProtocol, data_str: str) -> None:
    """
    Envia um frame de telemetria via comando 'P' (0x50).
    Formato wire: [0x03][0x50][data_fields...]

    Usa stream_telemetry() em vez de send_command() para garantir que
    todos os ~400 bytes cheguem ao ESP32 dentro da janela de 400ms/byte
    do FlowSerialTimedRead(). Esperar ACK entre chunks quebraria o timing.
    """
    payload = bytes([MSG_HEADER, 0x50]) + data_str.encode('ascii')
    arq.stream_telemetry(payload)


# ──────────────────────────────────────────────────────────────────────────────
# Serial port detection
# ──────────────────────────────────────────────────────────────────────────────
def find_port() -> Optional[str]:
    """Auto-detects the ESP32 USB CDC port on macOS."""
    candidates = []
    for pattern in ["/dev/cu.usbmodem*", "/dev/cu.SLAB_USBtoUART*", "/dev/cu.usbserial*"]:
        candidates.extend(glob.glob(pattern))

    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]

    print("Múltiplas portas encontradas:")
    for i, p in enumerate(candidates):
        print(f"  [{i}] {p}")
    choice = input("Escolha [0]: ").strip()
    idx = int(choice) if choice.isdigit() else 0
    return candidates[idx]


# ──────────────────────────────────────────────────────────────────────────────
# ESP32 boot readiness detection
# ──────────────────────────────────────────────────────────────────────────────
def wait_for_esp32_ready(ser: 'serial.Serial', timeout: float = 12.0) -> bool:
    """
    Aguarda o ESP32 terminar o boot E a inicialização do touch.

    O firmware imprime mensagens de boot pelo USB CDC (Serial) e, quando
    o loop() começa, emite "Aguardando SimHub...".  Porém, logo em seguida,
    shCustomProtocol.loop() chama initializeTouch() que bloqueia o loop
    por ~1-2s (delays + I2C scan).  Durante esse tempo, o ESP32 NÃO
    processa pacotes ARQ.

    Marcadores em ordem cronológica:
      1. "Aguardando SimHub..."           — loop() iniciou
      2. "TOUCH INITIALIZATION START"     — initializeTouch() chamado
      3. "TOUCH INITIALIZATION END"       — touch pronto, loop livre

    Esperamos pelo ÚLTIMO marcador para garantir que o ESP32 está
    realmente pronto para processar comandos.
    """
    # Marcadores em ordem de prioridade (do mais tardio ao mais cedo)
    READY_MARKERS = [
        b"TOUCH INITIALIZATION END",      # melhor — loop está livre
        b"TOUCH: SUCCESS",                 # touch encontrado
        b"TOUCH: ERROR",                   # touch não encontrado mas init acabou
        b"Aguardando SimHub",              # fallback — loop iniciou
        b"Setup complete",                 # fallback
        b"Display init OK",               # fallback
    ]
    # O marcador "definitivo" é qualquer um dos 3 primeiros (pós-touch-init)
    DEFINITIVE_MARKERS = READY_MARKERS[:3]

    print("  Aguardando ESP32 inicializar...", end="", flush=True)
    deadline = time.time() + timeout
    buf = b""
    found = False
    found_definitive = False

    while time.time() < deadline:
        n = ser.in_waiting
        if n:
            chunk = ser.read(n)
            buf += chunk
            # Verifica marcadores definitivos primeiro
            for marker in DEFINITIVE_MARKERS:
                if marker in buf:
                    found = True
                    found_definitive = True
                    break
            if found_definitive:
                break
            # Verifica marcadores de fallback
            if not found:
                for marker in READY_MARKERS:
                    if marker in buf:
                        found = True
                        break
            # Evita acúmulo excessivo
            if len(buf) > 1024:
                buf = buf[-1024:]
        else:
            time.sleep(0.05)

    if found_definitive:
        print(" pronto! (touch init completo)")
    elif found:
        # Encontramos "Aguardando SimHub" mas NÃO o fim do touch init.
        # Esperamos um pouco mais para o touch init terminar.
        print(" (esperando touch init)...", end="", flush=True)
        extra_deadline = time.time() + 3.0
        while time.time() < extra_deadline:
            n = ser.in_waiting
            if n:
                chunk = ser.read(n)
                buf += chunk
                for marker in DEFINITIVE_MARKERS:
                    if marker in buf:
                        found_definitive = True
                        break
                if found_definitive:
                    break
                if len(buf) > 1024:
                    buf = buf[-1024:]
            else:
                time.sleep(0.05)
        if found_definitive:
            print(" pronto!")
        else:
            print(" timeout (touch init pode não ter terminado)")
    else:
        print(f" timeout ({timeout:.0f}s) — continuando mesmo assim.")

    # Aguarda o loop() estabilizar após touch init e limpa buffers
    time.sleep(0.5)
    ser.reset_input_buffer()
    return found


# ──────────────────────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="SimHub Simulator — Emula o SimHub para demonstrar o dashboard ESP32-S3"
    )
    parser.add_argument("--port",     type=str,   default=None,  help="Porta serial (ex: /dev/cu.usbmodem1101)")
    parser.add_argument("--duration", type=float, default=180.0, help="Duração da simulação em segundos (default: 180)")
    parser.add_argument("--hz",       type=float, default=20.0,  help="Taxa de envio de frames (default: 20)")
    parser.add_argument("--debug",    action="store_true",        help="Habilita debug do protocolo ARQ")
    parser.add_argument("--no-handshake", action="store_true",   help="Pula o handshake (útil se já conectado)")
    args = parser.parse_args()

    # Detecta porta
    port = args.port
    if not port:
        port = find_port()
    if not port:
        print("ERRO: Nenhuma porta USB CDC encontrada.")
        print("Opções:")
        print("  1. Verifique se o ESP32 está conectado via USB")
        print("  2. Use --port /dev/cu.usbmodem<XXXX> para especificar manualmente")
        print("\nPortas disponíveis:")
        for p in glob.glob("/dev/cu.*"):
            print(f"  {p}")
        sys.exit(1)

    print(f"\n{'═'*60}")
    print(f"  SimHub Simulator — ESP32-S3 Dashboard Demo")
    print(f"{'═'*60}")
    print(f"  Porta:    {port}")
    print(f"  Duração:  {args.duration:.0f}s ({args.duration/60:.1f} min)")
    print(f"  Taxa:     {args.hz:.0f} Hz")
    print(f"{'═'*60}\n")

    # Abre a porta serial
    try:
        ser = serial.Serial(
            port=port,
            baudrate=115200,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1.0,
            write_timeout=2.0,
        )
        # NÃO toggle DTR/RTS — evita resetar o ESP32
        ser.dtr = False
        ser.rts = False
        time.sleep(0.5)

        # Limpa buffer de entrada
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.3)

        print(f"  ✓ Porta {port} aberta @ 115200 baud")

    except serial.SerialException as e:
        print(f"ERRO ao abrir {port}: {e}")
        sys.exit(1)

    # Aguarda o ESP32 terminar o boot antes de qualquer comunicação
    if not args.no_handshake:
        wait_for_esp32_ready(ser, timeout=8.0)
    else:
        print()

    # Instancia ARQ
    arq = ARQProtocol(ser)
    arq._debug = args.debug

    # Handshake
    if not args.no_handshake:
        try:
            do_handshake(arq)
        except Exception as e:
            print(f"  ⚠ Handshake falhou: {e}")
            print("  Continuando com envio de telemetria...")
    else:
        print("  [Handshake skipped]\n")

    # Simulação de corrida
    sim     = RaceSimulator(total_duration=args.duration)
    interval = 1.0 / args.hz
    frame_count = 0
    last_status = time.time()

    print("  Iniciando loop de telemetria (Ctrl+C para parar)...")
    print(f"  {'Lap':>4} {'Time':>10} {'Spd':>5} {'G':>2} {'RPM%':>5} {'Fuel':>6}  {'Alert':^20}")
    print(f"  {'─'*4} {'─'*10} {'─'*5} {'─'*2} {'─'*5} {'─'*6}  {'─'*20}")

    try:
        while not sim.is_finished():
            t_start = time.time()

            frame = sim.generate_frame()
            fields = frame.rstrip(";").split(";")

            try:
                send_telemetry(arq, frame)
                frame_count += 1
            except Exception as e:
                print(f"\n  ⚠ Erro ao enviar frame: {e}")

            # Status a cada 1 segundo
            if time.time() - last_status >= 1.0:
                last_status = time.time()
                spd   = fields[0] if len(fields) > 0 else "?"
                gear  = fields[1] if len(fields) > 1 else "?"
                rpm   = fields[2] if len(fields) > 2 else "?"
                lap_t = fields[5] if len(fields) > 5 else "?"
                fuel  = fields[36] if len(fields) > 36 else "?"
                alert = fields[42] if len(fields) > 42 else "?"
                lap   = sim.lap_count + 1
                print(f"  {lap:>4}  {lap_t:>10}  {spd:>4}  {gear:>1}  {rpm:>4}%  ~{float(fuel):>4.1f}L  {alert:<20}")

            # Mantém a taxa de envio
            elapsed = time.time() - t_start
            sleep_t = max(0, interval - elapsed)
            if sleep_t > 0:
                time.sleep(sleep_t)

    except KeyboardInterrupt:
        print("\n\n  Simulação interrompida pelo usuário.")

    finally:
        elapsed_total = time.time() - sim.start_time
        print(f"\n{'═'*60}")
        print(f"  Simulação encerrada")
        print(f"  Frames enviados:  {frame_count}")
        print(f"  Tempo decorrido:  {elapsed_total:.1f}s")
        print(f"  Voltas completas: {sim.lap_count}")
        print(f"  Taxa efetiva:     {frame_count / elapsed_total:.1f} Hz")
        print(f"{'═'*60}\n")
        ser.close()


if __name__ == "__main__":
    main()
