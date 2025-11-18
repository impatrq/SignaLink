# Computer Module 4 (CM4) - Resumen compacto

Este directorio contiene la configuración, scripts y código desplegado en la
Raspberry Pi Compute Module 4 (CM4) usado por el proyecto SignaLink.
El objetivo en la CM4 fue recibir los datos de los sensores (ESP32), procesar
audio (ASR) y generar salida (TTS / LCD) y exponer/gestionar la mensajería
local vía MQTT.

Resumen rápido:

- Broker MQTT (Mosquitto) configurado en el puerto `1884` con autenticación.
- Servicio `signalink.service` (systemd) que arranca los procesos principales
  y prepara el AP (hostapd/dnsmasq) si es necesario.
- Captura y reconocimiento de voz con Vosk (`code/microphone/recognizer.py`).
- Módulos de prueba / registro MQTT (`code/mqtt_broker/mqtt_test`).
- Script de instalación `install.sh` que prepara sistema, venv y dependencias.
- Dependencias Python listadas en `requirements.txt`.

Estructura relevante

- `install.sh` : script principal de instalación y configuración (requisitos
  del sistema, copiar configs, crear `venv`, instalar pip requirements,
  habilitar servicios systemd).
- `config/` : contiene `signalink.service`, configuración de Mosquitto,
  y archivos para el Access Point (`wifi-ap/`).
- `code/` : código de la aplicación.
  - `microphone/` : captura de audio y reconocimiento con Vosk (`recognizer.py`).
  - `mqtt_broker/` : scripts de prueba, logging y prueba de modelos tflite.

Cómo instalar (resumen)

1. Conecta a la CM4 (Raspberry Pi OS) y posiciona en este directorio:

```bash
cd ~/SignaLink/cm4
sudo bash install.sh
```

El `install.sh` realiza:

- Actualización del sistema e instalación de paquetes del sistema (Python,
  `mosquitto`, `hostapd`, `dnsmasq`, bibliotecas de audio, etc.).
- Copia la configuración de Mosquitto y crea el archivo de contraseñas.
- Copia las configuraciones de `hostapd` y `dnsmasq` para el modo AP.
- Crea un entorno virtual `venv` y ejecuta `pip install -r requirements.txt`.
- Copia y habilita el servicio systemd `signalink.service`.

Notas de uso rápido

- Activar el entorno virtual (si quieres ejecutar scripts manualmente):

```bash
source venv/bin/activate
```

- Ejecutar manualmente el recognizer de voz (usa Vosk instalado y modelo):

```bash
python3 code/microphone/recognizer.py
```

- Ver logs del servicio systemd:

```bash
sudo journalctl -u signalink.service -f
```

- Comprobar estado del broker Mosquitto:

```bash
sudo systemctl status mosquitto
```

Credenciales y topics

- Usuario MQTT por defecto: `franco` (ver `config/mosquitto_conf/passwd`).
- Topics usados en código:
  - `sensors/mpu_flex` : datos de sensores (desde ESP32).
  - `display/lcd` : mensajes que la CM4 publica/escucha para mostrar en LCD
    o para activar el reconocedor.

Puntos importantes / recomendaciones

- `install.sh` asume `python3.11` y rutas basadas en `/home/signalink/SignaLink`.
  Adapta estas rutas si cambias el usuario o ubicación del repo.
- Asegúrate de tener espacio y permisos al copiar archivos a `/etc/` y al
  habilitar servicios systemd (ejecutar como `sudo`).
- Si el reconocimiento de voz falla, confirma:
  - El modelo Vosk está en `code/microphone/vosk_model/` y la variable
    `MODEL_PATH` en `recognizer.py` apunta al modelo correcto.
  - El dispositivo de audio está configurado y `arecord` / `aplay` funciona.

Dónde mirar el código

- Reconocimiento de voz y MQTT: `code/microphone/recognizer.py`.
- Broker / pruebas / logging: `code/mqtt_broker/mqtt_test/` (incluye
  `broker.py`, `grabador.py` y logs de gestos).
- Servicio systemd: `config/signalink.service` y script `config/services.sh`.

Si quieres que lo deje aún más resumido (una página rápida para visitantes)
o, por el contrario, que incluya ejemplos detallados de debugging, dímelo y lo
ajusto.

---

Última actualización: breve guía generada automáticamente por el asistente.

# Carpeta de desarrollo en Computer Module 4

En esta carpeta se llevo a cabo todo los necesairo para lograr un sistema
funcional en el cual
