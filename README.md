# PCM5102 I2S DAC

## 📌 Descripción

El **PCM5102** es un **DAC (Digital to Analog Converter)** de Texas Instruments, ampliamente utilizado para proyectos de audio Hi-Fi con microcontroladores y SBCs (como Raspberry Pi, ESP32, Arduino, etc.).  
Convierte señales digitales en formato **I2S** a una salida de audio analógica estéreo de **alta calidad**.

Se caracteriza por:

- Excelente relación señal/ruido (SNR).
- Bajo nivel de distorsión armónica (THD+N).
- Funcionamiento con alimentación simple (3.3V o 5V, dependiendo de la placa módulo).
- Compatibilidad con resoluciones de hasta **32-bit** y frecuencias de muestreo hasta **384 kHz**.

---

## ⚙️ Especificaciones técnicas

- **Chip**: Texas Instruments PCM5102A
- **Resolución**: 16, 24 y 32 bits
- **Frecuencia de muestreo**: 8 kHz – 384 kHz
- **SNR (Signal-to-Noise Ratio)**: ~112 dB
- **THD+N (Total Harmonic Distortion + Noise)**: ~-93 dB
- **Salidas**: Estéreo (L/R) analógicas, en conector RCA o pines según módulo
- **Entradas**: Interfaz **I2S**
  - BCK (Bit Clock)
  - LRCK (Word Select / Left-Right Clock)
  - DIN (Data In)
- **Alimentación**: 3.3V (núcleo) / 5V (módulo)
- **Consumo**: ~10-20 mA

---

## 🔌 Pinout PCM5102

Los módulos más comunes tienen pines como:

| Pin     | Descripción             |
| ------- | ----------------------- |
| **VIN** | Alimentación 5V (Pin 2) |
| **GND** | Tierra (Pin 6)          |
| **LCK** | (Pin 35)                |
| **DIN** | (Pin 40)                |
| **BCK** | (Pin 12)                |
| **SCK** | (GND)                   |

---
