#!/usr/bin/env python3

import ADS1263

REF = 5.08  # Adjust to match your reference voltage

try:
    ADC = ADS1263.ADS1263()
    if ADC.ADS1263_init_ADC1('ADS1263_400SPS') == -1:
        exit(1)
    ADC.ADS1263_SetMode(0)  # Single-ended mode

    # Change this to however many channels you want
    channels = [0, 1, 2, 3]
    values = ADC.ADS1263_GetAll(channels)

    for i in channels:
        raw = values[i]
        if raw >> 31 == 1:
            voltage = REF * 2 - raw * REF / 0x80000000
        else:
            voltage = raw * REF / 0x7FFFFFFF
        print(f"{voltage:.6f}")

    ADC.ADS1263_Exit()

except Exception as e:
    print(f"Error: {e}")
    exit(1)
