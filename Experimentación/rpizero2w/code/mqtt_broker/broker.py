#!/usr/bin/python3 
from paho.mqtt import client as mqtt_client
from datetime import datetime
import sys

# Colores
COLOR_INFO = "\033[36m"    # Cian
COLOR_SENSOR = "\033[32m"  # Verde
COLOR_LCD = "\033[35m"     # Magenta
COLOR_ERROR = "\033[31m"   # Rojo
COLOR_RESET = "\033[0m"


# Variables Globales
broker = 'localhost'
port = 1884
topic_sensors = "sensors/mpu_flex"
topic_lcd = "display/lcd"
username = 'franco'
password = '_fr4nco_'

log_file = "/home/fr4nco/Documents/SignaLink/rpizero2W/code/mqtt_broker/gestures.log"


# Callback cuando se conecta
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"{COLOR_SENSOR}Conexion al broker exitosa...{COLOR_RESET}")
        client.subscribe(topic_sensors)  
        client.subscribe(topic_lcd)
    else:
        print(f"{COLOR_ERROR}Error de conexión: {rc}{COLOR_RESET}")

# Callback cuando llega un mensaje
def on_message(client, userdata, msg):
    data = msg.payload.decode()
    
    if msg.topic == topic_sensors:
        with open(log_file, "a") as f:
            f.write(f"{datetime.now()} - {data}\n")
        print(f"Sensor -> {data}")
        
    elif msg.topic == topic_lcd:
        print(f"LCD <- {data}")

# Crear cliente
try:
    client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
    client.username_pw_set(username, password)

    # Asignar callbacks
    client.on_connect = on_connect
    client.on_message = on_message

    # Conexión al broker
    client.connect(broker, port)

    # Loop principal con manejo de Ctrl+C
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print(f"\n{COLOR_ERROR}Saliendo...{COLOR_RESET}")
        client.disconnect()
        sys.exit(0)
    
except KeyboardInterrupt:
    print(f"{COLOR_ERROR}Saliendo...{COLOR_RESET}")
