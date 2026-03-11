#!/usr/bin/env python3
"""
Monitor e teste guiado do ESP-ButtonBox-WHEEL no macOS via USB HID.

Requisitos:
  pip3 install pygame

Uso:
  python3 scripts/monitor_hid_mac.py
  python3 scripts/monitor_hid_mac.py --guided
  python3 scripts/monitor_hid_mac.py --list
  python3 scripts/monitor_hid_mac.py --device-index 0 --guided
"""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass
from typing import Dict, List, Optional, Set, Tuple

try:
    import pygame
except ImportError:
    print("Erro: pygame não instalado. Rode: pip3 install pygame")
    sys.exit(1)


BUTTON_LABELS: Dict[int, str] = {
    1: "MFC SW",
    2: "ENC2 SW (BB)",
    3: "ENC3 SW (MAP)",
    4: "ENC4 SW (TC)",
    5: "ENC5 SW (ABS)",
    9: "Extra 1",
    10: "Extra 2",
    11: "Extra 3",
    12: "Extra 4",
    13: "RADIO",
    14: "FLASH",
    17: "Frontal 1",
    18: "Frontal 2",
    19: "Frontal 3",
    20: "Frontal 4",
    21: "Frontal 5",
    22: "Frontal 6",
    29: "5-way CENTER",
    30: "SHIFT (slot 30)",
    33: "Borboleta UP (+)",
    34: "Borboleta DOWN (-)",
    35: "Traseiro 1",
    36: "Traseiro 2",
    37: "Frontal extra (sem LED)",

    # Alguns hosts (pygame/macOS com este descritor) podem expor índices brutos
    # diferentes dos IDs HID esperados. Esses aliases ajudam no debug em bancada.
    58: "5-way CENTER / SHIFT (alias bruto)",
    60: "SHIFT / 5-way CENTER (alias bruto)",
}

# macOS/IOKit assigns pygame axis indices sorted by HID usage value:
# X=0x30→0, Y=0x31→1, Z=0x32→2, Rx=0x33→3, Ry=0x34→4, Rz=0x35→5,
# Slider=0x36→6, Dial=0x37→7, Vx=0x40→8, Vy=0x41→9
ENCODER_AXES: List[Tuple[int, str]] = [
    (0, "ENC2 (BB)"),
    (1, "ENC3 (MAP)"),
    (3, "ENC4 (TC)"),
    (4, "ENC5 (ABS)"),
    (6, "ENC6 (Lateral 1)"),
    (7, "ENC7 (Lateral 2)"),
    (8, "ENC8 (Lateral 3)"),
    (9, "ENC9 (Lateral 4)"),
]

HALL_AXES: List[Tuple[int, str]] = [
    (2, "Hall A (Clutch L / GPIO1)"),
    (5, "Hall B (Clutch R / GPIO2)"),
]

GUIDED_BUTTON_ORDER: List[int] = [
    1, 2, 3, 4, 5,
    9, 10, 11, 12, 13, 14,
    17, 18, 19, 20, 21, 22,
    33, 34, 35, 36, 37,
    29,
]


@dataclass
class RuntimeState:
    axes: Dict[int, float]
    buttons: Dict[int, bool]
    hat: Tuple[int, int]


