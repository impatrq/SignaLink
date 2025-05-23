#include <stdio.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SENSOR_PIN ADC_CHANNEL_0
#define LED5_GPIO 5
#define LED6_GPIO 6
#define LED7_GPIO 7
#define LED8_GPIO 8
#define LED9_GPIO 9

float VCC = 5;
float R2 = 10000;
float sensorMinResistance = 1000;
float sensorMaxResistance = 30000;

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void app_main(void)
{
    // Configura los GPIO como salida
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED5_GPIO) | (1ULL << LED6_GPIO) | (1ULL << LED7_GPIO) | (1ULL << LED8_GPIO) | (1ULL << LED9_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    // Configura el ADC Oneshot
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc_handle, SENSOR_PIN, &chan_cfg);

    while (1)
    {
        int ADCRaw = 0;
        adc_oneshot_read(adc_handle, SENSOR_PIN, &ADCRaw);
        float ADCVoltage = (ADCRaw * VCC) / 4095.0;
        float Resistance = R2 * (VCC / ADCVoltage - 1);
        uint8_t ReadValue = map(Resistance, sensorMinResistance, sensorMaxResistance, 0, 100);

        gpio_set_level(LED5_GPIO, 0);
        gpio_set_level(LED6_GPIO, 0);
        gpio_set_level(LED7_GPIO, 0);
        gpio_set_level(LED8_GPIO, 0);
        gpio_set_level(LED9_GPIO, 0);

        if (ReadValue < 20)
        {
            gpio_set_level(LED5_GPIO, 1);
        }
        else if (ReadValue < 40)
        {
            gpio_set_level(LED5_GPIO, 1);
            gpio_set_level(LED6_GPIO, 1);
        }
        else if (ReadValue < 60)
        {
            gpio_set_level(LED5_GPIO, 1);
            gpio_set_level(LED6_GPIO, 1);
            gpio_set_level(LED7_GPIO, 1);
        }
        else if (ReadValue < 80)
        {
            gpio_set_level(LED5_GPIO, 1);
            gpio_set_level(LED6_GPIO, 1);
            gpio_set_level(LED7_GPIO, 1);
            gpio_set_level(LED8_GPIO, 1);
        }
        else
        {
            gpio_set_level(LED5_GPIO, 1);
            gpio_set_level(LED6_GPIO, 1);
            gpio_set_level(LED7_GPIO, 1);
            gpio_set_level(LED8_GPIO, 1);
            gpio_set_level(LED9_GPIO, 1);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}