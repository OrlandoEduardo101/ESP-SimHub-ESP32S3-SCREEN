#!/usr/bin/env python3
"""
Monitor serial output do ESP32-S3 ButtonBox WHEEL no macOS.

Mostra logs de debug do firmware:
- Inicialização USB/HID
- MCP23017 detectado (ou erro)
- PCA9685 detectado (ou erro)
- Botões pressionados
- Encoders girados
- Menu MFC
- Clutch/Hall sensors

Uso:
  python3 scripts/monitor_serial_mac.py
  python3 scripts/monitor_serial_mac.py --port /dev/cu.usbmodem*
  python3 scripts/monitor_serial_mac.py --list
"""

import argparse
import sys
import time
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Erro: pyserial não instalado.")
    print("Rode: pip3 install pyserial")
    print("Ou no venv do projeto: source .venv/bin/activate && pip install pyserial")
    sys.exit(1)


def list_ports():
    """Lista portas seriais disponíveis no macOS"""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("Nenhuma porta serial detectada.")
        return []

    print("\nPortas seriais disponíveis:")
    for i, port in enumerate(ports):
        print(f"  [{i}] {port.device}")
        print(f"      Descrição: {port.description}")
        print(f"      Hardware ID: {port.hwid}")
    return ports


def find_esp32_port():
    """Tenta encontrar automaticamente a porta do ESP32-S3"""
    ports = list(serial.tools.list_ports.comports())

    # Procura por portas com padrões típicos do ESP32 no Mac
    candidates = [
        p for p in ports
        if 'usbmodem' in p.device.lower()
        or 'usbserial' in p.device.lower()
        or 'esp' in p.description.lower()
        or 'cp210' in p.description.lower()
        or 'ch340' in p.description.lower()
    ]

    if candidates:
        return candidates[0].device

    # Se não achar, retorna a primeira porta disponível
    if ports:
        return ports[0].device

    return None


def monitor_serial(port, baud=115200):
    """Monitora continuamente a saída serial"""
    print(f"\n{'='*80}")
    print(f"ESP32-S3 ButtonBox WHEEL - Monitor Serial")
    print(f"{'='*80}")
    print(f"Porta: {port}")
    print(f"Baud: {baud}")
    print(f"Aguardando dados... (Ctrl+C para sair)")
    print(f"{'='*80}\n")

    try:
        ser = serial.Serial(
            port=port,
            baudrate=baud,
            timeout=1,
            rtscts=False,
            dsrdtr=False
        )

        buffer = ""
        line_count = 0
        start_time = time.time()

        while True:
            if ser.in_waiting > 0:
                try:
                    chunk = ser.read(ser.in_waiting)
                    text = chunk.decode('utf-8', errors='replace')
                    buffer += text

                    # Processa linhas completas
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()

                        if line:
                            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                            elapsed = time.time() - start_time

                            # Destaca mensagens importantes
                            if '[ERROR]' in line or 'ERROR' in line or 'FAIL' in line:
                                prefix = "❌"
                            elif 'NOT FOUND' in line or 'not found' in line:
                                prefix = "⚠️ "
                            elif '[I2C]' in line and 'OK' in line:
                                prefix = "✓ "
                            elif '[USB]' in line:
                                prefix = "🔌"
                            elif '[BOOT]' in line:
                                prefix = "🚀"
                            elif '[BTN' in line or 'BTN' in line:
                                prefix = "🔘"
                            elif '[AXIS' in line or 'AXIS' in line:
                                prefix = "🎮"
                            elif 'MFC' in line or 'MENU' in line:
                                prefix = "📋"
                            elif 'CLUTCH' in line or 'HALL' in line:
                                prefix = "🎚️ "
                            else:
                                prefix = "  "

                            print(f"[{timestamp}] {prefix} {line}")
                            line_count += 1

                except UnicodeDecodeError as e:
                    print(f"[DECODE ERROR] {e}")
                    buffer = ""
            else:
                time.sleep(0.01)

    except serial.SerialException as e:
        print(f"\n❌ Erro ao abrir porta {port}: {e}")
        print("\nDicas:")
        print("  - Verifique se a porta está correta (use --list)")
        print("  - Certifique-se que nenhum outro programa está usando a porta")
        print("  - Tente desconectar e reconectar o cabo USB")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n\n{'='*80}")
        print(f"Monitor finalizado.")
        print(f"Linhas recebidas: {line_count}")
        print(f"Tempo total: {time.time() - start_time:.1f}s")
        print(f"{'='*80}\n")
    finally:
        if 'ser' in locals():
            ser.close()


def main():
    parser = argparse.ArgumentParser(
        description="Monitor serial do ESP32-S3 ButtonBox WHEEL no macOS",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  python3 scripts/monitor_serial_mac.py
  python3 scripts/monitor_serial_mac.py --port /dev/cu.usbmodem14201
  python3 scripts/monitor_serial_mac.py --list
  python3 scripts/monitor_serial_mac.py --baud 115200
        """
    )

    parser.add_argument('--port', '-p', help='Porta serial (ex: /dev/cu.usbmodem14201)')
    parser.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate (padrão: 115200)')
    parser.add_argument('--list', action='store_true', help='Lista portas seriais disponíveis')

    args = parser.parse_args()

    if args.list:
        list_ports()
        return

    port = args.port
    if not port:
        print("Auto-detectando porta serial do ESP32-S3...")
        port = find_esp32_port()

        if not port:
            print("\n❌ Nenhuma porta serial encontrada.")
            print("\nRode com --list para ver portas disponíveis:")
            print("  python3 scripts/monitor_serial_mac.py --list")
            sys.exit(1)

        print(f"✓ Porta detectada: {port}")

    monitor_serial(port, args.baud)


if __name__ == '__main__':
    main()
