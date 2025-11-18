#!/usr/bin/python3 
from paho.mqtt import client as mqtt_client
import time
import re
from pathlib import Path
import sys
import os
import re 

# Colores
COLOR_CIAN = "\033[36m"    # Cian
COLOR_VERDE = "\033[32m"  # Verde
COLOR_MAGENTA = "\033[35m" # Magenta
COLOR_ROJO = "\033[31m"   # Rojo
COLOR_RESET = "\033[0m"

# Path
GESTOS_DIR = Path("gestos")
current_dir = None

# Variables Globales
broker = 'localhost'
port = 1884
topic_sensors = "sensors/mpu_flex"
topic_lcd = "display/lcd"
username = 'franco'
password = '_fr4nco_'

# Variables para el control de la grabación
num_grabaciones = 20
duracion_grabacion = 3 # segundos
grabaciones_realizadas = 0
grabando = False

def setup_folder():
    global current_dir
    if not GESTOS_DIR.exists():
        GESTOS_DIR.mkdir(exist_ok=True)
        print(f"{COLOR_VERDE}Carpeta creada exitosamente{COLOR_RESET}")

    subfolders = [f.name for f in GESTOS_DIR.iterdir() if f.is_dir()]
    if subfolders:
        print("Gestos disponibles:")
        for i, f in enumerate(subfolders, 1):
            print(f"{i}. {f}")
    else:
        print("No hay gestos disponibles...")

    choice = input("Seleccione el número de un gesto o escriba un nombre nuevo: ")

    if choice.isdigit() and 1 <= int(choice) <= len(subfolders):
        current_dir = GESTOS_DIR / subfolders[int(choice) - 1]
    else:
        current_dir = GESTOS_DIR / choice
        current_dir.mkdir(exist_ok=True)
        print(f"Carpeta creada: {current_dir}")
    
    print(f"Guardado en: {current_dir}")
    print("Presiona ENTER para iniciar la grabación de 20 sesiones (3 segundos cada una)...")
    input() 

def parse_data_line(line):
    try:
        values = re.findall(r"-?\d+", line)
        values = [int(v) for v in values]
        return values
    except Exception as e:
        print(f"{COLOR_ROJO}Error en los datos: {e}{COLOR_RESET}")
        return None

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"{COLOR_VERDE}Conexion al broker exitosa...{COLOR_RESET}")
        client.subscribe(topic_sensors)
        client.subscribe(topic_lcd)
    else:
        print(f"{COLOR_ROJO}Error de conexión: {rc}{COLOR_RESET}")

def on_message(client, userdata, msg):
    global current_dir, grabando, grabaciones_realizadas
    if msg.topic == topic_sensors:
        if grabando:
            data = msg.payload.decode()
            values = parse_data_line(data)
            if values:
                log_file = current_dir / f"grabacion_{grabaciones_realizadas + 1}.csv"
                with open(log_file, "a") as f:
                    f.write(",".join(map(str, values)) + "\n")
    elif msg.topic == topic_lcd:
        print(f"LCD <- {msg.payload.decode()}")

client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
client.username_pw_set(username, password)
client.on_connect = on_connect
client.on_message = on_message

# Bucle principal de ejecución
try:
    setup_folder()
    client.connect(broker, port)
    client.loop_start()  

    # Bucle de grabación
    while grabaciones_realizadas < num_grabaciones:
        input(f"Presiona ENTER para iniciar la grabación {grabaciones_realizadas + 1} de {num_grabaciones}...")
        print(f"Iniciando grabación {grabaciones_realizadas + 1}...")
        grabando = True
        time.sleep(duracion_grabacion)
        grabando = False
        grabaciones_realizadas += 1
        print(f"Grabación {grabaciones_realizadas} finalizada.")
        time.sleep(1) # Pequeña pausa entre grabaciones

    print("¡20 grabaciones finalizadas! El programa se cerrará.")
    client.loop_stop()
    client.disconnect()
    sys.exit(0)
    
except KeyboardInterrupt:
    print(f"\n{COLOR_ROJO}Saliendo...{COLOR_RESET}")
    client.loop_stop()
    client.disconnect()
    sys.exit(0)
