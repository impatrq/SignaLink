import paho.mqtt.client as mqtt
import csv
import time
import re
import sys
from pathlib import Path
import subprocess

# === Variables Globales ===
broker = 'localhost'
port = 1884
topic_sensors = "sensors/mpu_flex"
topic_lcd = "display/lcd"
username = 'fr4nco'
password = '_fr4nco_'

# === Variables para la Recolección de Datos ===
GESTURE_NAME = "hola"
NUM_REPETITIONS = 20
RECORDING_DURATION = 3
LOG_FILE = Path('gestures.log')

# === FUNCIÓN PARA ANALIZAR LOS DATOS DE CADA SENSOR ===
def parse_data_line(line):
    try:
        sensor_data_str = line.split('sensor -> ')[-1]
        values = re.findall(r'(-?\d+)', sensor_data_str)
        numeric_values = [int(val) for val in values]
        return numeric_values
    except (ValueError, IndexError):
        return None

# === FUNCIÓN PARA LEER DATOS DEL ARCHIVO DE REGISTRO ===
def tail_file(file_path):
    proc = subprocess.Popen(['tail', '-f', str(file_path)], stdout=subprocess.PIPE, text=True)
    return proc.stdout

# === FUNCIÓN PARA INICIAR LA GRABACIÓN ===
def start_recording(gesture_name):
    global GESTURE_NAME

    GESTURE_NAME = gesture_name
    OUTPUT_DIR = Path("gestos") / GESTURE_NAME
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    print(f"\nPreparando para grabar el gesto: '{GESTURE_NAME}'")
    log_stream = tail_file(LOG_FILE)
    
    for i in range(1, NUM_REPETITIONS + 1):
        file_path = OUTPUT_DIR / f'gesture_{i:02d}.csv'
        print(f"\nPresiona ENTER para grabar la repetición {i}/{NUM_REPETITIONS} para: '{GESTURE_NAME}'.")
        input()
        
        print(f"Grabando... Realiza el gesto. Duración: {RECORDING_DURATION}s")

        with open(file_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['mpu_x', 'mpu_y', 'mpu_z', 'indice', 'mayor', 'anular', 'gordo', 'meñique', 'label'])
                    
                start_time = time.time()
                for line in log_stream:
                    if (time.time() - start_time) > RECORDING_DURATION:
                        break
                    
                    sensor_values = parse_data_line(line)
                    if sensor_values and len(sensor_values) == 8:
                        sensor_values.append(GESTURE_NAME)
                        writer.writerow(sensor_values)
            
            print(f"Grabación de la repetición {i} completa. Guardado en '{file_path}'.")

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
        print(f"sensor -> {data}")
    elif msg.topic == topic_lcd:
        print(f"lcd <- {data}")

# === Código principal ===
try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(broker, port)

    client.loop_start()
    start_recording("hola") 

except KeyboardInterrupt:
    print("\n\033[31m" + "Saliendo..." + "\033[0m")
    client.loop_stop()
    client.disconnect()
    sys.exit(0)
except Exception as e:
    print(f"\n\033[31m" + "Error inesperado: " + str(e) + "\033[0m")
    sys.exit(1)
