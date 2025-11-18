# **Funcionalidad de la comunicación MQTT**

La comunicación entre el **ESP32** (el guante) y la **Raspberry Pi** (el cerebro del sistema) se gestiona a través del protocolo **MQTT** (Message Queuing Telemetry Transport).

La Raspberry Pi actúa como el **broker MQTT**, el servidor central que recibe y distribuye todos los mensajes. El ESP32 funciona como un **cliente MQTT**, que se conecta a este broker para publicar datos y suscribirse para recibir comandos.

## **Tópicos y flujo de datos**

El proyecto utiliza dos tópicos MQTT principales para gestionar el flujo de información:

### 1. Guante a Raspberry Pi: Transmisión de Sensores

- **Tópico**: `sensors/mpu_flex`
- **Flujo**: `ESP32 -> Raspberry Pi`
- **Mensaje**: El ESP32 publica periódicamente (cada segundo) un mensaje que contiene los valores de los **5 sensores de flexión** (dedos) y los datos del giroscopio del **MPU6050** (orientación de la mano).
- **Descripción**: La Raspberry Pi se suscribe a este tópico para recibir en tiempo real las mediciones del guante. Estos datos son la entrada para el modelo de IA, que los interpreta para identificar una seña específica.

#### Flujo 1: Estado del Guante (ESP32 -> Raspberry Pi)

- **Mensaje**: Al conectarse, el ESP32 publica un mensaje `ON` con la bandera de **retención (retain)** activada.
- **Propósito**: Esto le permite a la Raspberry Pi saber que la pantalla OLED del guante está encendida y lista para recibir texto. La bandera de retención asegura que la RPi reciba el último estado conocido del LCD, incluso si se conecta después que el guante.

## **Ventajas del uso de MQTT**

- **Eficiencia energética**: Su protocolo ligero minimiza el uso de datos y potencia, lo cual es ideal para un dispositivo a batería como el guante.
- **Comunicación asíncrona**: El ESP32 no necesita esperar una respuesta de la Raspberry Pi. Simplemente publica los datos y el broker se encarga de entregarlos, liberando los recursos del microcontrolador.
- **Uso menor de Hardware**: La arquitectura de "publicar/suscribir" desacopla la Raspberry Pi del ESP32. Esto permite minimizar el uso de dos ESP32 y ahorrar uno de ellos (forma tradicional para el funcionamiento BLE).
