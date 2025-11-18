# Micrófono Corbatero USB (Modelo C11)

Este documento describe la configuración del micrófono corbatero USB utilizado en el proyecto SignaLink para la captura de audio.

## 🎙️ Descripción

El C11 es un micrófono de solapa (o corbatero) de tipo condensador con conexión USB. Se ha elegido para este proyecto por su capacidad para capturar la voz de manera clara y directa, minimizando el ruido de fondo. Esto es crucial para que el software de reconocimiento de voz (`vosk_recognizer.py`) funcione con la mayor precisión posible.

Al ser "Plug and Play", su integración con la Raspberry Pi es relativamente sencilla.

### Características Principales

- **Tipo**: Condensador Electret.
- **Patrón Polar**: Omnidireccional (captura sonido desde todas las direcciones).
- **Conexión**: USB 2.0.
- **Compatibilidad**: Raspberry Pi OS, Linux, Windows, macOS.
- **Ventaja**: Diseño compacto y manos libres, ideal para capturar la voz del usuario sin interferencias.

---

## 🔌 Conexión a la Raspberry Pi

1.  Conecta el micrófono a uno de los puertos USB disponibles en la Raspberry Pi Zero 2W.
2.  No se requieren drivers adicionales, ya que utiliza el controlador estándar `UAC` (USB Audio Class).

---

## 🧪 Prueba de Grabación

Puedes probar que todo funciona correctamente:

1.  **Graba 5 segundos de audio** desde el micrófono y guárdalo en un archivo:
    ```bash
    arecord -d 5 -f S16_LE -r 16000 test_mic.wav
    ```
2.  **Reproduce el archivo** a través del DAC. Deberías escuchar tu grabación por los altavoces.
    ```bash
    aplay test_mic.wav
    ```

Si la grabación y reproducción funcionan, ¡el micrófono está listo para ser usado por `vosk_recognizer.py`!
