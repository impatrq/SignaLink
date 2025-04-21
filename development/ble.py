#!/usr/bin/python3

from bluetooth import *

# Dirección MAC del ESP32-C3
esp32_mac = "E8:06:90:65:C6:38"  # Reemplázalo con la MAC del ESP32-C3

# Conectar al ESP32-C3
sock = BluetoothSocket(RFCOMM)
sock.connect((esp32_mac, 1))

print("Conexión establecida con el ESP32-C3")

try:
    while True:
        data = sock.recv(1024).decode("utf-8")
        print("Dato recibido:", data)
        # Aquí puedes controlar tu LED basado en los datos recibidos
finally:
    sock.close()
    print("Conexión cerrada")
