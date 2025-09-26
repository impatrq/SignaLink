import queue
import sounddevice as sd
import sys
import json
import unicodedata
import re
from vosk import Model, KaldiRecognizer
from paho.mqtt import client as mqtt

# Colores
COLOR_CIAN = "\033[36m"# Cian
COLOR_VERDE = "\033[32m"# Verde
COLOR_MAGENTA = "\033[35m"# Magenta
COLOR_ERROR = "\033[31m"# Rojo
COLOR_RESET = "\033[0m"

# Configuración
SAMPLE_RATE = 32000
BLOCK_DURATION = 0.1
MODEL_PATH = "../vosk_model/vosk-model-small-es-0.42"

# Configuración Mqtt
MQTT_BROKER = "localhost" # O la IP de tu broker
MQTT_PORT = 1884
TOPIC_STATUS = "display/lcd"
# Bandera para controlar la ejecución
start_processing = False
username = 'franco'
password = '_fr4nco_'

# Inicializar modelo 
try:
    vosk_model_obj = Model(MODEL_PATH)
    recognizer = KaldiRecognizer(vosk_model_obj, SAMPLE_RATE)
except Exception as e:
    print(f"Error al cargar el modelo Vosk desde {MODEL_PATH}: {e}", file=sys.stderr)
    print("Asegúrate de que la carpeta 'vosk_model' esté en la ubicación correcta y el modelo es compatible con 16000 Hz.", file=sys.stderr)
    sys.exit(1)

audio_q = queue.Queue()

def audio_callback(indata, frames, time, status):
    if status:
        print(f"Error de audio en callback: {status}", file=sys.stderr)
    audio_q.put(bytes(indata))

def normalize_text(text):
    text = text.replace('ñ', 'n').replace('Ñ', 'N')
    normalized_text = unicodedata.normalize('NFD', text)

    cleaned_text_list = []
    
    for char in normalized_text:
        if unicodedata.category(char) != 'Mn':
            cleaned_text_list.append(char)
    
    cleaned_text = "".join(cleaned_text_list)
    
    cleaned_text = re.sub(r'[^\x20-\x7E]', '', cleaned_text)

    return cleaned_text
    
# --- Funciones de Callbacks de MQTT ---
def on_connect(client, userdata, flags, reasoncode, properties):
    # Función llamada cuando el cliente se conecta al broker.
    print(f"Conectado a MQTT con resultado de código {reasoncode}")
    client.subscribe(TOPIC_STATUS)

def on_message(client, userdata, msg):
    # Función llamada cuando se recibe un mensaje en un tópico suscrito.
    global start_processing # <-- ¡IMPORTANTE! Se usa para modificar la variable global
    print(f"Mensaje recibido en '{msg.topic}': '{msg.payload.decode()}'")

    if msg.topic == TOPIC_STATUS and msg.payload.decode() == "ON":
        print(f"{COLOR_VERDE}¡LCD listo! Iniciando reconocimiento de voz...{COLOR_RESET}")
        start_processing = True

# --- Lógica principal ---
print("Esperando la señal de MQTT...")
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(username, password)

# Callbacks
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, MQTT_PORT)

# Bucle en segundo plano para manejar los mensajes de MQTT
client.loop_start()

# Esperar hasta recibir la señal para iniciar el procesamiento de voz
while not start_processing:
    sd.sleep(1000) # Espera 1 segundo para no consumir mucho CPU

# Iniciar grabación y reconocimiento una vez que la bandera es True
try:
    with sd.RawInputStream(samplerate=SAMPLE_RATE, blocksize=int(SAMPLE_RATE * BLOCK_DURATION),
                            dtype='int16', channels=1, callback=audio_callback):
        print("¡Audio stream iniciado! Esperando voz...")
        while True:
            try:
                data = audio_q.get_nowait()
            except queue.Empty:
                sd.sleep(10)
                continue

            if recognizer.AcceptWaveform(data):
                result = json.loads(recognizer.Result())
                detected_text = result.get("text", "").strip()
                if detected_text:
                    cleaned_text = normalize_text(detected_text)
                    
                    print(f"Texto detectado (original): {COLOR_VERDE}{detected_text}{COLOR_RESET}")
                    print(f"Texto detectado (limpio para OLED): {COLOR_CIAN}{cleaned_text}{COLOR_RESET}")
                    
                    # PUBLICAR EL TEXTO LIMPIO EN EL TÓPICO DE MQTT
                    client.publish(TOPIC_STATUS, cleaned_text)

            else:
                partial = json.loads(recognizer.PartialResult())
                partial_text = partial.get("partial", "").strip()
                if partial_text:
                    print(f"Parcial: {COLOR_MAGENTA}{partial_text}{COLOR_RESET}", end="\r")

except KeyboardInterrupt:
    print("\n Finalizado")
except Exception as e:
    print(f"\n Ocurrió un error inesperado: {e}", file=sys.stderr)
finally:
    print("Cerrando stream de audio...", file=sys.stderr)
    client.loop_stop()
    client.disconnect()
    print("Recursos liberados.", file=sys.stderr)
