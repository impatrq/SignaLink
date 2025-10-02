import subprocess
import tempfile

# Ruta a tu modelo de Piper
PIPER_MODEL = "/home/signalink/SignaLink/rpizero2W/code/voice_sound/models/es_AR-daniela-high.onnx"

def texto_a_voz(texto):
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as temp_wav:
        temp_wav_path = temp_wav.name
    temp_low_path = temp_wav_path.replace(".wav", "_low.wav")

    cmd = ["piper", "--model", PIPER_MODEL, "--output_file", temp_wav_path, "--length_scale", "1.5"]
    subprocess.run(cmd, input=texto.encode("utf-8"), check=True)

    subprocess.run(["sox", temp_wav_path, temp_low_path, "gain", "-12"])

    subprocess.run(["aplay", temp_low_path])

texto_a_voz("Hola, soy Daniela, soy la voz oficial de SignaLink, este proyecto trata de la inclusion social de personas sordos mudas, a continuación haremos pruebas del abecedario en lengua de señas.")
