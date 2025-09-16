import queue
import sounddevice as sd
import sys
import json
import serial 
import unicodedata
import re
from vosk import Model, KaldiRecognizer

# Colores
COLOR_CIAN = "\033[36m"    # Cian
COLOR_VERDE = "\033[32m"  # Verde
COLOR_MAGENTA = "\033[35m"     # Magenta
COLOR_ERROR = "\033[31m"   # Rojo
COLOR_RESET = "\033[0m"

# Configuración
SAMPLE_RATE = 32000
BLOCK_DURATION = 0.1
MODEL_PATH = "../vosk_model/vosk-model-small-es-0.42"

# Configuración UART
SERIAL_PORT = '/dev/ttyAMA0'
BAUD_RATE = 115200

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

# Iniciar grabación
try:
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
except Exception as e:
    print(f"\n  Ocurrió un error inesperado: {e}", file=sys.stderr)
finally:
    print("Cerrando stream de audio...", file=sys.stderr)
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Puerto serial cerrado.", file=sys.stderr)
    print("Recursos liberados.", file=sys.stderr)
