#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h" 
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TXD_PIN (GPIO_NUM_20)
#define RXD_PIN (GPIO_NUM_21)
#define UART_PORT_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_4
#define ADC_ATTEN ADC_ATTEN_DB_11 // ¡Volvemos a la constante anterior!
#define SAMPLE_RATE 16000

static const char *TAG = "ESP32_AUDIO_UART";
static adc_oneshot_unit_handle_t adc1_handle;

// ¡Prototipo de la función!
void adc_sample_and_send(void *arg);

void app_main(void) {
    // --- Configuración UART ---
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(TAG, "UART configurado en puerto %d (TX: %d, RX: %d) a %d bps",
             UART_PORT_NUM, TXD_PIN, RXD_PIN, uart_config.baud_rate);

    // --- Configuración ADC ---
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));
    ESP_LOGI(TAG, "ADC1 Canal %d configurado", ADC_CHANNEL);

    // --- Bucle principal para muestreo y envío ---
    int delay_us = 1000000 / SAMPLE_RATE;
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = (void (*)(void*)) &adc_sample_and_send,
        .name = "adc_sampler",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, delay_us));

    ESP_LOGI(TAG, "Inicio del muestreo y envío de audio...");
}

void adc_sample_and_send(void *arg) {
    uint16_t adc_reading = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL, (int*)&adc_reading);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Enviando por UART: %d (0x%04X)", adc_reading, (uint16_t)adc_reading);
        uart_write_bytes(UART_PORT_NUM, (const char *)&adc_reading, sizeof(adc_reading));
    } else {
        ESP_LOGE(TAG, "Error al leer el ADC: %s", esp_err_to_name(ret));
    }
} 