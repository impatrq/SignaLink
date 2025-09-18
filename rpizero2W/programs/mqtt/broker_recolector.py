import paho.mqtt.client as mqtt
import csv
import time
import re
import sys
from pathlib import Path
import os
from datetime import datetime

# === Variables Globales ===
broker = 'localhost'
port = 1884
topic_sensors = "sensors/mpu_flex"
topic_lcd = "display/lcd"
username = 'franco'
password = '_fr4nco_'

# === Variables para la Recolección de Datos ===
GESTURE_NAME = "hola"
NUM_REPETITIONS = 20
RECORDING_DURATION = 3
data_buffer = []

# === FUNCIÓN PARA ANALIZAR LOS DATOS DE CADA SENSOR ===
def parse_data_line(line):
    try:
        # Busca la parte del mensaje que contiene los valores de los sensores
        if "sensor -> " in line:
            sensor_data_str = line.split('sensor -> ')[-1].strip()
            # La expresión regular busca cualquier secuencia de caracteres que no sean espacios en blanco.
            values = re.findall(r'(\S+)', sensor_data_str)
            
            # Convierte los valores a números (int o float)
            numeric_values = []
            for val in values:
                try:
                    numeric_values.append(int(val))
                except ValueError:
                    try:
                        numeric_values.append(float(val))
                    except ValueError:
                        continue
            return numeric_values
        return None
    except (ValueError, IndexError):
        return None

# === FUNCIÓN PARA INICIAR LA GRABACIÓN ===
def start_recording(gesture_name):
    global GESTURE_NAME, data_buffer
    
    GESTURE_NAME = gesture_name
    OUTPUT_DIR = Path("gestos") / GESTURE_NAME
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    print(f"\nPreparando para grabar el gesto: '{GESTURE_NAME}'")
    
    for i in range(1, NUM_REPETITIONS + 1):
        file_path = OUTPUT_DIR / f'gesture_{i:02d}.csv'
        print(f"\nPresiona ENTER para grabar la repetición {i}/{NUM_REPETITIONS} para: '{GESTURE_NAME}'.")
        input()
        
        # Limpia el buffer de datos antes de empezar a grabar
        data_buffer.clear()
        
        print(f"Grabando... Realiza el gesto. Duración: {RECORDING_DURATION}s")

        start_time = time.time()
        while (time.time() - start_time) < RECORDING_DURATION:
            # Esperamos para no saturar el CPU
            time.sleep(0.01)
                
        # En este punto, 'data_buffer' ya contiene los datos de la grabación
        if not data_buffer:
            print(f"Advertencia: No se recolectaron datos para la repetición {i}.")
            continue
                
        with open(file_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['mpu_x', 'mpu_y', 'mpu_z', 'indice', 'mayor', 'anular', 'gordo', 'meñique', 'label'])
                        
            for row_data in data_buffer:
                if len(row_data) >= 8:
                    row_data.append(GESTURE_NAME)
                    writer.writerow(row_data)
                else:
                    print(f"Advertencia: Se encontró una línea incompleta. Saltando: {row_data}")
                    
        print(f"Grabación de la repetición {i} completa. Guardado en '{file_path}'. Se grabaron {len(data_buffer)} filas.")

# === Callback cuando se conecta ===
def on_connect(client, userdata, flags, rc, properties):
    if rc == 0:
        print("\033[36m" + "Conexion al broker exitosa..." + "\033[0m")
        client.subscribe(topic_sensors)
        client.subscribe(topic_lcd)
    else:
        print("\033[31m" + "Error de conexión: " + str(rc) + "\033[0m")

# === Callback cuando llega un mensaje ===
def on_message(client, userdata, msg):
    data = msg.payload.decode('utf-8')
    if msg.topic == topic_sensors:
        # Solo procesa y guarda los datos cuando se está grabando
        if 'Grabando...' in globals() and time.time() - start_time < RECORDING_DURATION:
            sensor_values = parse_data_line(f"sensor -> {data}")
            if sensor_values:
                data_buffer.append(sensor_values)
                print(f"sensor -> {data}")
    elif msg.topic == topic_lcd:
        print(f"LCD <- {data}")

# === Código principal ===
try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    
    # Se ha modificado on_message para que se gestione la captura de datos temporalmente
    client.on_message = on_message
    
    client.connect(broker, port)
    client.loop_start()
    
    # Espera 2 segundos para que la conexión y el logging se establezcan
    time.sleep(2)

    start_recording("hola")

except KeyboardInterrupt:
    print("\n\033[31m" + "Saliendo..." + "\033[0m")
    client.loop_stop()
    client.disconnect()
    sys.exit(0)
except Exception as e:
    print(f"\n\033[31m" + "Error inesperado: " + str(e) + "\033[0m")
    sys.exit(1)      
