#!/usr/bin/env python3
import pigpio, numpy as np, time

PIN = 12
FS = 16000          # audio sample rate
F_PWM = 96000      # PWM carrier

pi = pigpio.pi()
if not pi.connected:
    raise SystemExit("pigpiod no conectado")

# cargar samples en [-1,1]
samples = np.loadtxt("audio_points.txt", dtype=float)
mx = max(abs(samples.min()), abs(samples.max()))
s = samples / mx

# mapear a duty 0..1e6
duties = ((s + 1.0) * 500_000).astype(int)

# iniciar PWM hardware a 0%
pi.hardware_PWM(PIN, F_PWM, 0)

Ts = 1.0 / FS
try:
    for d in duties:
        pi.hardware_PWM(PIN, F_PWM, int(d))
        # ⚠️ este sleep NO alcanza para 16kHz estables!
        time.sleep(Ts)
finally:
    pi.hardware_PWM(PIN, 0, 0)
    pi.stop()
