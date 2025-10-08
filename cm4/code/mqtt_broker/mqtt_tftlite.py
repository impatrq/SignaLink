import paho.mqtt.client as mqtt
import tflite_runtime.interpreter as tflite
import simpleaudio as sa
from loguru import logger
from tqdm import tqdm
import numpy as np
import subprocess
import pickle

# Loguru Config
logger.remove()
logger.add(lambda msg: print(msg, end=""), colorize=True, format="<cyan>{time:HH:mm:ss}</cyan>| <level>{level}</level> | <magenta>{message}</magenta>")

# MQTT
BROKER = 'localhost'
PORT = 1884
TOPIC_SENSOR = "sensors/mpu_flex"
USERNAME = 'franco'
PASSWORD = '_fr4nco_'

# Piper Model
PIPER_MODEL = "/home/signalink/SignaLink/cm4/code/mqtt_broker/voice/models/es_AR-daniela-high.onnx"

# Cargo Modelos
logger.info("Cargando modelo TFLite...")
interpreter = tflite.Interpreter(model_path="/home/signalink/SignaLink/cm4/code/mqtt_broker/tfmodel/signalink_model.tflite")
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Cargo LabelEncoder
logger.info("Cargando LabelEncoder")
with open("/home/signalink/SignaLink/cm4/code/mqtt_broker/tfmodel/label_encoder.pkl", "rb") as f:
    label_encoder = pickle.load(f)

def predict(data):
    input_data = np.array(data, dtype=np.float32).reshape(input_details[0]['shape'])
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    output_data = interpreter.get_tensor(output_details[0]['index'])
    predicted_class = np.argmax(output_data)
    return label_encoder.inverse_transform([predicted_class])[0]

def play_audio(label: str):
  try:
      audio_file = f"voice/words/{label}.wav"
      wave_obj = sa.WaveObject.from_wave_file(audio_file)
      play_obj = wave_obj.play()
      play_obj.wait_done()
  except Exception as e:
      print(f"Error al reproducir audio: {e}")

def on_message(client, userdata, msg):
  try:
      logger.info("Mensaje recibido desde MQTT")
      payload = np.frombuffer(msg.payload, dtype=np.float32)
      label = predict(payload)
      logger.success(f" Señal detectada: <green>{label}</green>")
      play_audio(label)
  except Exception as e:
      logger.error(f"Error procesando mensaje: {e}")

# Conectar al broker MQTT
def main():
    logger.info("Conectando al broker MQTT")
    client = mqtt.Client(mqtt_client.CallbackAPIVersion.VERSION2)
    client.username_pw_set(username, password)

    # Asignar callbacks
    client.on_connect = on_connect
    client.on_message = on_message

    # Conexión al broker
    client.connect(broker, port)

    # Suscripcion al topicos
    client.suscribe(TOPIC_SENSOR)

if __name__ == "__main__":
    main()
