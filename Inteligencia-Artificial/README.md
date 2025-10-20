# Carpeta de Trabajo - IA de Reconocimiento de Señas

Este módulo es una *base de entrenamiento con TensorFlow en Python* para el proyecto *SignaLink*. Simula el reconocimiento de señas a partir de los datos obtenidos de los sensores flex, giroscopio y acelerómetro.

<div align="center">
<img src="images/redneuronal.png" alt="" width="20%"/>
</div>

## 📌 Objetivo

Desarrollar una red neuronal capaz de identificar señas utilizando:

•⁠  ⁠5 sensores flex (uno por dedo)
•⁠  ⁠3 ejes del giroscopio (X, Y, Z)
•⁠  ⁠3 ejes del acelerómetro (X, Y, Z)

Total de *11 entradas por muestra*.

## ⚙️ Requisitos

•⁠  ⁠Python 3.8+
•⁠  ⁠TensorFlow

## ✅ Flujo completo de entrenamiento y uso del modelo IA en SignaLink

1.⁠ ⁠*Guardar datos reales de los sensores*

•⁠  ⁠5 sensores flex (dedos)
•⁠  ⁠3 ejes del giroscopio 
•⁠  ⁠3 ejes del acelerómetro 
•⁠  ⁠Asignar etiqueta a cada conjunto de datos (en este caso a cada seña)

Esos valores se deben guardar en un archivo *.csv* o *.json*

2.⁠ ⁠*Instalar TensorFlow Lite en Raspberry Pi Zero 2W*

⁠ bash
   pip install tflite-runtime
    ⁠

3.⁠ ⁠*🧠 Entrenar el modelo en la PC*

•⁠  ⁠Usar Python con TensorFlow
•⁠  ⁠Leer los datos reales
•⁠  ⁠Crear y entrenar la red neuronal
•⁠  ⁠Guardar el modelo como .keras

4.⁠ ⁠*⚙️ Convertir el modelo a .tflite*

    Los modelos entrenados con TensorFlow en la PC (extensión .keras o .h5) son muy pesados y están diseñados para computadoras con buena potencia. La Raspberry Pi Zero 2W es un dispositivo de bajo consumo, así que se necesita convertir el modelo a TensorFlow Lite (.tflite), que es un formato liviano, optimizado y rápido.

5.⁠ ⁠*🍓 Ejecutar el modelo .tflite en la Raspberry Pi*

    Una vez entrenado el modelo en la PC y convertido a formato TensorFlow Lite (.tflite), el siguiente paso es usarlo dentro de la Raspberry Pi Zero 2W para hacer predicciones en tiempo real. Se utiliza la biblioteca tflite-runtime, una versión ligera especialmente diseñada para dispositivos con recursos limitados.
