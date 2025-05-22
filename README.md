# Carpeta de Trabajo MPU6050 

El MPU6050 es un sensor que integra un giroscopo de 3 ejes y un acelerómetro de 3 ejes en un mismo chip, lo que permite medir movimiento en 6 grados de libertad. Combina la medición de la aceleración (acelerómetro) y la velocidad angular (giroscopio) para obtener información sobre el movimiento y la orientación de nuestra mano.

## 🧭 Giroscopo:

Mide la velocidad angular, es decir, la velocidad a la que se rota en cada uno de los tres ejes (X, Y, Z). Utiliza un sistema MEMS (Micro-Electro-Mechanical Systems) para detectar la rotación. El MPU6050 tiene un rango programable para la velocidad angular, que puede ser ajustado a 250, 500, 1000 o 2000 grados por segundo. La sensibilidad del giroscopio se mide en grados por segundo.

## 🧭 Acelerómetro:

Mide la aceleración en cada uno de los tres ejes (X, Y, Z), es decir, el cambio de velocidad. También utiliza un sistema MEMS para detectar la aceleración. El MPU6050 tiene un rango programable para la aceleración, que puede ser ajustado a ±2, ±4, ±8 o ±16 g. La aceleración en el eje Z, en un objeto estático, corresponde a la gravedad, mientras que en los ejes X e Y, idealmente, debería ser cero.

## ✋ Interpretación de Movimientos de la Mano (Basado en el Código):

En nuestro código, hemos configurado el MPU6050 para interpretar los movimientos de tu mano de una manera más intuitiva, asumiendo que el sensor está colocado sobre el empeine. Para lograr esto y asegurar que los ángulos se muestren de forma precisa, hemos llevado a cabo:

- Mapeo de Ejes: Hemos redefinido cómo los datos crudos del MPU6050 se traducen a los movimientos de tu mano:

    - El Eje X representa la inclinación lateral de la mano (hacia la derecha o izquierda).

    - El Eje Y representa el movimiento de levantar o bajar la mano (hacia adelante o hacia atrás).

    - El Eje Z representa la rotación de la muñeca.

- Detección de mano neutra: 

    Hemos añadido una condición para detectar cuando la mano está en una posición "neutra" al inicio o durante el uso, permitiendo que el sistema reconozca tu posición de inicial.

Estos ajustes permiten que el sensor no solo mida el movimiento, sino que también lo interprete de una forma más comprensible para simular los gestos de la mano.

## 📷 Imagen del Microcontrolador MPU6050 

![](img/mpu6050.jpg)

## 🧭 Ejes X Y Z

![](img/ejes_mpu6050.jpg)

## 🔗 Datasheet

https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
