from vosk import Model, KaldiRecognizer
import pyaudio
import json

model = Model("../vosk_model/vosk-model-small-es-0.42")
rec = KaldiRecognizer(model,16000)

p = pyaudio.PyAudio()

stream = p.open(format=pyaudio.paInt16, channels=1, rate=16000,
                input=True, frames_per_buffer=8000)
stream.start_stream()

print("🎤 Hablá al micrófono... (Ctrl+C para salir)")
try:
    while True:
        data = stream.read(4000, exception_on_overflow=False)
        if rec.AcceptWaveform(data):
            result = rec.Result()
            text = json.loads(result)["text"]
            print("Texto detectado:", text)
except KeyboardInterrupt:
    print("Finalizado.")
