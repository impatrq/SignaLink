# **Funcionalidad de la comunicación MQTT**

---

La comunicación entre el **ESP32** (el guante con sensores) y la **Raspberry Pi Zero 2W** (el cerebro del sistema) se gestiona a través del protocolo **MQTT** (Message Queuing Telemetry Transport). MQTT es un protocolo de mensajería ligero, ideal para una comunicación eficiente y escalable.

La Raspberry Pi actúa como el **broker MQTT**, el servidor central que recibe, filtra y distribuye todos los mensajes. El ESP32 funciona como un **cliente MQTT**, que se conecta a este broker para publicar y suscribirse a tópicos específicos.

---

## **Tópicos y flujo de datos**

El proyecto utiliza dos flujos de comunicación principales, cada uno con sus propios tópicos MQTT:

1.  **Guante a Raspberry Pi: Transmisión de gestos**
    * **Tópico**: `sensors/data`
    * **Mensaje**: El ESP32 publica un mensaje que contiene los valores de los sensores flex (flexión de los dedos) y los datos del MPU6050 (movimiento y orientación de la mano).
    * **Descripción**: La Raspberry Pi se suscribe a este tópico para recibir en tiempo real las mediciones del guante. Estos datos son la entrada para el modelo de IA, que los interpreta para identificar una seña específica.

2.  **Raspberry Pi a Guante: Visualización de voz a texto**
    * **Tópico (Guante publica)**: `device/lcd`
    * **Mensaje**: El ESP32 publica un mensaje `ON` o `OFF` para indicar si la pantalla OLED está lista y encendida.
    * **Descripción**: La Raspberry Pi se suscribe a este tópico para saber si debe enviar texto.
    * **Tópico (Raspberry Pi publica)**: `comandos/esp32`
    * **Mensaje**: La Raspberry Pi publica un mensaje de texto que representa el discurso capturado por el micrófono.
    * **Descripción**: El ESP32 se suscribe a este tópico. Cuando detecta un nuevo mensaje, actualiza el texto en la pantalla OLED, permitiendo que el usuario con discapacidad auditiva "lea" lo que se le está diciendo.

---

## **Ventajas del uso de MQTT**

* **Eficiencia energética**: Su protocolo ligero minimiza el uso de datos y potencia, lo cual es ideal para un dispositivo a batería como el guante.
* **Comunicación asíncrona**: El ESP32 no necesita esperar una respuesta de la Raspberry Pi. Simplemente publica los datos y el broker se encarga de entregarlos, liberando los recursos del microcontrolador.
* **Uso menor de Hardwaare**: La arquitectura de "publicar/suscribir" desacopla la Raspberry Pi del ESP32. Esto permite minimizar el uso de dos ESP32 y ahorrar uno de ellos (forma tradicional para el funcionamiento BLE). 