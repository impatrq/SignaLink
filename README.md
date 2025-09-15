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

## 🔌 Pinout típico del módulo PCM5102
Los módulos más comunes tienen pines como:

| Pin        | Descripción |
|------------|-------------|
| **5V / VCC** | Alimentación (algunos módulos permiten 3.3V) |
| **GND**     | Tierra |
| **BCK**     | Bit Clock (desde el microcontrolador) |
| **LRCK**    | Word Select / Left-Right Clock |
| **DIN**     | Datos de audio (serial data input) |
| **SCK**     | Master Clock (opcional, muchos módulos funcionan sin él) |
| **L OUT**   | Salida analógica canal izquierdo |
| **R OUT**   | Salida analógica canal derecho |

---

## 🛠️ Conexión con microcontroladores
Ejemplo con **ESP32 (I2S integrado)**:

| ESP32 (I2S) | PCM5102 |
|-------------|---------|
| GPIO BCK    | BCK     |
| GPIO WS/LRCK| LRCK    |
| GPIO DATA   | DIN     |
| GND         | GND     |
| 3.3V / 5V   | VCC     |

> ⚠️ Notar que algunos módulos requieren 5V en VCC, pero internamente regulan a 3.3V.

---

## 📋 Ejemplo de uso (ESP32 con Arduino IDE)
```cpp
#include "driver/i2s.h"

#define I2S_BCK  26
#define I2S_WS   25
#define I2S_DATA 22

void setup() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .intr_alloc_flags = 0,
    .tx_desc_auto_clear = true
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
  // Ejemplo: enviar samples a 440 Hz
  static const int sample_rate = 44100;
  static float freq = 440.0;
  static int i = 0;
  int16_t sample = (int16_t)(sin(2 * PI * freq * i / sample_rate) * 32767);
  i++;
  size_t bytes_written;
  i2s_write(I2S_NUM_0, (char*)&sample, sizeof(sample), &bytes_written, portMAX_DELAY);
}
