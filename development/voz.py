import speech_recognition as sr
import os

r = sr.Recognizer()

# Usa micrófono por defecto
with sr.Microphone() as source:
    print("Calibrando el micrófono... (quedate callado un toque)")
    r.adjust_for_ambient_noise(source, duration=3)
    print("Listo! Decime algo...")

    while True:
        try:
            print("Escuchando...")
            audio = r.listen(source)

            print("Procesando...")
            texto = r.recognize_google(audio, language="es-ES")
            print("Reconocido:", texto)

            # Acá podés comparar palabras clave
            if "encender" in texto.lower():
                print("Comando: ENCENDER")
                # Acá prendés el LED por ejemplo
            elif "apagar" in texto.lower():
                print("Comando: APAGAR")
                # Acá lo apagás
        except sr.UnknownValueError:
            print("No entendí, repetime")
        except sr.RequestError as e:
            print(f"Error con el servicio de Google: {e}")
