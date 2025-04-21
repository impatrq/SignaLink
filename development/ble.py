import time
import RPi.GPIO as GPIO
from bluepy.btle import Peripheral, UUID, DefaultDelegate

# Configuracion del pin del LED
LED_PIN = 17
GPIO.setmode(GPIO.BCM)
GPIO.setup(LED_PIN, GPIO.OUT)

# Delegado BLE para manejar notificaciones
class BLEDelegate(DefaultDelegate):
    def __init__(self):
        super().__init__()

    def handleNotification(self, cHandle, data):
        print(f"Notificación recibida: {data}")
        if data == b'ON':
            print("LED Encendido")
            GPIO.output(LED_PIN, GPIO.HIGH)
        elif data == b'OFF':
            print("LED Apagado")
            GPIO.output(LED_PIN, GPIO.LOW)

# Funcion para conectarse a un dispositivo BLE y escuchar
def start_ble_client(address):
    peripheral = Peripheral(address)
    peripheral.setDelegate(BLEDelegate())

    print("Conectado al periferico BLE. Esperando datos...")

    try:
        while True:
            if peripheral.waitForNotifications(1.0):
                continue
            print("Esperando notificaciones...")
            time.sleep(1)
    finally:
        peripheral.disconnect()

# Función principal
if __name__ == '__main__':
    try:
        ble_address = "E8:06:90:65:C6:38"
        start_ble_client(ble_address)
    except KeyboardInterrupt:
        print("Interrumpido por el usuario")
    finally:
        GPIO.cleanup()
