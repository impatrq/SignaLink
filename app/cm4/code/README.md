# Arquitectura de Software de SignaLink

Este directorio (`programs/`) contiene todos los scripts y configuraciones de software que se ejecutan en la Raspberry Pi Zero 2W para dar vida al proyecto SignaLink.

## 🗺️ Visión General

El sistema está diseñado con una arquitectura modular y desacoplada, utilizando el protocolo **MQTT** como eje central de comunicación. Cada componente (programa) funciona como un microservicio independiente que se comunica con los demás publicando o suscribiéndose a mensajes en un broker MQTT local.

Esto permite que cada parte del sistema se pueda desarrollar, probar y ejecutar de forma independiente.

---

## 📂 Estructura de Directorios

A continuación se describe el propósito de cada subdirectorio principal:

### 🎤 `mic/`

- **Propósito**: Captura y reconocimiento de voz.
- **Componente Clave**: `vosk_recognizer.py`.
- **Funcionamiento**: Utiliza el micrófono USB para escuchar comandos de voz. Cuando detecta una frase, la procesa y la publica en el tópico MQTT `display/lcd` para que sea mostrada en la pantalla. Espera una señal `ON` en el mismo tópico para saber que la pantalla está lista antes de empezar.

### 💬 `mqtt/`

- **Propósito**: Comunicación central y recolección de datos.
- **Componentes Clave**: `broker.py`, `broker_recolector.py`.
- **Funcionamiento**:
  - **Mosquitto**: Es el broker (servidor) MQTT que enruta todos los mensajes.
  - `broker.py`: Un script de depuración que escucha todos los tópicos e imprime los mensajes, útil para monitorear el sistema.
  - `broker_recolector.py`: Una herramienta esencial para grabar y etiquetar los gestos de la mano, generando los archivos `.csv` necesarios para entrenar el modelo de Machine Learning.

### 🔊 `voice_sound/`

- **Propósito**: Configuración de la salida de audio.
- **Componente Clave**: `README.md` con instrucciones.
- **Funcionamiento**: Este directorio contiene la documentación para configurar el DAC (Digital-to-Analog Converter) PCM5102. Este componente es el responsable de generar el audio que se reproduce a través de los altavoces.

### 🤖 `gesture_recognition/` (Directorio implícito)

- **Propósito**: Interpretar los gestos de la mano.
- **Funcionamiento**: Un script (no mostrado en el contexto, pero parte de la lógica) se suscribe al tópico MQTT `sensors/mpu_flex`, recibe los datos del guante en tiempo real, los procesa con el modelo de TFLite entrenado y determina qué gesto se está realizando. El resultado (la palabra o frase) se envía a través de `espeak-ng` al DAC para ser vocalizado.

### 🖥️ `lcd_driver/` (Directorio implícito)

- **Propósito**: Controlar la pantalla LCD.
- **Funcionamiento**: Un script se suscribe al tópico MQTT `display/lcd`. Cuando recibe un mensaje, lo muestra en la pantalla. Al iniciar, publica el mensaje `ON` para indicarle al resto del sistema (como al `vosk_recognizer.py`) que está listo para recibir texto.

---

## 🚀 Cómo Empezar

1.  **Instalación**:
    Asegúrate de haber ejecutado el script de instalación principal desde el directorio raíz (`rpizero2W/`).

    ```bash
    ./install.sh
    ```

2.  **Activar Entorno Virtual**:
    Antes de ejecutar cualquier script de Python, activa siempre el entorno virtual.

    ```bash
    source ../venv/bin/activate
    ```

3.  **Ejecutar Componentes**:
    Cada componente se ejecuta en su propia terminal. Por ejemplo, para iniciar el reconocedor de voz:
    ```bash
    cd mic/recognizer/
    python3 vosk_recognizer.py
    ```
