/* File: main.c
   Path: main.c
   Versión: MPU6050 + flex idéntico al original, SIN LCD/LVGL/SSD1306.
   Definiciones I2C incluidas aquí.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ===== I2C / Pines (ahora definidos aquí) ===== */
#define I2C_BUS_PORT            I2C_NUM_0
#define EXAMPLE_PIN_NUM_SDA    6
#define EXAMPLE_PIN_NUM_SCL    7
#define EXAMPLE_PIN_NUM_RST   -1
#define EXAMPLE_I2C_HW_ADDR   0x3C
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (400 * 1000) /* usado como clock I2C speed */

/* ================= other config ================= */
#define ADC_UNIT ADC_UNIT_1
#define R_FIXED 10000.0
#define VCC 3.3f
#define N 8
#define PUBLISH_PERIOD_MS 100

#define FLEX0_CHANNEL ADC_CHANNEL_0
#define FLEX1_CHANNEL ADC_CHANNEL_1
#define FLEX2_CHANNEL ADC_CHANNEL_2
#define FLEX3_CHANNEL ADC_CHANNEL_3
#define FLEX4_CHANNEL ADC_CHANNEL_4

/* ---------------------------
   Globals
--------------------------- */
static int64_t last_mpu_time = 0;

static const char *TAG = "ManoMQTT";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle[5];

static esp_mqtt_client_handle_t mqtt_client = NULL;

/* ---------------------------
   MPU6050
--------------------------- */
static bool mpu_present = false;
static uint8_t mpu_addr = 0x68; /* intenta 0x68; si tu módulo usa AD0=1 usa 0x69 */

/* ---------------------------
   Filtro Kalman para ángulo MPU6050 (igual al original)
--------------------------- */
typedef struct {
    float angle;
    float bias;
    float rate;
    float P[2][2];
    float Q_angle;
    float Q_bias;
    float R_measure;
} Kalman_t;

static Kalman_t kalmanX, kalmanY, kalmanZ;

void kalman_init(Kalman_t *kalman) {
    kalman->angle = 0.0f;
    kalman->bias = 0.0f;
    kalman->rate = 0.0f;
    kalman->P[0][0] = 1.0f;
    kalman->P[0][1] = 0.0f;
    kalman->P[1][0] = 0.0f;
    kalman->P[1][1] = 1.0f;
    kalman->Q_angle = 0.001f;
    kalman->Q_bias = 0.003f;
    kalman->R_measure = 0.03f;
}

float kalman_update(Kalman_t *kalman, float newAngle, float newRate, float dt) {
    kalman->rate = newRate - kalman->bias;
    kalman->angle += dt * kalman->rate;

    kalman->P[0][0] += dt * (dt * kalman->P[1][1] - kalman->P[0][1] - kalman->P[1][0] + kalman->Q_angle);
    kalman->P[0][1] -= dt * kalman->P[1][1];
    kalman->P[1][0] -= dt * kalman->P[1][1];
    kalman->P[1][1] += kalman->Q_bias * dt;

    float S = kalman->P[0][0] + kalman->R_measure;
    float K[2];
    K[0] = kalman->P[0][0] / S;
    K[1] = kalman->P[1][0] / S;

    float y = newAngle - kalman->angle;
    if (y > 180.0f)  y -= 360.0f;
    if (y < -180.0f) y += 360.0f;

    kalman->angle += K[0] * y;
    kalman->bias  += K[1] * y;

    float P00_temp = kalman->P[0][0];
    float P01_temp = kalman->P[0][1];

    kalman->P[0][0] -= K[0] * P00_temp;
    kalman->P[0][1] -= K[0] * P01_temp;
    kalman->P[1][0] -= K[1] * P00_temp;
    kalman->P[1][1] -= K[1] * P01_temp;

    if (kalman->angle > 180.0f)  kalman->angle -= 360.0f;
    if (kalman->angle < -180.0f) kalman->angle += 360.0f;

    return kalman->angle;
}

/* ---------------------------
   Helpers I2C / MPU6050 (idénticos al original)
 --------------------------- */
static esp_err_t mpu6050_write_reg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr_7bit << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t mpu6050_read_regs(uint8_t dev_addr_7bit, uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr_7bit << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr_7bit << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static bool mpu6050_init_if_present(uint8_t dev_addr_7bit)
{
    uint8_t whoami = 0;
    if (mpu6050_read_regs(dev_addr_7bit, 0x75, &whoami, 1) == ESP_OK)
    {
        ESP_LOGI(TAG, "MPU6050 WHO_AM_I = 0x%02x", whoami);
        if (mpu6050_write_reg(dev_addr_7bit, 0x6B, 0x00) == ESP_OK)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            return true;
        }
    }
    return false;
}

