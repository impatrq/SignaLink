# Carpeta de trabajo Sensor de Flexión 4.5" (10–30 kΩ)

Este repositorio contiene información y ejemplos de uso para el sensor de flexión de 4.5 pulgadas, que varía su resistencia entre 10 kΩ (en posición recta) y aproximadamente 30 kΩ (cuando se flexiona a 45°). Este tipo de sensor es útil para detectar movimientos o cambios de ángulo en diversas aplicaciones.

## 🖐 ¿Qué hacen los sensores flex en SignaLink?

Los sensores flex (flex sensors) son resistencias variables que cambian su valor cuando se doblan. Cuanto más se dobla el sensor, mayor es su resistencia. En el proyecto SignaLink, estos sensores están colocados sobre los dedos de un guante, de modo que:
	1.	Cada vez que un dedo se flexiona o se estira, el sensor detecta ese movimiento como un cambio de resistencia.
	2.	Ese cambio de resistencia se convierte en una señal analógica, que puede ser leída por un microcontrolador (como el ESP32).
	3.	Estas señales permiten determinar la posición aproximada de cada dedo en tiempo real.

## 🎯 ¿Para qué sirve esto?

El propósito principal en SignaLink es reconocer las señas del lenguaje de señas manual. Al combinar los valores de varios sensores flex (uno por dedo, por ejemplo), se puede inferir qué gesto está haciendo la persona.

Luego:
	•	Esa seña se traduce por voz, usando un sistema de síntesis (TTS) en la Raspberry Pi del sistema.

## 📦 Especificaciones Técnicas

- **Longitud total:** 4.5 pulgadas (~114 mm)
- **Resistencia en posición recta:** ~10 kΩ
- **Resistencia al flexionar (45°):** ~30 kΩ
- **Rango de temperatura de operación:** -35°C a +80°C
- **Ciclo de vida:** >1 millón de ciclos
- **Tolerancia de resistencia:** ±30%
- **Conector:** Espaciado de 0.1", compatible con breadboards

## 🛠️ Aplicaciones Comunes

- Interfaces de usuario flexibles
- Guantes para captura de movimiento
- Controladores de juegos
- Dispositivos médicos y de rehabilitación
- Instrumentos musicales electrónicos

## 🔌 Conexión y Uso Básico

El sensor funciona como un divisor de voltaje. 

## ⚠️ Consideraciones
- Evita doblar el sensor más allá de su límite especificado para prolongar su vida útil.
- Asegúrate de que las conexiones estén firmes para obtener lecturas precisas.
- La resistencia puede variar ligeramente entre diferentes unidades debido a la tolerancia de fabricación.

## Datasheet

https://www.mouser.com/datasheet/2/381/Flex_Sensor_Datasheet_v2019a-3304101.pdf
