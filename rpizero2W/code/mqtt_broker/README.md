# MQTT en SignaLink

Este directorio contiene los scripts relacionados con la comunicación MQTT (Message Queuing Telemetry Transport) dentro del proyecto SignaLink. MQTT es un protocolo de mensajería ligero, ideal para la comunicación entre los diferentes componentes del sistema que corren en la Raspberry Pi Zero 2W.

## 📜 Descripción

Se utiliza un **broker MQTT** local (Mosquitto) como un servidor central de mensajería. Los distintos programas (reconocimiento de voz, recolección de datos de sensores, control de la pantalla LCD) se conectan a este broker para enviar y recibir mensajes de forma desacoplada.

Esto permite que cada componente funcione de manera independiente. Por ejemplo, el script de los sensores no necesita saber nada sobre el script de la pantalla; simplemente publica los datos en un "tópico" y el que esté interesado, se suscribe a él.

## ⚙️ Configuración del Broker

La configuración está definida en los scripts de Python y es consistente en todo el proyecto.

- **Broker/Host**: `localhost` (el broker corre en la misma Raspberry Pi)
- **Puerto**: `1884`
- **Usuario**: `franco`
- **Contraseña**: `_fr4nco_`

### Tópicos (Topics)

Los tópicos son los "canales" a través de los cuales se envían los mensajes:

- `sensors/mpu_flex`:

  - **Publica**: El microcontrolador (ESP32) a través de un puente Serial-a-MQTT.
  - **Suscribe**: `broker_recolector.py` para guardar los datos de entrenamiento.
  - **Mensaje**: Una cadena de texto con los valores de los sensores, por ejemplo: `16244 1448 -3244 1988 2061 2095 2100 2125`.

- `display/lcd`:
  - **Publica**: `vosk_recognizer.py` para enviar el texto reconocido por voz.
  - **Suscribe**: El script que controla la pantalla LCD para mostrar el texto.
  - **Mensaje**: El texto a mostrar en la pantalla, o comandos especiales como `ON`.

## 🐍 Scripts de Python

### `broker.py`

Este es un script de **monitoreo y depuración**. Se suscribe a todos los tópicos (`sensors/mpu_flex` y `display/lcd`) e imprime en la consola cualquier mensaje que recibe. Además, guarda los datos del sensor en un archivo de log (`gestures.log`).

**Uso**:

Es útil para verificar que los mensajes se están enviando y recibiendo correctamente en el broker.

```bash
# Activa el entorno virtual
source ../../venv/bin/activate

# Ejecuta el script
python3 broker.py
```

### `broker_recolector.py`

Este script es una herramienta para **crear el dataset de gestos**. Su función es escuchar los datos del tópico `sensors/mpu_flex` y guardarlos en archivos `.csv` de manera estructurada.

**Funcionamiento**:

1.  Define un gesto a grabar (ej: `GESTURE_NAME = "hola"`).
2.  Pide al usuario que presione ENTER para iniciar la grabación de cada repetición.
3.  Durante `RECORDING_DURATION` segundos, almacena todos los datos recibidos del sensor.
4.  Guarda los datos recolectados en un archivo CSV, añadiendo una etiqueta (`label`) con el nombre del gesto.
5.  Repite el proceso `NUM_REPETITIONS` veces.

**Uso**:

Para generar los datos necesarios para entrenar el modelo de Machine Learning.

```bash
# Activa el entorno virtual
source ../../venv/bin/activate

# Modifica GESTURE_NAME en el script y ejecuta
python3 broker_recolector.py
```

## 🚀 Uso General

1.  **Instalar Mosquitto**: El broker se instala con el script principal `install.sh`.

```bash
sudo apt install mosquitto mosquitto-clients
```

2.  **Iniciar el Broker**: El servicio de Mosquitto normalmente se inicia automáticamente. Para verificarlo o iniciarlo manualmente:

```bash
# Verificar estado
sudo systemctl status mosquitto

# Iniciar servicio
sudo systemctl start mosquitto
```