static bool mpu6050_read_gyro_deg(int *gx, int *gy, int *gz)
{
    uint8_t buf[6];
    if (mpu6050_read_regs(mpu_addr, 0x43, buf, 6) != ESP_OK)
        return false;
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);
    *gx = (int)(raw_x / 131.0f + (raw_x >= 0 ? 0.5f : -0.5f));
    *gy = (int)(raw_y / 131.0f + (raw_y >= 0 ? 0.5f : -0.5f));
    *gz = (int)(raw_z / 131.0f + (raw_z >= 0 ? 0.5f : -0.5f));
    return true;
}

/* ---------------------------
   Helpers ADC / Flex (idéntico al original)
--------------------------- */
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
        if (adc_oneshot_read(adc_handle, channel, &adc_raw) == ESP_OK)
            sum_adc += adc_raw;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    int adc_avg = sum_adc / N;
    int voltage_mv = 0;
    if (cali)
        adc_cali_raw_to_voltage(cali, adc_avg, &voltage_mv);
    else
        voltage_mv = (adc_avg * 3300) / 4095;
    float voltage = voltage_mv / 1000.0f;

    float R_flex = 0.0f;
    if (flex_idx == 2) // Dedo mayor
    {
        float R_FIXED_MAYOR = 42000.0f;
        if (voltage > 0.01f)
            R_flex = R_FIXED_MAYOR * (VCC / voltage - 1.0f);
    }
    else
    {
        if (voltage > 0.01f)
            R_flex = R_FIXED * (VCC / voltage - 1.0f);
    }

    if (flex_init_val[flex_idx] == 10000)
    {
        snprintf(estado_out, len, "%d", 10000);
        flex_init_val[flex_idx] = (int)R_flex;
    }
    else
    {
        snprintf(estado_out, len, "%d", (int)R_flex);
    }
}

/* ---------------------------
   Lee acelerómetro (reg 0x3B..0x40), devuelve valores en g (float)
--------------------------- */
static bool mpu6050_read_accel_g(float *ax, float *ay, float *az)
{
    uint8_t buf[6];
    if (mpu6050_read_regs(mpu_addr, 0x3B, buf, 6) != ESP_OK)
        return false;
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);
    *ax = raw_x / 16384.0f;
    *ay = raw_y / 16384.0f;
    *az = raw_z / 16384.0f;
    return true;
}

/* ---------------------------
   Tarea periódica: publicar MPU + flex cada PUBLISH_PERIOD_MS
--------------------------- */
static void sensor_publish_task(void *arg)
{
    ESP_LOGI(TAG, "sensor_publish_task iniciado (cada %d ms)", PUBLISH_PERIOD_MS);
    last_mpu_time = esp_timer_get_time();

    while (1)
    {
        char estado_flex[5][20];
        for (int i = 0; i < 5; ++i)
        {
            leer_estado_flex(i, (adc_channel_t)(ADC_CHANNEL_0 + i), cali_handle[i], estado_flex[i], sizeof(estado_flex[i]));
        }

        int gx = 0, gy = 0, gz = 0;
        float ax = 0, ay = 0, az = 0;

        int disp_x_entero = 0;
        int disp_y_entero = 0;
        int disp_z_entero = 0;

        int64_t now = esp_timer_get_time();
        float delta_t = (now - last_mpu_time) / 1000000.0f;
        last_mpu_time = now;

        float angle_acc_x = 0, angle_acc_y = 0;

        if (mpu_present && mpu6050_read_gyro_deg(&gx, &gy, &gz) && mpu6050_read_accel_g(&ax, &ay, &az))
        {
            angle_acc_x = atan2f(ay, az) * 180.0f / M_PI;
            angle_acc_y = atan2f(ax, az) * 180.0f / M_PI;

            float kalman_angle_x = kalman_update(&kalmanX, angle_acc_x, gx, delta_t);
            float kalman_angle_y = kalman_update(&kalmanY, angle_acc_y, gy, delta_t);
            static float yaw = 0.0f;
            yaw += gz * delta_t;
            if (yaw >= 360.0f) yaw -= 360.0f;
            if (yaw < 0.0f) yaw += 360.0f;
            float kalman_angle_z = yaw;

            float disp_x = kalman_angle_x;
            float disp_y = kalman_angle_y;
            float disp_z = kalman_angle_z;

            if (disp_x < 0.0f) disp_x += 360.0f;
            if (disp_y < 0.0f) disp_y += 360.0f;
            if (disp_z < 0.0f) disp_z += 360.0f;

            if (disp_x >= 360.0f) disp_x -= 360.0f;
            if (disp_y >= 360.0f) disp_y -= 360.0f;
            if (disp_z >= 360.0f) disp_z -= 360.0f;

            disp_x_entero = (int)disp_x;
            disp_y_entero = (int)disp_y;
            disp_z_entero = (int)disp_z;
        }
        else
        {
            disp_x_entero = disp_y_entero = disp_z_entero = 0;
        }

        char mensaje[512];
        snprintf(mensaje, sizeof(mensaje),
                 "%d,%d,%d,"    // MPU X, Y, Z (enteros)
                 "%s,%s,%s,%s", // 4 Flex Sensores (cadenas de resistencia)
                 disp_x_entero, disp_y_entero, disp_z_entero,
                 estado_flex[1], estado_flex[2], estado_flex[3], estado_flex[4]);

        ESP_LOGI(TAG, "MQTT RAW DATA: %s", mensaje);

        if (mqtt_client)
        {
            int msg_id = esp_mqtt_client_publish(mqtt_client, "sensors/mpu_flex", mensaje, 0, 1, 0);
            ESP_LOGI(TAG, "Publicado sensors/mpu_flex, msg_id=%d", msg_id);
        }

        vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));
    }
}

