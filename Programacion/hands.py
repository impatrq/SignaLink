import time
import board
import busio
from adafruit_ads1x15.analog_in import AnalogIn
from adafruit_ads1x15.ads1115 import ADS1115

# Configuración del bus I2C
i2c = busio.I2C(board.SCL, board.SDA)

# Inicialización del ADC
ads = ADS1115(i2c)

# Canales analógicos (puedes usar ADS.P0, ADS.P1, etc.)
canal_flex = AnalogIn(ads, ADS1115.P0)

while True:
    # Lectura del valor del sensor flex
    valor_flex = canal_flex.value
    voltaje_flex = canal_flex.voltage

    print(f"Valor: {valor_flex}, Voltaje: {voltaje_flex:.2f} V")
    time.sleep(0.5)