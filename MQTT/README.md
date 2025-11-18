  
# ⚙️ Comunicación MQTT 

Este documento describe la arquitectura y el flujo de comunicación entre una Raspberry Pi Compute Module 4 y un Esp32-C2 Super mini, utilizando MQTT

## 💡 Concepto General

El proyecto se basa en el patrón de comunicación Publish/Subscribe (Publicar/Suscribir) que define MQTT. Esta arquitectura utiliza un punto central o servidor llamado Broker para gestionar todos los mensajes, asegurando que la comunicación sea eficiente, ligera y asíncrona.

## 💻 Roles de los dispositivos

### 1.Raspberry Pi CM4

Actua como el servidos central de mensajes; su funcion en es recibir las publicaciones y reenviaros a los clientes que se hayan suscrito a un tema especifico

### 2. Esp32-C3 Super Mini

Actua como el cliente y se conecta a la red local del Broker para poder publicar datos o suscribirse 
