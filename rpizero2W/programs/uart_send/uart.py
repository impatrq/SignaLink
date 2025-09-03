#!/usr/bin/python3
import serial
PORT = "/dev/ttyAMA0"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=1)

print(f"Escuchando en {PORT} a {BAUD} baudios...\n")

try:
    while True:
        line = ser.readline()   # Lee hasta \n o hasta que termine el timeout
        if line:
            try:
                # Decodifica a UTF-8 (los datos que mandás son ASCII, así que funciona igual)
                text = line.decode("utf-8", errors="ignore").strip()
                print(text)
            except Exception as e:
                print(f"[Error de decode] Datos crudos: {line}")
except KeyboardInterrupt:
    print("\nSaliendo...") 
    ser.close()