class HidTester:
    def __init__(
        self,
        joystick: pygame.joystick.Joystick,
        deadzone: float = 0.08,
        axis_change: float = 0.03,
        show_axes: Optional[Set[int]] = None,
        hide_axes: Optional[Set[int]] = None,
        no_axis: bool = False,
        label_offset: int = 0,
        label_divisor: int = 1,
    ):
        self.joystick = joystick
        self.deadzone = deadzone
        self.axis_change = axis_change
        self.show_axes = show_axes
        self.hide_axes = hide_axes or set()
        self.no_axis = no_axis
        self.label_offset = label_offset
        self.label_divisor = max(1, label_divisor)
        self.state = RuntimeState(axes={}, buttons={}, hat=(0, 0))

    def _map_button_id(self, button_raw: int) -> int:
        return (button_raw + self.label_offset) // self.label_divisor

        for i in range(self.joystick.get_numaxes()):
            self.state.axes[i] = self.joystick.get_axis(i)
        for i in range(self.joystick.get_numbuttons()):
            self.state.buttons[i + 1] = bool(self.joystick.get_button(i))
        self.state.hat = self.joystick.get_hat(0) if self.joystick.get_numhats() > 0 else (0, 0)

    def poll_events(self, verbose: bool = True) -> Dict[str, List]:
        out: Dict[str, List] = {"axis": [], "down": [], "up": [], "hat": []}
        for event in pygame.event.get():
            if event.type == pygame.JOYBUTTONDOWN:
                button_raw = event.button + 1
                button_label_id = self._map_button_id(button_raw)
                self.state.buttons[button_raw] = True
                out["down"].append(button_raw)
                if verbose:
                    label = BUTTON_LABELS.get(button_label_id, "(sem rótulo)")
                    if self.label_offset == 0 and self.label_divisor == 1:
                        print(f"[BTN {button_raw:02d}] DOWN  {label}")
                    else:
                        print(f"[BTN {button_raw:02d}] DOWN  {label} (map->{button_label_id:02d})")
            elif event.type == pygame.JOYBUTTONUP:
                button_raw = event.button + 1
                button_label_id = self._map_button_id(button_raw)
                self.state.buttons[button_raw] = False
                out["up"].append(button_raw)
                if verbose:
                    label = BUTTON_LABELS.get(button_label_id, "(sem rótulo)")
                    if self.label_offset == 0 and self.label_divisor == 1:
                        print(f"[BTN {button_raw:02d}] UP    {label}")
                    else:
                        print(f"[BTN {button_raw:02d}] UP    {label} (map->{button_label_id:02d})")
            elif event.type == pygame.JOYAXISMOTION:
                axis_id = event.axis
                value = float(event.value)
                prev = self.state.axes.get(axis_id, 0.0)
                self.state.axes[axis_id] = value
                delta = abs(value - prev)
                if delta >= self.axis_change and (abs(value) >= self.deadzone or abs(prev) >= self.deadzone):
                    out["axis"].append((axis_id, value))
                    should_print_axis = not self.no_axis
                    if axis_id in self.hide_axes:
                        should_print_axis = False
                    if self.show_axes is not None and axis_id not in self.show_axes:
                        should_print_axis = False
                    if verbose and should_print_axis:
                        print(f"[AXIS {axis_id}] {value:+.3f}")
            elif event.type == pygame.JOYHATMOTION:
                hat_value = tuple(event.value)
                self.state.hat = hat_value
                out["hat"].append(hat_value)
                if verbose:
                    print(f"[HAT ] {hat_value}")
        return out

    def run_live_monitor(self) -> None:
        print("Monitor ativo. Ctrl+C para sair.")
        try:
            while True:
                self.poll_events(verbose=True)
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\nMonitor finalizado.")

    def _wait_for_button_press(self, button_id: int, timeout_s: float = 12.0) -> bool:
        end = time.time() + timeout_s
        while time.time() < end:
            events = self.poll_events(verbose=False)
            if button_id in events["down"]:
                return True
            time.sleep(0.01)
        return False

    def _capture_axis_window(self, axis_id: int, seconds: float = 4.0) -> Tuple[float, float, float, float]:
        start = time.time()
        first = self.joystick.get_axis(axis_id) if axis_id < self.joystick.get_numaxes() else 0.0
        last = first
        low = first
        high = first
        while time.time() - start < seconds:
            self.poll_events(verbose=False)
            if axis_id < self.joystick.get_numaxes():
                v = self.joystick.get_axis(axis_id)
                last = v
                low = min(low, v)
                high = max(high, v)
            time.sleep(0.01)
        return first, last, low, high

    def guided_test(self) -> None:
        print("\n=== TESTE GUIADO ===")
        print("Se algo não responder: revise solda, diodo, continuidade e mapeamento de slot.")
        if self.label_offset != 0 or self.label_divisor != 1:
            print(f"[INFO] Mapeamento ativo: slot = (BTN bruto {self.label_offset:+d}) / {self.label_divisor}")

        ok_buttons = 0
        fail_buttons = []

        print("\n[1/4] Botões (matriz)")
        for bid in GUIDED_BUTTON_ORDER:
            label = BUTTON_LABELS.get(bid, "(sem rótulo)")
            raw_expected = (bid * self.label_divisor) - self.label_offset
            input(f"- Pressione {label} [BTN {bid}] e tecle Enter para armar...")
            if self._wait_for_button_press(raw_expected, timeout_s=12.0):
                print(f"  ✓ BTN {bid} detectado")
                ok_buttons += 1
            else:
                print(f"  ✗ BTN {bid} NÃO detectado")
                fail_buttons.append((bid, label))

        print("\n[2/4] 5-way direções (HAT)")
        hat_steps = [
            ((0, 1), "UP"),
            ((0, -1), "DOWN"),
            ((-1, 0), "LEFT"),
            ((1, 0), "RIGHT"),
        ]
        ok_hat = 0
        fail_hat = []
        for expected, name in hat_steps:
            input(f"- Mova o 5-way para {name} e tecle Enter para armar...")
            found = False
            end = time.time() + 10.0
            while time.time() < end:
                events = self.poll_events(verbose=False)
                for hv in events["hat"]:
                    if tuple(hv) == expected:
                        found = True
                        break
                if found:
                    break
                time.sleep(0.01)
            if found:
                print(f"  ✓ HAT {name} detectado")
                ok_hat += 1
            else:
                print(f"  ✗ HAT {name} NÃO detectado")
                fail_hat.append(name)

        print("\n[3/4] Encoders (sentido) — gire no sentido horário")
        encoder_results = []
        for axis_id, label in ENCODER_AXES:
            if axis_id >= self.joystick.get_numaxes():
                encoder_results.append((label, "N/A", "eixo não existe no device"))
                print(f"  - {label}: N/A (axis {axis_id} não existe)")
                continue
            input(f"- {label}: gire HORÁRIO por ~2s e tecle Enter para iniciar janela...")
            first, last, low, high = self._capture_axis_window(axis_id, seconds=4.0)
            span = high - low
            net = last - first
            if span < 0.15:
                status = "FALHA"
                msg = f"sem movimento suficiente (span={span:.3f})"
            else:
                direction = "positivo" if net > 0 else "negativo"
                status = "OK"
                msg = f"movimento detectado, delta líquido {direction} (span={span:.3f}, net={net:+.3f})"
            encoder_results.append((label, status, msg))
            print(f"  {('✓' if status == 'OK' else '✗')} {label}: {msg}")

        print("\n[4/4] Halls")
        hall_results = []
        for axis_id, label in HALL_AXES:
            if axis_id >= self.joystick.get_numaxes():
                hall_results.append((label, "N/A", "eixo não existe no device"))
                print(f"  - {label}: N/A (axis {axis_id} não existe)")
                continue
            input(f"- {label}: mova a embreagem do mínimo ao máximo por ~2s e tecle Enter...")
            first, last, low, high = self._capture_axis_window(axis_id, seconds=4.0)
            span = high - low
            if span < 0.20:
                hall_results.append((label, "FALHA", f"variação baixa (span={span:.3f})"))
                print(f"  ✗ {label}: variação baixa (span={span:.3f})")
            else:
                hall_results.append((label, "OK", f"variação boa (span={span:.3f}, min={low:+.3f}, max={high:+.3f})"))
                print(f"  ✓ {label}: variação boa (span={span:.3f})")

        print("\n=== RESUMO ===")
        print(f"Botões: {ok_buttons}/{len(GUIDED_BUTTON_ORDER)} OK")
        print(f"HAT:    {ok_hat}/{len(hat_steps)} OK")

        enc_ok = sum(1 for _, s, _ in encoder_results if s == "OK")
        enc_all = sum(1 for axis_id, _ in ENCODER_AXES if axis_id < self.joystick.get_numaxes())
        print(f"Encoders: {enc_ok}/{enc_all} com movimento detectado")

        hall_ok = sum(1 for _, s, _ in hall_results if s == "OK")
        hall_all = sum(1 for axis_id, _ in HALL_AXES if axis_id < self.joystick.get_numaxes())
        print(f"Halls:    {hall_ok}/{hall_all} com variação adequada")

        if fail_buttons:
            print("\nBotões falhos:")
            for bid, label in fail_buttons:
                print(f"- BTN {bid}: {label}")

        if fail_hat:
            print("\nDireções HAT falhas:")
            for name in fail_hat:
                print(f"- {name}")

        wrong_enc = [r for r in encoder_results if r[1] != "OK"]
        if wrong_enc:
            print("\nEncoders com problema:")
            for label, _, msg in wrong_enc:
                print(f"- {label}: {msg}")

        wrong_hall = [r for r in hall_results if r[1] != "OK"]
        if wrong_hall:
            print("\nHalls com problema:")
            for label, _, msg in wrong_hall:
                print(f"- {label}: {msg}")


