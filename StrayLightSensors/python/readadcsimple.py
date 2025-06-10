#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import time
import ADS1263
import RPi.GPIO as GPIO
import sys

REF = 5.08  # Adjust for your reference voltage

try:
    ADC = ADS1263.ADS1263()
    if ADC.ADS1263_init_ADC1('ADS1263_400SPS') == -1:
        sys.exit("ADC1 init failed")
    
    ADC.ADS1263_SetMode(0)  # 0 = single-ended

    channelList = [0, 1, 2, 3]  # Channels to read continuously

    while True:
        values = ADC.ADS1263_GetAll(channelList)
        floats = []

        for i in channelList:
            if values[i] >> 31 == 1:
                val = REF * 2 - values[i] * REF / 0x80000000
            else:
                val = values[i] * REF / 0x7fffffff
            floats.append(val)

        # Print CSV-style float values on a single line
        print(",".join(f"{v:.6f}" for v in floats))
        sys.stdout.flush()  # Ensure the line is sent immediately
        time.sleep(0.2)  # Optional: limit read rate to 5 Hz

except KeyboardInterrupt:
    print("Terminated by user", file=sys.stderr)
    ADC.ADS1263_Exit()
    GPIO.cleanup()
