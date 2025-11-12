#!/usr/bin/python3
import re
import os
import time
import threading
import numpy as np
import paho.mqtt.client as mqtt
import tflite_runtime.interpreter as tflite
import simpleaudio as sa
import subprocess
from collections import deque
from loguru import logger
import pickle
import warnings

# -----------------------
# IGNORAR WARNINGS
# -----------------------
warnings.filterwarnings("ignore", message="The value of the smallest subnormal")
warnings.filterwarnings("ignore", category=UserWarning, module="sklearn")

# -----------------------
# CONFIGURACIÓN
# -----------------------
BROKER = 'localhost'
PORT = 1884
TOPIC_SENSOR = "sensors/mpu_flex"
USERNAME = 'franco'
PASSWORD = '_fr4nco_'

MODEL_PATH = "/home/signalink/SignaLink/app/cm4/code/mqtt_broker/tfmodel/signalink_model.tflite"
LABEL_PATH = "/home/signalink/SignaLink/app/cm4/code/mqtt_broker/tfmodel/label_encoder.pkl"
SCALER_PATH = "/home/signalink/SignaLink/app/cm4/code/mqtt_broker/tfmodel/scaler.pkl"
AUDIO_DIR = "/home/signalink/SignaLink/app/cm4/code/mqtt_broker/voice/words"

FEATURES = 7
BUFFER_SIZE = 30
EXPECTED_TOTAL = FEATURES * BUFFER_SIZE

PRED_BUFFER_LEN = 5
MIN_STABLE_COUNT = 3
COOLDOWN_SECONDS = 1.2

MOVEMENT_RANGE_THRESHOLD = 20.0

TEMPO = 0.6
GAIN_DB = -12

# -----------------------
# CONFIGURACIÓN BLUETOOTH
# ----------------------
JBL_LORENZO = "20:18:5B:75:84:51"
JBL_MAC = "90:F2:60:7D:27:9C"
JBL_SINK = f"bluez_sink.{JBL_MAC.replace(':', '_')}.a2dp_sink"
JBL_CARD = f"bluez_card.{JBL_MAC.replace(':', '_')}"
BLUETOOTH_CONNECT_TIMEOUT = 20 # Segundos para intentar la conexión


# -----------------------
# LOGGER
# -----------------------
logger.remove()
logger.add(lambda msg: print(msg, end=""), colorize=True,
           format="<cyan>{time:HH:mm:ss}</cyan> | <level>{level}</level> | <magenta>{message}</magenta>")

# -----------------------
# CARGAR MODELO / ENCODER / SCALER
# -----------------------
logger.info("Cargando modelo TFLite...")
interpreter = tflite.Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

model_input_shape = tuple(int(x) for x in input_details[0]['shape'])
model_expected = int(np.prod(model_input_shape))
logger.info(f"Modelo espera shape={model_input_shape} (total {model_expected} valores)")

logger.info("Cargando LabelEncoder...")
with open(LABEL_PATH, "rb") as f:
    label_encoder = pickle.load(f)

logger.info("Cargando Scaler...")
with open(SCALER_PATH, "rb") as f:
    scaler = pickle.load(f)

# -----------------------
# BUFFERS Y ESTADO
# -----------------------
data_buffer = deque(maxlen=BUFFER_SIZE)
prediction_buffer = deque(maxlen=PRED_BUFFER_LEN)
last_triggered = None
last_trigger_time = 0.0
audio_cache = {}
audio_lock = threading.Lock() 

# -----------------------
# CONEXIÓN BLUETOOTH Y AUDIO
# -----------------------

