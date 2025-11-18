#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "rom/ets_sys.h"

// Pines definidos para ADC
#define ADC_CHANNEL ADC_CHANNEL_1 // GPIO 01
#define UART_PORT_NUM UART_NUM_1
#define TX_PIN GPIO_NUM_20 // GPIO 20
#define RX_PIN GPIO_NUM_21 // GPIO 21

void app_main(void)
{
    // Configura UART para salida de datos
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_PORT_NUM, 256, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Configura ADC1 en modo oneshot
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config);

    while (1)
    {
        int val = 0;
        for (uint32_t i = 0; i < 1000; i++)
        {
            // Leer el valor del ADC
            adc_oneshot_read(adc1_handle, ADC_CHANNEL, &val);

            char buffer[16];
            int len = snprintf(buffer, sizeof(buffer), "%d\n", val);
            uart_write_bytes(UART_PORT_NUM, buffer, len);

            // (Opcional) Imprimir en consola
            printf("%d\n", val);

            ets_delay_us(62);
        }
        return;
    }
}