import serial
import csv
import time
import re
from pathlib import Path

# === CONFIGURACIÓN ===
SERIAL_PORT = '/dev/ttyAMA0'
BAUD_RATE = 115200

GESTURE_NAME = "hola" 
PARTICIPANT = 1         
NUM_REPETITIONS = 20
RECORDING_DURATION = 3
OUTPUT_DIR = Path("gestos") / GESTURE_NAME

# === FUNCIÓN PARA OBTENER DATOS DEL SERIAL ===
def get_serial_data():

    if ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line:
                return line
        except UnicodeDecodeError:
            return None
    return None

# === FUNCIÓN PARA ANALIZAR LOS DATOS DE CADA SENSOR ===
def parse_data_line(line):

    try:
        values = re.findall(r'(-?\d+)', line)
        numeric_values = [int(val) for val in values]
        return numeric_values
    except (ValueError, IndexError):
        return None

# === FUNCIÓN PARA HACER LAS GRABACIONES DE LOS GESTOS Y GUARDARLOS EN UN ARCHIVO CSV ===
def record_gesture(repetition_num):

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    file_path = OUTPUT_DIR / f"gesture_{repetition_num:02d}.csv"
    
    print(f"\nPresiona ENTER para grabar la repetición {repetition_num}/{NUM_REPETITIONS} para '{GESTURE_NAME}'...")
    input()
    print(f"Grabando... Realiza el gesto. Duración: {RECORDING_DURATION}s")

    with open(file_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['mpu_x', 'mpu_y', 'mpu_z', 'indice', 'mayor', 'anular', 'gordo', 'meñique', 'label'])
        
        start_time = time.time()
        while (time.time() - start_time) < RECORDING_DURATION:
            data_line = get_serial_data()
            if data_line:
                sensor_values = parse_data_line(data_line)
                if sensor_values and len(sensor_values) == 8:
                    sensor_values.append(GESTURE_NAME)
                    writer.writerow(sensor_values)
    
    print(f" Grabación completa. Guardado en '{file_path}'.")

# === INICIO DEL PROGRAMA ===
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Conectado a {SERIAL_PORT} a {BAUD_RATE} baudios.")

    # Bucle para grabar todas las repeticiones
    for i in range(1, NUM_REPETITIONS + 1):
        record_gesture(i)
    
except serial.SerialException as e:
    print(f"ERROR: No se pudo abrir el puerto serial {SERIAL_PORT}.")
    print(e)
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Puerto serial cerrado.")