def init_pygame() -> None:
    pygame.init()
    pygame.joystick.init()


def list_devices() -> List[pygame.joystick.Joystick]:
    devices = []
    for i in range(pygame.joystick.get_count()):
        js = pygame.joystick.Joystick(i)
        js.init()
        devices.append(js)
    return devices


def pick_device(devices: List[pygame.joystick.Joystick], device_index: Optional[int]) -> Optional[pygame.joystick.Joystick]:
    if not devices:
        return None
    if device_index is not None:
        if 0 <= device_index < len(devices):
            return devices[device_index]
        return None
    for js in devices:
        if "ESP-ButtonBox-WHEEL" in js.get_name():
            return js
    return devices[0]


def parse_axis_list(raw: Optional[str]) -> Optional[Set[int]]:
    if raw is None:
        return None
    raw = raw.strip()
    if not raw:
        return set()
    out: Set[int] = set()
    for part in raw.split(','):
        part = part.strip()
        if not part:
            continue
        out.add(int(part))
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description="Monitor/teste HID do ESP-ButtonBox-WHEEL no macOS")
    parser.add_argument("--list", action="store_true", help="Lista dispositivos de joystick/gamepad encontrados")
    parser.add_argument("--device-index", type=int, default=None, help="Índice do dispositivo a usar")
    parser.add_argument("--guided", action="store_true", help="Executa teste guiado")
    parser.add_argument("--deadzone", type=float, default=0.08, help="Deadzone para eixos")
    parser.add_argument("--axis-change", type=float, default=0.03, help="Variação mínima para log de eixo")
    parser.add_argument("--no-axis", action="store_true", help="Oculta logs de eixos (mantém botões e HAT)")
    parser.add_argument("--hide-axis", type=str, default=None, help="Lista de eixos para ocultar (ex: 2,3)")
    parser.add_argument("--show-axis", type=str, default=None, help="Mostra apenas estes eixos (ex: 0,1,4)")
    parser.add_argument("--label-offset", type=int, default=0,
                        help="Offset aplicado antes do mapeamento de rótulo")
    parser.add_argument("--label-divisor", type=int, default=1,
                        help="Divisor do ID bruto para obter slot (ex: 2 quando host mostra 2,4,6...)")
    args = parser.parse_args()

    try:
        hide_axes = parse_axis_list(args.hide_axis)
        show_axes = parse_axis_list(args.show_axis)
    except ValueError:
        print("Erro: --hide-axis/--show-axis deve conter inteiros separados por vírgula (ex: 2,3)")
        sys.exit(1)

    init_pygame()
    devices = list_devices()

    if args.list:
        if not devices:
            print("Nenhum gamepad/joystick detectado via HID.")
            return
        print("Dispositivos detectados:")
        for i, js in enumerate(devices):
            print(f"[{i}] {js.get_name()} | axes={js.get_numaxes()} buttons={js.get_numbuttons()} hats={js.get_numhats()}")
        return

    joystick = pick_device(devices, args.device_index)
    if joystick is None:
        print("Nenhum dispositivo encontrado.")
        print("Conecte o ESP por USB e rode novamente, ou use: --list")
        sys.exit(1)

    print(f"Usando device: {joystick.get_name()}")
    print(f"Axes={joystick.get_numaxes()} Buttons={joystick.get_numbuttons()} Hats={joystick.get_numhats()}")
    label_offset = args.label_offset
    label_divisor = max(1, args.label_divisor)
    if joystick.get_numbuttons() > 200:
        print("[INFO] Mapeamento bruto detectado: BTN 58/60 podem representar SHIFT e 5-way CENTER.")
        if args.label_divisor == 1:
            label_divisor = 2
            if label_offset == -1:
                label_offset = 0
            print(f"[INFO] Auto-ajuste aplicado: slot = (BTN bruto {label_offset:+d}) / 2")

    tester = HidTester(
        joystick,
        deadzone=args.deadzone,
        axis_change=args.axis_change,
        show_axes=show_axes,
        hide_axes=hide_axes,
        no_axis=args.no_axis,
        label_offset=label_offset,
        label_divisor=label_divisor,
    )

    if args.guided:
        tester.guided_test()
    else:
        tester.run_live_monitor()


if __name__ == "__main__":
    main()
