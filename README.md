# Carpeta De Trabajo ESP32 - MQQT (Message Queuing Telemetry Transport)
En esta rama se trabajará la utilizacion del MQQT para llevar a cabo la funcion del broker y los clientes que lograran la conexion entre todos los modulos del proyecto. 

---

MQTT (Message Queuing Telemetry Transport) es un protocolo de mensajería ligero, ideal para el Internet de las Cosas (IoT) y la comunicación entre máquinas (M2M) en entornos con ancho de banda limitado y dispositivos con recursos escasos. A diferencia de las conexiones punto a punto, MQTT opera bajo un modelo de publicación-suscripción (publish-subscribe).

En este modelo, los dispositivos no se comunican directamente. En su lugar, todos se conectan a un intermediario central llamado Broker. Los dispositivos que quieren enviar datos se convierten en publicadores (Publishers), enviando mensajes a un tema (Topic) específico en el Broker. Los dispositivos que quieren recibir esos datos se convierten en suscriptores (Subscribers), y simplemente se suscriben al tema de su interés. Esto permite una comunicación desacoplada y flexible.

El modelo de publicación-suscripción de MQTT es una alternativa poderosa al enfoque de cliente-servidor de GATT, especialmente para redes con múltiples dispositivos que necesitan intercambiar información de forma asíncrona.