/* ---------------------------
   MQTT handlers / start (adaptado: se suscribe a comandos/esp32)
--------------------------- */
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    mqtt_client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        kalman_init(&kalmanX);
        kalman_init(&kalmanY);
        kalman_init(&kalmanZ);

        int msg_id = esp_mqtt_client_subscribe(mqtt_client, "comandos/esp32", 0);
        ESP_LOGI(TAG, "Suscrito a 'comandos/esp32', msg_id=%d", msg_id);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA TOPIC=%.*s DATA=%.*s", event->topic_len, event->topic, event->data_len, event->data);
        /* Aquí podés parsear comandos si hace falta */
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle)
        {
            log_error_if_nonzero("esp_tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("sock errno", event->error_handle->esp_transport_sock_errno);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    const char *broker_uri = "mqtt://192.168.4.1:1884";
    const char *mqtt_user = "franco";
    const char *mqtt_pass = "_fr4nco_";

#ifdef CONFIG_BROKER_URL
    broker_uri = CONFIG_BROKER_URL;
#endif
#ifdef CONFIG_EXAMPLE_MQTT_USERNAME
    mqtt_user = CONFIG_EXAMPLE_MQTT_USERNAME;
#endif
#ifdef CONFIG_EXAMPLE_MQTT_PASSWORD
    mqtt_pass = CONFIG_EXAMPLE_MQTT_PASSWORD;
#endif

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = broker_uri,
        .broker.address.port = 1884,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,
    };

    if (mqtt_user || mqtt_pass)
    {
        mqtt5_cfg.credentials.username = mqtt_user;
        mqtt5_cfg.credentials.authentication.password = mqtt_pass;
    }

    mqtt_client = esp_mqtt_client_init(&mqtt5_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* ---------------------------
   app_main
--------------------------- */
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* ADC init (flex sensors) */
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    adc_oneshot_chan_cfg_t chan_cfg = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, FLEX0_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, FLEX1_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, FLEX2_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, FLEX3_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, FLEX4_CHANNEL, &chan_cfg));
    for (int i = 0; i < 5; i++)
    {
        adc_cali_curve_fitting_config_t cali_cfg;
        cali_cfg.unit_id = ADC_UNIT;
        cali_cfg.chan = ADC_CHANNEL_0 + i;
        cali_cfg.atten = ADC_ATTEN_DB_12;
        cali_cfg.bitwidth = ADC_BITWIDTH_12;
        adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle[i]);
    }

    /* I2C init (solo MPU6050 ahora) */
    ESP_LOGI(TAG, "Initialize I2C driver port %d SDA=%d SCL=%d", I2C_BUS_PORT, EXAMPLE_PIN_NUM_SDA, EXAMPLE_PIN_NUM_SCL);
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ},
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_BUS_PORT, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_BUS_PORT, I2C_MODE_MASTER, 0, 0, 0));

    /* Escaneo I2C (debug) */
    ESP_LOGI(TAG, "Escaneando bus I2C (puerto %d) ...", I2C_BUS_PORT);
    int detected_addr = -1;
    for (int addr = 1; addr < 127; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t r = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (r == ESP_OK)
        {
            ESP_LOGI(TAG, "I2C device encontrado en 0x%02x", addr);
            if (detected_addr == -1)
                detected_addr = addr;
            if (addr == EXAMPLE_I2C_HW_ADDR)
            {
                detected_addr = addr;
                break;
            }
        }
    }
    if (detected_addr == -1)
        ESP_LOGW(TAG, "No se detectó device I2C");
    else
        ESP_LOGI(TAG, "Direccion I2C detectada: 0x%02x", detected_addr);

    /* detectar MPU6050 (misma linea I2C) */
    if (mpu6050_init_if_present(mpu_addr))
    {
        mpu_present = true;
        ESP_LOGI(TAG, "MPU6050 detectado en 0x%02x", mpu_addr);
    }
    else
    {
        ESP_LOGI(TAG, "MPU6050 no detectado (bus I2C %d, addr 0x%02x)", I2C_BUS_PORT, mpu_addr);
    }

    /* arrancar tareas y servicios */
    xTaskCreate(sensor_publish_task, "sensor_publish_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Connecting to network (example_connect)");
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}
