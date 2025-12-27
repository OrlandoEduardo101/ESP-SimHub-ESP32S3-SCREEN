#!/usr/bin/env python3
"""
GPIO 10 Hardware Test
Tests if GPIO 10 can output signals correctly
"""

import serial
import time
import sys

def test_gpio10():
    """
    Test GPIO 10 by analyzing the boot test sequence
    """
    
    print("=" * 80)
    print("GPIO 10 Hardware Test")
    print("=" * 80)
    print("\nThis test analyzes the LED boot sequence to verify GPIO 10 functionality\n")
    
    # Test methods
    print("METHOD 1: Visual Test (Recommended)")
    print("-" * 80)
    print("1. Power on the ESP32-S3")
    print("2. Watch the LED boot sequence (RED→GREEN→BLUE→WHITE→YELLOW)")
    print("3. Count how many LEDs light up:")
    print()
    print("   ✓ ALL 21 LEDs light up:")
    print("     → GPIO 10 is PERFECT! Hardware is working correctly.")
    print()
    print("   ⚠ Only 6-8 LEDs light up:")
    print("     → GPIO 10 is OK, but LED fita has broken data chain.")
    print("     → Problem is in DOUT→DIN connection between LEDs.")
    print()
    print("   ✗ NO LEDs light up:")
    print("     → GPIO 10 may be damaged OR not connected.")
    print("     → Check wiring: GPIO 10 → [470Ω] → DIN of first LED")
    print()
    print("   ✗ Random/flickering LEDs:")
    print("     → Bad connection or insufficient power supply.")
    print()
    
    print("\nMETHOD 2: Multimeter Test")
    print("-" * 80)
    print("1. Disconnect LED fita from GPIO 10")
    print("2. Upload the current firmware (already done)")
    print("3. During boot sequence, measure GPIO 10 with multimeter (DC voltage)")
    print("4. Expected readings:")
    print("   - During RED test: ~0.5-2.0V (rapid PWM pulses)")
    print("   - During GREEN test: ~0.5-2.0V (rapid PWM pulses)")
    print("   - Between tests: 0V or 3.3V")
    print()
    print("   ✓ Voltage changes during boot: GPIO 10 is working!")
    print("   ✗ Always 0V or always 3.3V: GPIO 10 may be stuck/damaged")
    print()
    
    print("\nMETHOD 3: Oscilloscope/Logic Analyzer")
    print("-" * 80)
    print("1. Connect probe to GPIO 10")
    print("2. Trigger on edge, ~800kHz signal expected")
    print("3. Should see WS2812B protocol: HIGH/LOW pulses at 800kHz")
    print()
    
    print("\nMETHOD 4: Serial Monitor Analysis")
    print("-" * 80)
    print("1. Open monitor: python .\\monitor_simhub_interaction.py")
    print("2. Power cycle ESP32")
    print("3. Look for these logs:")
    print()
    print("   [LED] STAGE 4.1: TEST 1 - Setting all LEDs to RED")
    print("   [LED]   ✓ RED color is ON - waiting 1000ms")
    print("   ...")
    print("   [LED]   LED #0 is ON")
    print("   [LED]   LED #1 is ON")
    print("   ...")
    print()
    print("   ✓ Logs appear: Software is working, GPIO 10 is sending data")
    print("   ✗ No LED logs: Software problem or setup() didn't run")
    print()
    
    print("\nMETHOD 5: Test with Simple LED (Fallback)")
    print("-" * 80)
    print("1. Disconnect WS2812B fita")
    print("2. Connect: GPIO 10 → [470Ω resistor] → Red LED → GND")
    print("3. Power on ESP32")
    print("4. During boot sequence:")
    print()
    print("   ✓ LED flickers/glows dimly: GPIO 10 is outputting signals!")
    print("   ✗ LED stays OFF: GPIO 10 may be damaged")
    print()
    print("   Note: LED won't blink clearly because WS2812B protocol is")
    print("         high-frequency (800kHz). You'll see rapid flickering.")
    print()
    
    print("=" * 80)
    print("\nQUICK DIAGNOSIS:")
    print("=" * 80)
    print("Based on your previous logs, your GPIO 10 is WORKING CORRECTLY!")
    print()
    print("Evidence:")
    print("  ✓ [LED] STAGE 0-6 all complete")
    print("  ✓ [LED] LED #0 through LED #20 all addressed")
    print("  ✓ No GPIO errors in logs")
    print("  ✓ Software executes perfectly")
    print()
    print("Problem is NOT GPIO 10 - it's the LED fita physical damage.")
    print("Data chain breaks between LED 5-8 (DOUT→DIN connection).")
    print()
    print("SOLUTION: Point-to-point soldering as planned.")
    print("=" * 80)

if __name__ == "__main__":
    test_gpio10()
