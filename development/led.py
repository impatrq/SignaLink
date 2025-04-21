import RPi.GPIO as GPIO
import time

PIN = 3 # PIN en uso

try:
    GPIO.setmode(GPIO.BOARD) # Uso numeros fisicos de los pines -> GPIO.BCD (Uso pines GPIO)
    GPIO.setup(PIN, GPIO.OUT) # Configuro PIN como salida

    while True:
        GPIO.output(PIN, GPIO.HIGH) # Apago el LED
        time.sleep(1) # Espero 1 segundo
        GPIO.output(PIN, GPIO.LOW) # Enciendo el LED
        time.sleep(1) # Espero 1 segundo

except KeyboardInterrupt:
    print ("\n Finalizando el codigo.")

finally:
    GPIO.cleanup() # Restablezo el GPIO

