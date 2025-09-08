import queue
import sounddevice as sd
import sys
import json
import serial 
import unicodedata
import re
from vosk import Model, KaldiRecognizer

# === Configuración ===
SAMPLE_RATE = 32000
BLOCK_DURATION = 0.1
MODEL_PATH = "../vosk_model/vosk-model-small-es-0.42"

# === Configuración UART ===
SERIAL_PORT = '/dev/ttyAMA0'
BAUD_RATE = 115200

# === Inicializar modelo ===
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

# === Función para enviar texto por UART ===
def send_text_via_uart(text, ser):
    try:
        ser.write((text + '\n').encode('ascii')) 
        # Prefijo normal, texto enviado en cian negrita
        print(f"UART: Enviado \033[1;36m{text}\033[0m")
    except UnicodeEncodeError as e:
        print(f"Error Unicode al enviar por UART: {e}. El carácter no ASCII es: '{text.encode('unicode_escape').decode('ascii')}'", file=sys.stderr)
        print("Esto significa que normalize_text no eliminó algún carácter no ASCII.", file=sys.stderr)
    except Exception as e:
        print(f"Error al enviar por UART: {e}", file=sys.stderr)

# === Iniciar grabación ===
try:
    # Inicializar la comunicación serial
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"UART: Conectado a {SERIAL_PORT} a {BAUD_RATE} baudios.")

    with sd.RawInputStream(samplerate=SAMPLE_RATE, blocksize=int(SAMPLE_RATE * BLOCK_DURATION),
                            dtype='int16', channels=1, callback=audio_callback):
        print("Esperando voz...")

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
                    
                    print(f"Texto detectado (original): \033[1;32m{detected_text}\033[0m")
                    print(f"Texto detectado (limpio para OLED): \033[1;34m{cleaned_text}\033[0m")
                    send_text_via_uart(cleaned_text, ser)
            else:
                partial = json.loads(recognizer.PartialResult())
                partial_text = partial.get("partial", "").strip()
                if partial_text:
                    print(f"Parcial: \033[1;33m{partial_text}\033[0m", end="\r")

except KeyboardInterrupt:
    print("\n Finalizado")
except serial.SerialException as e:
    print(f"\n Error de puerto serial: {e}", file=sys.stderr)
    print("Asegúrate de que el puerto serial no esté en uso y los permisos sean correctos.", file=sys.stderr)
except Exception as e:
    print(f"\n  Ocurrió un error inesperado: {e}", file=sys.stderr)
finally:
    print("Cerrando stream de audio...", file=sys.stderr)
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Puerto serial cerrado.", file=sys.stderr)
    print("Recursos liberados.", file=sys.stderr)
