#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"

// Includes para los sensores
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "sensor_mqtt";

// Global MQTT client handle
static esp_mqtt_client_handle_t global_mqtt_client = NULL;
static bool mqtt_is_connected = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_is_connected = true;
        // No se suscribe ni publica aquí, la tarea de los sensores lo hará
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_is_connected = false;
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.127.44:1884",
        .credentials.username = "franco",
        .credentials.authentication.password = "_fr4nco_",
    };
    
    global_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(global_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(global_mqtt_client);
}

// ==== MPU6050 Y SENSORES FLEX CONFIGURACIÓN Y FUNCIONES ====
#define I2C_MASTER_SDA_IO 6
#define I2C_MASTER_SCL_IO 7
#define I2C_MASTER_NUM 0
#define I2C_MASTER_FREQ_HZ 400000

#define MPU6050_ADDR 0x69
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H 0x43
#define GYRO_YOUT_H 0x45
#define GYRO_ZOUT_H 0x47
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C

#define ADC_UNIT ADC_UNIT_1
#define R_FIXED 10000.0
#define VCC 3.3
#define N 8

#define FLEX0_CHANNEL ADC_CHANNEL_0
#define FLEX1_CHANNEL ADC_CHANNEL_1
#define FLEX2_CHANNEL ADC_CHANNEL_2
#define FLEX3_CHANNEL ADC_CHANNEL_3
#define FLEX4_CHANNEL ADC_CHANNEL_4

static const char *MPU_TAG = "MPU6050";

typedef struct
{
    float accel_offset_x, accel_offset_y, accel_offset_z;
    float gyro_offset_x, gyro_offset_y, gyro_offset_z;
} mpu6050_calibration_t;

static mpu6050_calibration_t calibration = {0};
static int64_t last_time_us = 0;
static float current_yaw = 0.0;

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle[5];

static char ultimo_gesto[64] = "";
static char ultimo_estado_flex[5][20] = {"", "", "", "", ""};

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK)
        return err;
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, 2, 1000 / portTICK_PERIOD_MS);
}

static esp_err_t mpu6050_register_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
}

