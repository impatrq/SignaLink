# Carpeta de trabajo Sensor de Flexión 4.5" (10–30 kΩ)

Este repositorio contiene información y ejemplos de uso para el sensor de flexión de 4.5 pulgadas, que varía su resistencia entre 10 kΩ (en posición recta) y aproximadamente 30 kΩ (cuando se flexiona a 45°). Este tipo de sensor es útil para detectar movimientos o cambios de ángulo en diversas aplicaciones.

## ¿Qué es un sensor flex? 🤔

Un sensor flex es un transductor resistivo flexible que varía su resistencia eléctrica en función del grado de curvatura o flexión al que es sometido. Estos sensores son ampliamente utilizados para medir deformaciones angulares en aplicaciones como guantes inteligentes 🧤, robótica 🤖, biomecánica 🦿, wearables ⌚ y sistemas de interfaz humano-máquina 🧑‍💻.

## Estructura y materiales 🧩

Típicamente, un sensor flex consiste en:

- Un sustrato flexible (plástico o poliéster) 🪢
- Una pista conductora de carbono o polímero conductor 🟤 impresa sobre el sustrato
- Terminales metálicos en los extremos para conexión eléctrica ⚡

## Principio de funcionamiento ⚙️

- Cuando el sensor está **recto** ➡️ (sin flexión), los elementos conductores están próximos y la resistencia es **baja** 🔽.
- Al **doblarse** ➰, el sustrato se deforma, separando las partículas conductoras y **aumentando la resistencia** 🔼.
- La relación entre el ángulo de flexión y la resistencia suele ser aproximadamente lineal 📈 en muchos modelos comerciales.

## 🖐 ¿Qué hacen los sensores flex en SignaLink?

En el proyecto SignaLink, estos sensores están colocados sobre los dedos de un guante, de modo que: 1. Cada vez que un dedo se flexiona o se estira, el sensor detecta ese movimiento como un cambio de resistencia. 2. Ese cambio de resistencia se convierte en una señal analógica, que puede ser leída por un microcontrolador.. 3. Estas señales permiten determinar la posición aproximada de cada dedo en tiempo real.

## 🎯 ¿Para qué sirve esto?

El propósito principal en SignaLink es reconocer las señas del lenguaje de señas manual. Al combinar los valores de varios sensores flex (uno por dedo, por ejemplo), se puede inferir qué gesto está haciendo la persona.

Luego:
• Esa seña se traduce por voz, usando un sistema de síntesis (TTS) en la Raspberry Pi del sistema.

## 🛠️ Aplicaciones Comunes

- Interfaces de usuario flexibles
- Guantes para captura de movimiento
- Controladores de juegos
- Dispositivos médicos y de rehabilitación
- Instrumentos musicales electrónicos

## Referencias 📚

- [Flex Sensor Datasheet (Spectra Symbol)](https://www.spectrasymbol.com/wp-content/uploads/2016/08/FLEX-SENSOR-DATA-SHEET.pdf)
- Ejemplo de uso en guantes inteligentes: [Smart_Glove](https://github.com/bufferboy-commits/Smart_Glove)
