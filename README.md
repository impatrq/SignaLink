# Carpeta De Trabajo ESP32 - Bluetooth Low Energy (BLE)
En esta rama se trabajará la utilizacion del BLE.
Se encontrarán dos carpetas necesarias para su funcionamiento:
  - **Gatt Client**
  - **Gatt Server** 
------------------------------------------------
## ⭐ Gatt - ¿Que es?
Gatt (Generic ATTribute Profile) hace uso de un protocolo de datos llamado Protocolo de Atributo (ATT) que se utiiza para almacenar servicios, caracteristicas y datos. Esta define la forma en que dos Dispositivos BLE transfieren datos de ida y vuelta. Lo más importante a tener en cuenta con GATT y las conexiones es que las conexiones son exclusivas. Lo que significa que un periférico BLE solo puede estar conectado a un dispositivo central a la vez. Tan pronto como un periférico se conecta a un dispositivo central, dejará de anunciarse y otros dispositivos ya no podrán verlo ni conectarse a él hasta que la conexión existente se rompa.
Establecer una conexión es la única forma de permitir la comunicación bidireccional, donde el dispositivo central puede enviar datos significativos al periférico y viceversa.

## 🔗 Servicios:
Los servicios se utilizan para dividir los datos en entidades lógicas y contienen bloques específicos de datos llamados características. Un servicio puede tener una o más características, y cada servicio se distingue de otros servicios mediante un ID numérico único llamado UUID, que puede ser de 16 bits (para servicios BLE oficialmente adoptados) o de 128 bits (para servicios personalizados).

# 🔗 Caractertisticas:
El concepto de nivel más bajo en las transacciones GATT es la Característica, que encapsula un único punto de datos (aunque puede contener un arreglo de datos relacionados, como los valores X/Y/Z de un acelerómetro de 3 ejes). Al igual que los Servicios, cada Característica se distingue a través de un UUID predefinido de 16 o 128 bits, y puedes usar las características estándar definidas por el Bluetooth SIG (lo que asegura la interoperabilidad entre el hardware/software habilitado para BLE) o definir tus propias características personalizadas que solo tu periférico y software entienden.

## 📌 Gatt Server
Es el encargado de almacenar los datos necesarios como servicios y caracteristicas.

## 📌 Gatt Client
Este es el cual se quiere conectar al dispositivo buscando servicios y caracteristicas, es decir, envia solicitudes al servidor para descubrir que es lo que contiene.
