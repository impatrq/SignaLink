#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Configuración
#define ADC_CHANNEL_0   ADC_CHANNEL_0 // GPIO0
#define ADC_CHANNEL_1   ADC_CHANNEL_1 // GPIO1
#define ADC_CHANNEL_2   ADC_CHANNEL_2 // GPIO2
#define ADC_CHANNEL_3   ADC_CHANNEL_3 // GPIO3
#define ADC_CHANNEL_4   ADC_CHANNEL_4 // GPIO4
#define ADC_UNIT        ADC_UNIT_1    // ADC1
#define R_FIXED         10000.0       // Ohmios (resistencia fija)
#define VCC             3.3           // Voltaje de referencia (3.3V)
#define N               8             // Número de muestras para el promedio

void app_main(void) {
    // Configuración ADC oneshot
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config);
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &config);
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_2, &config);
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_3, &config);
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &config);

    // Calibración para todos los canales
    adc_cali_handle_t cali_handle_0 = NULL;
    adc_cali_curve_fitting_config_t cali_config_0 = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL_0,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config_0, &cali_handle_0);

    adc_cali_handle_t cali_handle_1 = NULL;
    adc_cali_curve_fitting_config_t cali_config_1 = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config_1, &cali_handle_1);

    adc_cali_handle_t cali_handle_2 = NULL;
    adc_cali_curve_fitting_config_t cali_config_2 = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL_2,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config_2, &cali_handle_2);

    adc_cali_handle_t cali_handle_3 = NULL;
    adc_cali_curve_fitting_config_t cali_config_3 = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL_3,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config_3, &cali_handle_3);

    adc_cali_handle_t cali_handle_4 = NULL;
    adc_cali_curve_fitting_config_t cali_config_4 = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL_4,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config_4, &cali_handle_4);

    while (1) {
        // Promedio para Sensor 0
        int sum_adc_0 = 0;
        for (int i = 0; i < N; i++) {
            int adc_raw_0 = 0;
            adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &adc_raw_0);
            sum_adc_0 += adc_raw_0;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        int adc_raw_0_avg = sum_adc_0 / N;
        int voltage_mv_0 = 0;
        adc_cali_raw_to_voltage(cali_handle_0, adc_raw_0_avg, &voltage_mv_0);
        float voltage_0 = voltage_mv_0 / 1000.0f;
        float R_flex_0 = R_FIXED * (VCC / voltage_0 - 1.0);

        // Promedio para Sensor 1
        int sum_adc_1 = 0;
        for (int i = 0; i < N; i++) {
            int adc_raw_1 = 0;
            adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &adc_raw_1);
            sum_adc_1 += adc_raw_1;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        int adc_raw_1_avg = sum_adc_1 / N;
        int voltage_mv_1 = 0;
        adc_cali_raw_to_voltage(cali_handle_1, adc_raw_1_avg, &voltage_mv_1);
        float voltage_1 = voltage_mv_1 / 1000.0f;
        float R_flex_1 = R_FIXED * (VCC / voltage_1 - 1.0);

        // Promedio para Sensor 2
        int sum_adc_2 = 0;
        for (int i = 0; i < N; i++) {
            int adc_raw_2 = 0;
            adc_oneshot_read(adc_handle, ADC_CHANNEL_2, &adc_raw_2);
            sum_adc_2 += adc_raw_2;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        int adc_raw_2_avg = sum_adc_2 / N;
        int voltage_mv_2 = 0;
        adc_cali_raw_to_voltage(cali_handle_2, adc_raw_2_avg, &voltage_mv_2);
        float voltage_2 = voltage_mv_2 / 1000.0f;
        float R_flex_2 = R_FIXED * (VCC / voltage_2 - 1.0);

        // Promedio para Sensor 3
        int sum_adc_3 = 0;
        for (int i = 0; i < N; i++) {
            int adc_raw_3 = 0;
            adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &adc_raw_3);
            sum_adc_3 += adc_raw_3;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        int adc_raw_3_avg = sum_adc_3 / N;
        int voltage_mv_3 = 0;
        adc_cali_raw_to_voltage(cali_handle_3, adc_raw_3_avg, &voltage_mv_3);
        float voltage_3 = voltage_mv_3 / 1000.0f;
        float R_flex_3 = R_FIXED * (VCC / voltage_3 - 1.0);

        // Promedio para Sensor 4
        int sum_adc_4 = 0;
        for (int i = 0; i < N; i++) {
            int adc_raw_4 = 0;
            adc_oneshot_read(adc_handle, ADC_CHANNEL_4, &adc_raw_4);
            sum_adc_4 += adc_raw_4;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        int adc_raw_4_avg = sum_adc_4 / N;
        int voltage_mv_4 = 0;
        adc_cali_raw_to_voltage(cali_handle_4, adc_raw_4_avg, &voltage_mv_4);
        float voltage_4 = voltage_mv_4 / 1000.0f;
        float R_flex_4 = R_FIXED * (VCC / voltage_4 - 1.0);

        // Estados individuales
        // ------------------------
        // Sensor 0 estado, vamos a usarlo para el INDICE(flex N°10)
        const char* estado_0 = "";
        if (R_flex_0 < 50000 && R_flex_0 > 37000 ) {
            estado_0 = "muy flexionado";
        } else if (R_flex_0 > 20000 && R_flex_0 < 37000) {
            estado_0 = "medio";
        } else if (R_flex_0 > 50000) {
            estado_0 = "maximo";
        } else if (R_flex_0 > 13000 && R_flex_0 < 20000) {
            estado_0 = "recto";
        }

        // Sensor 1 estado, vamos a usarlo para el DEDO MAYOR(flex N°12)
        const char* estado_1 = "";
        if (R_flex_1 < 40000 && R_flex_1 > 29000 ) {
            estado_1 = "muy flexionado";
        } else if (R_flex_1 > 15000 && R_flex_1 < 29000) {
            estado_1 = "medio";
        } else if (R_flex_1 > 40000) {
            estado_1 = "maximo";
        } else if (R_flex_1 > 11000 && R_flex_1 < 15000) {
            estado_1 = "recto";
        }

        // Sensor 2 estado, hasta el momento no anda DEDO ANULAR(flex N°9)
        const char* estado_2 = "";
        if (R_flex_2 < 9000 && R_flex_2 > 8000 ) {
            estado_2 = "muy flexionado";
        } else if (R_flex_2 > 6000 && R_flex_2 < 8000) {
            estado_2 = "medio";
        } else if (R_flex_2 > 5000 && R_flex_2 < 6000) {
            estado_2 = "recto";
        }

        // Sensor 3 estado(flex N°7) DEDO GORDO
        const char* estado_3 = "";
        if (R_flex_3 < 67000 && R_flex_3 > 55000 ) {
            estado_3 = "muy flexionado";
        } else if (R_flex_3 > 20000 && R_flex_3 < 55000) {
            estado_3 = "medio";
        } else if (R_flex_3 > 67000) {
            estado_3 = "maximo";
        } else if (R_flex_3 > 13000 && R_flex_3 < 20000) {
            estado_3 = "recto";
        }

        // Sensor 4 estado flex N° 11 DEDO MEÑIQUE
        const char* estado_4 = "";
        if (R_flex_4 < 52000 && R_flex_4 > 45000 ) {
            estado_4 = "muy flexionado";
        } else if (R_flex_4 > 20000 && R_flex_4 < 45000) {
            estado_4 = "medio";
        } else if (R_flex_4 > 52000) {
            estado_4 = "maximo";
        } else if (R_flex_4 > 13000 && R_flex_4 < 20000) {
            estado_4 = "recto";
        }

        // Frase combinada para los 5 sensores
        const char* frase = "";
        int rectos = 0, muy_flexionados = 0, medios = 0, maximos = 0;

        // Cuenta cada estado
        if (strcmp(estado_0, "recto") == 0) rectos++;
        if (strcmp(estado_1, "recto") == 0) rectos++;
        if (strcmp(estado_2, "recto") == 0) rectos++;
        if (strcmp(estado_3, "recto") == 0) rectos++;
        if (strcmp(estado_4, "recto") == 0) rectos++;

        if (strcmp(estado_0, "muy flexionado") == 0) muy_flexionados++;
        if (strcmp(estado_1, "muy flexionado") == 0) muy_flexionados++;
        if (strcmp(estado_2, "muy flexionado") == 0) muy_flexionados++;
        if (strcmp(estado_3, "muy flexionado") == 0) muy_flexionados++;
        if (strcmp(estado_4, "muy flexionado") == 0) muy_flexionados++;

        if (strcmp(estado_0, "medio") == 0) medios++;
        if (strcmp(estado_1, "medio") == 0) medios++;
        if (strcmp(estado_2, "medio") == 0) medios++;
        if (strcmp(estado_3, "medio") == 0) medios++;
        if (strcmp(estado_4, "medio") == 0) medios++;

        if (strcmp(estado_0, "maximo") == 0) maximos++;
        if (strcmp(estado_1, "maximo") == 0) maximos++;
        if (strcmp(estado_2, "maximo") == 0) maximos++;
        if (strcmp(estado_3, "maximo") == 0) maximos++;
        if (strcmp(estado_4, "maximo") == 0) maximos++;

        // Decide la frase principal
        if (strcmp(estado_0, "muy flexionado") == 0 &&
            strcmp(estado_1, "muy flexionado") == 0 &&
            strcmp(estado_3, "muy flexionado") == 0 &&
            strcmp(estado_4, "muy flexionado") == 0 &&
            strcmp(estado_2, "muy flexionado") == 0) {
            frase = "A";
        } else if (rectos == 5) {
            frase = "Todos rectos";
        } else if (muy_flexionados == 5) {
            frase = "Todos muy flexionados";
        } else if (medios == 5) {
            frase = "Todos medio";
        } else if (maximos == 5) {
            frase = "Todos al maximo";
        } 

        // Imprimir resultados
        printf("--------------------------------------------------\n");
        printf("S0: ADC %d | Vout %.3f V | R %.1f Ω | %s\n", adc_raw_0_avg, voltage_0, R_flex_0, estado_0);
        printf("S1: ADC %d | Vout %.3f V | R %.1f Ω | %s\n", adc_raw_1_avg, voltage_1, R_flex_1, estado_1);
        printf("S2: ADC %d | Vout %.3f V | R %.1f Ω | %s\n", adc_raw_2_avg, voltage_2, R_flex_2, estado_2);
        printf("S3: ADC %d | Vout %.3f V | R %.1f Ω | %s\n", adc_raw_3_avg, voltage_3, R_flex_3, estado_3);
        printf("S4: ADC %d | Vout %.3f V | R %.1f Ω | %s\n", adc_raw_4_avg, voltage_4, R_flex_4, estado_4);
        printf("Frase: %s\n", frase);
        printf("--------------------------------------------------\n");

        // Delay para evitar saturar la salida
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}