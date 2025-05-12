from machine import ADC, Pin
import time

# Micrófono
mic = ADC(Pin(0))
mic.atten(ADC.ATTN_11DB)
mic.width(ADC.WIDTH_12BIT)

# LED
led = Pin(2, Pin.OUT)

# Parámetros
umbral_voz = 300
promedio_ruido = 0
decaimiento = 0.01  # Qué tan rápido se adapta el promedio
delay = 0.01        # Tiempo entre lecturas

# Calibración inicial del ruido ambiente
print("Calibrando...")
for i in range(50):
    promedio_ruido += mic.read()
    time.sleep(0.01)
promedio_ruido /= 50
print("Promedio inicial de ruido:", int(promedio_ruido))

while True:
    valor = mic.read()
    diferencia = abs(valor - promedio_ruido)

    if diferencia < umbral_voz:
        promedio_ruido = (1 - decaimiento) * promedio_ruido + decaimiento * valor
        led.off()
        print("No hay voz")
    else:
        print("🎤 ¡Voz detectada!")
        led.on()

    time.sleep(delay)