static void mpu6050_init(void)
{
    ESP_ERROR_CHECK(mpu6050_register_write_byte(PWR_MGMT_1, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(mpu6050_register_write_byte(GYRO_CONFIG, 0x00));
    ESP_ERROR_CHECK(mpu6050_register_write_byte(ACCEL_CONFIG, 0x00));
    ESP_LOGI(MPU_TAG, "MPU6050 inicializado");
}

static int16_t read_raw_data(uint8_t register_addr)
{
    uint8_t data[2];
    if (mpu6050_register_read_bytes(register_addr, data, 2) != ESP_OK)
        return 0;
    return (data[0] << 8) | data[1];
}

static void calibrate_mpu6050(int samples)
{
    long ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    for (int i = 0; i < samples; i++)
    {
        ax += read_raw_data(ACCEL_XOUT_H);
        ay += read_raw_data(ACCEL_YOUT_H);
        az += read_raw_data(ACCEL_ZOUT_H);
        gx += read_raw_data(GYRO_XOUT_H);
        gy += read_raw_data(GYRO_YOUT_H);
        gz += read_raw_data(GYRO_ZOUT_H);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    calibration.accel_offset_x = (float)ax / samples;
    calibration.accel_offset_y = (float)ay / samples;
    calibration.accel_offset_z = ((float)az / samples) - 16384.0;
    calibration.gyro_offset_x = (float)gx / samples;
    calibration.gyro_offset_y = (float)gy / samples;
    calibration.gyro_offset_z = (float)gz / samples;
    ESP_LOGI(MPU_TAG, "Calibración completa");
}

static void calculate_euler(float ax, float ay, float az, float *roll, float *pitch)
{
    *roll = atan2(ax, az) * (180.0 / M_PI);
    float temp_pitch = atan2(ay, az) * (180.0 / M_PI);
    *pitch = fmod(temp_pitch + 180.0, 360.0);
    if (*pitch < 0)
        *pitch += 360.0;
    if (*roll < 0)
        *roll += 360.0;
}

static void get_sensor_data(float *accel_x_g, float *accel_y_g, float *accel_z_g,
                            float *gyro_x_dps, float *gyro_y_dps, float *gyro_z_dps,
                            float *angle_x_deg, float *angle_y_deg, float *angle_z_deg)
{
    int16_t raw_ax = read_raw_data(ACCEL_XOUT_H);
    int16_t raw_ay = read_raw_data(ACCEL_YOUT_H);
    int16_t raw_az = read_raw_data(ACCEL_ZOUT_H);
    int16_t raw_gx = read_raw_data(GYRO_XOUT_H);
    int16_t raw_gy = read_raw_data(GYRO_YOUT_H);
    int16_t raw_gz = read_raw_data(GYRO_ZOUT_H);

    *accel_x_g = ((float)raw_ax - calibration.accel_offset_x) / 16384.0;
    *accel_y_g = ((float)raw_ay - calibration.accel_offset_y) / 16384.0;
    *accel_z_g = ((float)raw_az - calibration.accel_offset_z) / 16384.0;
    *gyro_x_dps = ((float)raw_gx - calibration.gyro_offset_x) / 131.0;
    *gyro_y_dps = ((float)raw_gy - calibration.gyro_offset_y) / 131.0;
    *gyro_z_dps = ((float)raw_gz - calibration.gyro_offset_z) / 131.0;

    calculate_euler(*accel_x_g, *accel_y_g, *accel_z_g, angle_x_deg, angle_y_deg);

    int64_t now = esp_timer_get_time();
    float dt = (float)(now - last_time_us) / 1000000.0;
    current_yaw += (*gyro_z_dps * dt);
    last_time_us = now;
    if (current_yaw > 360.0)
        current_yaw -= 360.0;
    else if (current_yaw < 0.0)
        current_yaw += 360.0;
    *angle_z_deg = current_yaw;
}

static void leer_estado_flex(int flex_idx, int channel, adc_cali_handle_t cali, char *estado_out, size_t len)
{
    static int initialized = 0;
    static int flex_init_val[5] = {0};
    if (!initialized)
    {
        for (int i = 0; i < 5; i++)
            flex_init_val[i] = 10000;
        initialized = 1;
    }
    int sum_adc = 0;
    for (int i = 0; i < N; i++)
    {
        int adc_raw = 0;
        adc_oneshot_read(adc_handle, channel, &adc_raw);
        sum_adc += adc_raw;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    int adc_avg = sum_adc / N;
    int voltage_mv = 0;
    adc_cali_raw_to_voltage(cali, adc_avg, &voltage_mv);
    float voltage = voltage_mv / 1000.0f;
    float R_flex = R_FIXED * (VCC / voltage - 1.0);
    if (flex_init_val[flex_idx] == 10000)
    {
        snprintf(estado_out, len, "%d", 10000);
        flex_init_val[flex_idx] = -1;
    }
    else
    {
        snprintf(estado_out, len, "%d", (int)R_flex);
    }
}

static void sensor_task(void *arg)
{
    float ang_x, ang_y, ang_z;
    last_time_us = esp_timer_get_time();

    while (1)
    {
        // Espera a que el cliente MQTT se conecte
        if (!mqtt_is_connected) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // Obtener valores de los sensores y armar el mensaje
        float ax, ay, az, gx, gy, gz;
        get_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &ang_x, &ang_y, &ang_z);
        char notify_mpu_msg[64];
        snprintf(notify_mpu_msg, sizeof(notify_mpu_msg), "MPU6050: x=%.0f, y=%.0f, z=%.0f", ang_x, ang_y, ang_z);

        char estado_flex[5][20];
        leer_estado_flex(0, FLEX0_CHANNEL, cali_handle[0], estado_flex[0], sizeof(estado_flex[0]));
        leer_estado_flex(1, FLEX1_CHANNEL, cali_handle[1], estado_flex[1], sizeof(estado_flex[1]));
        leer_estado_flex(2, FLEX2_CHANNEL, cali_handle[2], estado_flex[2], sizeof(estado_flex[2]));
        leer_estado_flex(3, FLEX3_CHANNEL, cali_handle[3], estado_flex[3], sizeof(estado_flex[3]));
        leer_estado_flex(4, FLEX4_CHANNEL, cali_handle[4], estado_flex[4], sizeof(estado_flex[4]));

        // Solo publicar si hay cambios en los datos
        bool flex_cambio = false;
        for (int i = 0; i < 5; i++)
        {
            if (strcmp(estado_flex[i], ultimo_estado_flex[i]) != 0)
            {
                flex_cambio = true;
                break;
            }
        }
        bool mpu_cambio = (strcmp(notify_mpu_msg, ultimo_gesto) != 0);

        if (flex_cambio || mpu_cambio)
        {
            char mensaje_mqtt[256];
            snprintf(mensaje_mqtt, sizeof(mensaje_mqtt),
                     "%s | Indice:%s Mayor:%s Anular:%s Gordo:%s Meñique:%s",
                     notify_mpu_msg, estado_flex[0], estado_flex[1], estado_flex[2], estado_flex[3], estado_flex[4]);

            ESP_LOGI(TAG, "Publicando a MQTT: %s", mensaje_mqtt);

            int msg_id = esp_mqtt_client_publish(global_mqtt_client, "test/topic", mensaje_mqtt, 0, 1, 0);
            if (msg_id != -1) {
                ESP_LOGI(TAG, "Publicación exitosa, msg_id=%d", msg_id);
            } else {
                ESP_LOGE(TAG, "Fallo al publicar, msg_id=%d", msg_id);
            }

            // Actualiza los últimos estados
            strcpy(ultimo_gesto, notify_mpu_msg);
            for (int i = 0; i < 5; i++)
            {
                strcpy(ultimo_estado_flex[i], estado_flex[i]);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Publicar cada 500 ms
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("sensor_mqtt", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet */
    ESP_ERROR_CHECK(example_connect());

    // === INICIALIZACIÓN MPU6050 ===
    ESP_LOGI(MPU_TAG, "Inicializando I2C y MPU6050...");
    ESP_ERROR_CHECK(i2c_master_init());
    mpu6050_init();
    calibrate_mpu6050(500);

    // === INICIALIZACIÓN ADC PARA SENSORES FLEX ===
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT};
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    adc_oneshot_config_channel(adc_handle, FLEX0_CHANNEL, &config);
    adc_oneshot_config_channel(adc_handle, FLEX1_CHANNEL, &config);
    adc_oneshot_config_channel(adc_handle, FLEX2_CHANNEL, &config);
    adc_oneshot_config_channel(adc_handle, FLEX3_CHANNEL, &config);
    adc_oneshot_config_channel(adc_handle, FLEX4_CHANNEL, &config);

    adc_cali_curve_fitting_config_t cali_cfg;
    for (int i = 0; i < 5; i++)
    {
        cali_cfg.unit_id = ADC_UNIT;
        cali_cfg.chan = ADC_CHANNEL_0 + i;
        cali_cfg.atten = ADC_ATTEN_DB_12;
        cali_cfg.bitwidth = ADC_BITWIDTH_12;
        adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle[i]);
    }
    
    // === INICIO DEL CLIENTE MQTT Y LA TAREA DE SENSORES ===
    mqtt_app_start();

    // La tarea de sensores se inicia aquí, pero espera a que MQTT esté conectado para publicar
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}