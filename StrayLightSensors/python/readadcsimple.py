#!/usr/bin/env python3
# readadcsimple.py

import ADS1263
import RPi.GPIO as GPIO

REF = 5.08  # Modify according to your actual reference voltage

try:
    ADC = ADS1263.ADS1263()
    if (ADC.ADS1263_init_ADC1('ADS1263_400SPS') == -1):
        exit(1)

    ADC.ADS1263_SetMode(0)  # Single-ended mode
    value = ADC.ADS1263_GetChannalValue(0)  # Read channel 0

    # Convert to voltage
    if (value >> 31 == 1):
        voltage = REF * 2 - value * REF / 0x80000000
    else:
        voltage = value * REF / 0x7FFFFFFF

    print(f"{voltage:.6f}")  # Output clean float value

    ADC.ADS1263_Exit()

except Exception as e:
    print(f"Error: {e}")
    exit(1)
