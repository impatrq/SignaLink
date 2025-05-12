#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h" // API recomendada para lecturas individuales
#include "esp_log.h"
#include "math.h" // Para fabs()

// --- Configuraciones ---
#define LED_PIN GPIO_NUM_5              // Pin del LED (GPIO5)
#define MIC_ADC_CHANNEL ADC_CHANNEL_4   // Canal ADC conectado al micrófono (GPIO4 -> ADC1_CH4 en ESP32-C3)
#define ADC_ATTENUATION ADC_ATTEN_DB_11 // Atenuación para rango completo (0-3.3V aprox. dependiendo de Vref)

#define AMBIENT_SAMPLE_COUNT 200   // Número de muestras para calcular el ruido ambiente
#define DETECTION_SAMPLE_COUNT 20  // Número de muestras para detectar sonido actual
#define TASK_DELAY_MS 50           // Pausa entre detecciones (milisegundos)
#define SOUND_THRESHOLD_MARGIN 100 // Margen sobre el ruido ambiente para detectar voz (ajustar según pruebas)
// --- Fin Configuraciones ---

static const char *TAG = "MIC_DETECTOR";
static adc_oneshot_unit_handle_t adc1_handle; // Handle para la unidad ADC1
static int ambient_noise_level = 0;           // Nivel de ruido ambiente calculado (promedio de amplitud)
static int dc_offset = 0;                     // Nivel DC del micrófono (aproximadamente VCC/2)

// --- Funciones de Configuración ---

static void configure_led(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "LED configurado en GPIO %d", LED_PIN);
}

// Función para configurar el ADC en modo OneShot
static esp_err_t configure_adc(void)
{
    // Configuración de inicialización de la unidad ADC
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1 // Usamos ADC1 en ESP32-C3
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error al inicializar ADC1: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configuración del canal ADC
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Ancho de bits (normalmente 12 bits)
        .atten = ADC_ATTENUATION          // Atenuación seleccionada
    };
    ret = adc_oneshot_config_channel(adc1_handle, MIC_ADC_CHANNEL, &config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error al configurar canal ADC %d: %s", MIC_ADC_CHANNEL, esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "ADC1 Canal %d configurado", MIC_ADC_CHANNEL);
    }
    return ret;
}

// --- Función de Calibración de Ruido Ambiente ---

static void calculate_ambient_noise(void)
{
    long total_value = 0;
    long total_amplitude = 0;
    int adc_reading = 0;

    ESP_LOGI(TAG, "Calculando ruido ambiente... Mantén silencio por favor.");

    // 1. Tomar muestras para estimar el nivel DC y el ruido
    for (int i = 0; i < AMBIENT_SAMPLE_COUNT; i++)
    {
        esp_err_t ret = adc_oneshot_read(adc1_handle, MIC_ADC_CHANNEL, &adc_reading);
        if (ret == ESP_OK)
        {
            total_value += adc_reading;
        }
        else
        {
            ESP_LOGW(TAG, "Error lectura ADC en calibración: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // Pequeña pausa entre muestras
    }

    // 2. Estimar el offset DC (el nivel promedio cuando no hay sonido)
    dc_offset = total_value / AMBIENT_SAMPLE_COUNT;
    ESP_LOGI(TAG, "Offset DC estimado: %d", dc_offset);

    // 3. Calcular el nivel promedio de amplitud (ruido) alrededor del offset DC
    total_value = 0; // Reutilizamos para la suma de amplitudes absolutas
    for (int i = 0; i < AMBIENT_SAMPLE_COUNT; i++)
    {
        esp_err_t ret = adc_oneshot_read(adc1_handle, MIC_ADC_CHANNEL, &adc_reading);
        if (ret == ESP_OK)
        {
            total_amplitude += abs(adc_reading - dc_offset); // Suma de la amplitud absoluta
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    ambient_noise_level = total_amplitude / AMBIENT_SAMPLE_COUNT;
    ESP_LOGI(TAG, "Nivel de ruido ambiente (amplitud promedio): %d", ambient_noise_level);
    ESP_LOGI(TAG, "Umbral de detección de sonido: > %d", ambient_noise_level + SOUND_THRESHOLD_MARGIN);
    ESP_LOGI(TAG, "Calibración completada. Puedes hablar.");
}

// --- Tarea de Detección de Sonido ---

static void sound_detection_task(void *pvParameter)
{
    int current_amplitude_sum = 0;
    int current_avg_amplitude = 0;
    int adc_reading = 0;

    while (1)
    {
        current_amplitude_sum = 0;
        // Tomar unas pocas muestras para promediar el nivel actual
        for (int i = 0; i < DETECTION_SAMPLE_COUNT; ++i)
        {
            esp_err_t ret = adc_oneshot_read(adc1_handle, MIC_ADC_CHANNEL, &adc_reading);
            if (ret == ESP_OK)
            {
                current_amplitude_sum += abs(adc_reading - dc_offset); // Acumular amplitud
            }
            else
            {
                ESP_LOGW(TAG, "Error lectura ADC en detección: %s", esp_err_to_name(ret));
            }
            // No añadimos delay aquí para muestrear rápido, el delay principal está al final del while(1)
        }
        current_avg_amplitude = current_amplitude_sum / DETECTION_SAMPLE_COUNT;

        // Comparar con el umbral
        if (current_avg_amplitude > (ambient_noise_level + SOUND_THRESHOLD_MARGIN))
        {
            // Sonido detectado por encima del umbral
            gpio_set_level(LED_PIN, 1); // Encender LED
            // ESP_LOGD(TAG, "Sonido detectado! Nivel: %d", current_avg_amplitude); // Descomentar para debug detallado
        }
        else
        {
            // Solo ruido ambiente o silencio
            gpio_set_level(LED_PIN, 0); // Apagar LED
            // ESP_LOGD(TAG, "Ambiente. Nivel: %d", current_avg_amplitude); // Descomentar para debug detallado
        }

        // Esperar antes de la próxima detección
        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }
}

// --- Función Principal ---

void app_main(void)
{
    // Es buena práctica inicializar NVS aunque no se use directamente aquí
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    configure_led();
    esp_err_t adc_config_status = configure_adc();

    if (adc_config_status == ESP_OK)
    {
        // Dar tiempo para que el usuario haga silencio
        ESP_LOGI(TAG, "Preparando para calibrar ruido ambiente en 3 segundos...");
        vTaskDelay(pdMS_TO_TICKS(3000));

        // Calcular ruido ambiente
        calculate_ambient_noise();

        // Crear la tarea de detección de sonido
        xTaskCreate(sound_detection_task, "sound_detect_task", 2048, NULL, 5, NULL);
        ESP_LOGI(TAG, "Tarea de detección de sonido iniciada.");
    }
    else
    {
        ESP_LOGE(TAG, "Fallo al configurar ADC. La detección de sonido no puede iniciar.");
    }
}