def connect_bluetooth_jbl():
    """Intenta conectar el altavoz JBL y configurar PulseAudio."""
    logger.info(f"Iniciando conexión Bluetooth a {JBL_MAC}...")

    # 1. Intentar la conexión con bluetoothctl
    try:
        # El comando 'bluetoothctl connect' puede tardar y no siempre sale con error si falla.
        # Es mejor usar un bucle simple o un timeout.
        logger.debug(f"Ejecutando: bluetoothctl connect {JBL_MAC}")

        # Ejecutar el comando de conexión
        connect_cmd = f'bluetoothctl connect {JBL_MAC}'

        # Correr el comando y limitar el tiempo de espera (timeout)
        result = subprocess.run(connect_cmd, shell=True, timeout=BLUETOOTH_CONNECT_TIMEOUT, 
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        if "Connection successful" not in result.stdout:
            logger.error(f"Fallo la conexión Bluetooth. Asegúrese de que el JBL esté encendido y en modo emparejamiento. Salida: {result.stdout.strip()}")
            return False

        logger.success("Conexión Bluetooth exitosa!")
        time.sleep(2) # Dar tiempo para que el sink aparezca en PulseAudio

        # 2. Configurar PulseAudio
        logger.info(f"Configurando audio PulseAudio ({JBL_SINK})...")

        # Establecer perfil A2DP (Alta Calidad)
        subprocess.run(["pactl", "set-card-profile", JBL_CARD, "a2dp_sink"], check=True)

        # Establecer el JBL como sink predeterminado
        subprocess.run(["pactl", "set-default-sink", JBL_SINK], check=True)
        
        logger.success("JBL configurado como salida de audio predeterminada.")
        return True

    except subprocess.TimeoutExpired:
        logger.error(f"La conexión Bluetooth excedió el tiempo de espera de {BLUETOOTH_CONNECT_TIMEOUT} segundos.")
        return False
    except subprocess.CalledProcessError as e:
        logger.error(f"Error al configurar PulseAudio: {e}. Verifique que el servicio esté corriendo.")
        return False
    except Exception as e:
        logger.error(f"Error inesperado en la conexión Bluetooth: {e}")
        return False


# -----------------------
# UTILIDADES
# -----------------------
number_re = re.compile(r"[-+]?\d*\.\d+|\d+")

def parse_mqtt_payload(payload: str):
    nums = number_re.findall(payload)
    if not nums:
        logger.warning("No se encontraron números en payload.")
        return None
    vals = [float(x) for x in nums]
    if len(vals) != FEATURES:
        logger.warning(f"Esperados {FEATURES} valores, recibidos {len(vals)} -> '{payload[:80]}'")
        return None
    return vals

def make_model_input_from_buffer(buff):
    arr = np.array(buff, dtype=np.float32)

    try:
        arr = scaler.transform(arr)
    except Exception as e:
        logger.error(f"Error aplicando scaler: {e}")

    arr = arr.flatten()
    if arr.size != model_expected:
        logger.debug(f"Flatten size {arr.size} != modelo espera {model_expected}. Ajustando (pad/trim).")
        if arr.size < model_expected:
            arr = np.pad(arr, (0, model_expected - arr.size), mode='constant', constant_values=0.0)
        else:
            arr = arr[:model_expected]
    return arr.reshape(model_input_shape)

def movement_range(buff):
    a = np.array(buff, dtype=np.float32)
    return float(np.max(a) - np.min(a))

def ensure_adjusted_audio(pred):
    key = f"{pred}_t{TEMPO}_g{abs(GAIN_DB)}"
    if key in audio_cache and os.path.exists(audio_cache[key]):
        return audio_cache[key]

    original = os.path.join(AUDIO_DIR, f"{pred}.wav")
    if not os.path.exists(original):
        raise FileNotFoundError(f"Archivo de audio no encontrado: {original}")

    adjusted = f"/tmp/signalink_{key}.wav"
    if not os.path.exists(adjusted):
        cmd = ["sox", original, adjusted, "tempo", str(TEMPO), "gain", str(GAIN_DB)]
        logger.info(f"Generando audio ajustado para '{pred}' -> {adjusted}")
        try:
            subprocess.run(cmd, check=True)
        except Exception as e:
            logger.error(f"Error ejecutando sox: {e}. Usando original.")
            adjusted = original
    audio_cache[key] = adjusted
    return adjusted

def _play_blocking(file_path, pred):
    global audio_lock
    try:
        if not audio_lock.acquire(blocking=False): 
           logger.warning(f"Dispositivo ocupado, no se puede reproducir ahora")
           return
        logger.debug(f"Reproduciendo {pred} desde {file_path}")
        
        # Se usa 'aplay' sin especificar la tarjeta para que use el sink predeterminado (JBL)
        result = subprocess.run(["aplay", file_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        if result.returncode != 0:
           logger.error(f"Error reproduciendo '{pred}': {result.stderr.strip()}")
        else:
           logger.debug(f"Reproducción de {pred} finalizada") 
    except Exception as e:
        logger.error(f"Error general en _play_blocking ({pred}): {e}")
    finally:
        if audio_lock.locked():
           audio_lock.release()
        time.sleep(0.1)

def play_audio(pred):
    # Lógica de Reposo: No reproducir si empieza con 'reposo'
    if pred.lower().startswith("reposo"):
        logger.debug(f"Prediccion '{pred}' -> no se reproduce audio (Es un estado de reposo)")
        return

    try:
        path = ensure_adjusted_audio(pred)
        logger.debug(f"Reproduciendo '{pred}'")
        threading.Thread(target=_play_blocking, args=(path, pred), daemon=True).start()
    except Exception as e:
        logger.error(f"Error en play_audio: {e}")

# -----------------------
# PROCESAMIENTO DE MENSAJES
# -----------------------
def process_mqtt_message(payload: str):
    vals = parse_mqtt_payload(payload)
    if vals is None:
        return None

    data_buffer.append(vals)

    if len(data_buffer) < BUFFER_SIZE:
        return None

    rng = movement_range(data_buffer)

    if rng < MOVEMENT_RANGE_THRESHOLD:
        return None

    try:
        arr = np.array(data_buffer, dtype=np.float32).flatten().reshape(1, -1)
        arr_scaled = scaler.transform(arr)
    except Exception as e:
        logger.error(f"Error aplicando scaler: {e}")
        return None

    model_input = arr_scaled.reshape(model_input_shape)
    return model_input

# -----------------------
# CALLBACK MQTT
# -----------------------
def on_message(client, userdata, msg):
    global last_triggered, last_trigger_time
    payload = msg.payload.decode(errors='ignore')
    model_input = process_mqtt_message(payload)
    if model_input is None:
        return

    try:
        interpreter.set_tensor(input_details[0]['index'], model_input)
        interpreter.invoke()
        output = interpreter.get_tensor(output_details[0]['index'])
        pred_idx = int(np.argmax(output))
        pred = label_encoder.inverse_transform([pred_idx])[0]
    except Exception as e:
        logger.error(f"Error en inferencia: {e}")
        return

    prediction_buffer.append(pred)
    last_n = list(prediction_buffer)[-MIN_STABLE_COUNT:]
    if len(last_n) >= MIN_STABLE_COUNT and all(x == pred for x in last_n):
        now = time.time()
        if pred != last_triggered and (now - last_trigger_time) > COOLDOWN_SECONDS:
            logger.success(f"Señal estable detectada: {pred}")
            play_audio(pred)
            last_triggered = pred
            last_trigger_time = now
            prediction_buffer.clear()
    else:
        logger.debug(f"Inestable: {list(prediction_buffer)}")

# -----------------------
# MAIN
# -----------------------
def main():
    # 1. Conexión Bluetooth al inicio
    if not connect_bluetooth_jbl():
        logger.warning("El script continuará, pero el audio no saldrá por el JBL.")

    # 2. Lógica MQTT/Modelo
    logger.info("Conectando al broker MQTT...")
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_message = on_message

    try:
        client.max_inflight_messages_set(100)
        client.max_queued_messages_set(1000)
        client.reconnect_delay_set(min_delay=1, max_delay=5)
    except Exception:
        pass

    client.connect(BROKER, PORT, keepalive=30)
    client.subscribe(TOPIC_SENSOR, qos=1)
    client.loop_start()

    logger.success(f"Suscrito a {TOPIC_SENSOR} en {BROKER}:{PORT}. Esperando mensajes...")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logger.info("Saliendo por teclado...")
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
