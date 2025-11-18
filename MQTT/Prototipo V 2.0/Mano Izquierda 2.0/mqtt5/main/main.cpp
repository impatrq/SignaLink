/*
  main.cpp - ESP32 + SparkFun BNO08x (BNO085) integration
  Reemplaza al MPU6050: publica ejes fusionados (Euler yaw/pitch/roll)
  en el mismo formato: "X,Y,Z,flex1,flex2,flex3,flex4".
*/

extern "C" {
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

#include "i2c_oled.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ssd1306.h"
}

#include <Arduino.h>
#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"

static const char *TAG = "ManoMQTT";
static const char *LCD_TAG = "example";

/* ---------------------------
   Defines / configuración
--------------------------- */

#ifndef EXAMPLE_PIN_NUM_SDA
#ifdef CONFIG_EXAMPLE_PIN_NUM_SDA
#define EXAMPLE_PIN_NUM_SDA CONFIG_EXAMPLE_PIN_NUM_SDA
#else
#define EXAMPLE_PIN_NUM_SDA 6
#endif
#endif
#ifndef EXAMPLE_PIN_NUM_SCL
#ifdef CONFIG_EXAMPLE_PIN_NUM_SCL
#define EXAMPLE_PIN_NUM_SCL CONFIG_EXAMPLE_PIN_NUM_SCL
#else
#define EXAMPLE_PIN_NUM_SCL 7
#endif
#endif
#ifndef EXAMPLE_PIN_NUM_RST
#ifdef CONFIG_EXAMPLE_PIN_NUM_RST
#define EXAMPLE_PIN_NUM_RST CONFIG_EXAMPLE_PIN_NUM_RST
#else
#define EXAMPLE_PIN_NUM_RST -1
#endif
#endif
#ifndef EXAMPLE_I2C_HW_ADDR
#ifdef CONFIG_EXAMPLE_I2C_HW_ADDR
#define EXAMPLE_I2C_HW_ADDR CONFIG_EXAMPLE_I2C_HW_ADDR
#else
#define EXAMPLE_I2C_HW_ADDR 0x3C
#endif
#endif
#ifndef EXAMPLE_LCD_H_RES
#define EXAMPLE_LCD_H_RES 128
#endif
#ifndef EXAMPLE_LCD_V_RES
#ifdef CONFIG_EXAMPLE_SSD1306_HEIGHT
#define EXAMPLE_LCD_V_RES CONFIG_EXAMPLE_SSD1306_HEIGHT
#else
#define EXAMPLE_LCD_V_RES 32
#endif
#endif
#ifndef EXAMPLE_LCD_CMD_BITS
#define EXAMPLE_LCD_CMD_BITS 8
#endif
#ifndef EXAMPLE_LCD_PIXEL_CLOCK_HZ
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ 400000
#endif
#ifndef I2C_BUS_PORT
#define I2C_BUS_PORT I2C_NUM_0
#endif

#define ADC_UNIT ADC_UNIT_1
#define R_FIXED 10000.0
#define VCC 3.3f
#define N 8                  
#define PUBLISH_PERIOD_MS 100 /* publicar cada 100 ms */

#define FLEX0_CHANNEL ADC_CHANNEL_0
#define FLEX1_CHANNEL ADC_CHANNEL_1
#define FLEX2_CHANNEL ADC_CHANNEL_2
#define FLEX3_CHANNEL ADC_CHANNEL_3
#define FLEX4_CHANNEL ADC_CHANNEL_4

/* ---------------------------
   Globals
--------------------------- */

static int64_t last_mpu_time = 0;

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle[5];

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool lcd_ready = false; /* indica que el LCD fue inicializado */
static lv_obj_t *g_label = NULL;

/* ---------------------------
   BNO085 (SparkFun) setup
--------------------------- */
BNO080 myIMU;           // objeto SparkFun
static bool bno_present = false;

/* ---------------------------
   MPU6050 (fallback) - acceso I2C directo
   Nota: MPU6050 no tiene magnetómetro; yaw no estará disponible correctamente.
--------------------------- */
static const uint8_t MPU_ADDR = 0x68;
static bool mpu_present = false;
static float last_known_yaw = 0.0f;

/* ---------------------------
   Low-power state flag
--------------------------- */
static volatile bool 
low_power_mode = false; // true = en reposo (no publicar sensores)

/* ---------------------------
   Helpers: actualizar texto del LCD desde otros tasks
--------------------------- */
void lcd_set_text(const char *txt)
{
    if (!txt)
        return;
    if (lvgl_port_lock(0))
    {
        if (g_label)
        {
            lv_label_set_text(g_label, txt);
            lv_label_set_long_mode(g_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        }
        else
        {
            ESP_LOGW(TAG, "lcd_set_text: label no inicializado");
        }
        lvgl_port_unlock();
    }
    else
    {
        ESP_LOGW(TAG, "lcd_set_text: no se pudo tomar lock lvgl");
    }
}

/* ---------------------------
   Helpers ADC / Flex
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
    if (flex_idx == 2) // Dedo mayor: ajustar calibración
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
   Conversión quaternion -> Euler (deg)
   Fórmulas usado: roll(x), pitch(y), yaw(z)
--------------------------- */
static void quat_to_euler_deg(float qw, float qx, float qy, float qz, float *roll, float *pitch, float *yaw)
{
    // roll (x-axis rotation)
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    *roll = atan2f(sinr_cosp, cosr_cosp) * 180.0f / M_PI;

    // pitch (y-axis)
    float sinp = 2.0f * (qw * qy - qz * qx);
    if (fabsf(sinp) >= 1.0f)
        *pitch = copysignf(90.0f, sinp); // use 90 degrees if out of range
    else
        *pitch = asinf(sinp) * 180.0f / M_PI;

    // yaw (z-axis)
    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    *yaw = atan2f(siny_cosp, cosy_cosp) * 180.0f / M_PI;
}

/* ---------------------------
   MPU6050 I2C helpers (lectura accel, sin librería externa)
   - detect & init: despierta al MPU (PWR_MGMT_1 = 0)
   - read accel regs 0x3B..0x40
   - convierte a g usando sensibilidad 16384 (±2g)
--------------------------- */
static bool mpu_detect_and_init()
{
    Wire.beginTransmission(MPU_ADDR);
    int r = Wire.endTransmission();
    if (r != 0)
    {
        ESP_LOGW(TAG, "MPU6050 no detectado en 0x%02x (endTransmission=%d)", MPU_ADDR, r);
        return false;
    }

    // wake up MPU (PWR_MGMT_1 = 0)
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0x00);
    if (Wire.endTransmission() != 0)
    {
        ESP_LOGW(TAG, "No se pudo escribir en MPU6050 PWR_MGMT_1");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "MPU6050 detectado e inicializado en 0x%02x", MPU_ADDR);
    return true;
}

static bool mpu_read_accel_g(float *ax, float *ay, float *az)
{
    // read 6 bytes from ACCEL_XOUT_H (0x3B)
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) // repeated start
        return false;
    if (Wire.requestFrom((int)MPU_ADDR, 6) != 6)
        return false;
    int16_t raw_ax = (Wire.read() << 8) | Wire.read();
    int16_t raw_ay = (Wire.read() << 8) | Wire.read();
    int16_t raw_az = (Wire.read() << 8) | Wire.read();
    const float SENS = 16384.0f; // ±2g default
    *ax = raw_ax / SENS;
    *ay = raw_ay / SENS;
    *az = raw_az / SENS;
    return true;
}

/* ---------------------------
   Power control helpers
--------------------------- */
static void handle_power_off_cmd()
{
    ESP_LOGI(TAG, "Recibido comando POWER OFF -> entrando en reposo");
    if (mqtt_client)
    {
        esp_mqtt_client_publish(mqtt_client, "power/off", "Entrando en Reposo..", 0, 1, 0);
    }
    lcd_set_text("Entrando en Reposo..");
    // marcar modo reposo: el task de sensores dejará de publicar
    low_power_mode = true;
    // opcional: si querés desactivar hardware pesado agregá código acá.
}

static void handle_power_on_cmd()
{
    ESP_LOGI(TAG, "Recibido comando POWER ON -> arrancando sistema");
    if (mqtt_client)
    {
        esp_mqtt_client_publish(mqtt_client, "power/on", "Prendiendo Sistema..", 0, 1, 0);
    }
    lcd_set_text("Prendiendo Sistema..");
    low_power_mode = false;
    // intentar reactivar BNO si estaba inhabilitado
    if (!bno_present)
    {
        ESP_LOGI(TAG, "Intentando re-inicializar BNO08x tras power/on");
        if (myIMU.begin() == true)
        {
            bno_present = true;
            myIMU.enableRotationVector();
            ESP_LOGI(TAG, "BNO08x re-inicializado OK");
        }
        else
        {
            ESP_LOGW(TAG, "No se pudo re-inicializar BNO08x tras power/on");
        }
    }
    else
    {
        // re-habilitar feature por si se había parado
        myIMU.enableRotationVector();
    }
}

/* ---------------------------
   Tarea periódica: publicar BNO085 (Euler) + flex cada PUBLISH_PERIOD_MS
   Añadida lógica de detección de fallo del BNO y fallback a MPU6050.
--------------------------- */
static void sensor_publish_task(void *arg)
{
    ESP_LOGI(TAG, "sensor_publish_task iniciado (cada %d ms)", PUBLISH_PERIOD_MS);
    last_mpu_time = esp_timer_get_time(); // microsegundos

    int bno_missing_count = 0;
    const int BNO_MISSING_THRESHOLD = 100; // 100 * 100ms = ~10s

    while (1)
    {
        if (low_power_mode)
        {
            // En reposo: no enviamos datos de sensores. Mantener conexión MQTT para recibir power/on.
            vTaskDelay(pdMS_TO_TICKS(1000)); // dormir 1s y volver a verificar
            continue;
        }

        char estado_flex[5][20];
        for (int i = 0; i < 5; ++i)
        {
            leer_estado_flex(i, (adc_channel_t)(ADC_CHANNEL_0 + i), cali_handle[i], estado_flex[i], sizeof(estado_flex[i]));
        }

        int disp_x_entero = 0;
        int disp_y_entero = 0;
        int disp_z_entero = 0;

        // Prioridad BNO085 -> fallback MPU6050
        if (bno_present)
        {
            if (myIMU.dataAvailable())
            {
                float qw = 1.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;
                myIMU.getQuat(&qw, &qx, &qy, &qz);

                float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;
                quat_to_euler_deg(qw, qx, qy, qz, &roll, &pitch, &yaw);

                // Convertir a 0..360 (para mantener formato existente)
                if (roll < 0.0f) roll += 360.0f;
                if (pitch < 0.0f) pitch += 360.0f;
                if (yaw < 0.0f) yaw += 360.0f;
                if (roll >= 360.0f) roll -= 360.0f;
                if (pitch >= 360.0f) pitch -= 360.0f;
                if (yaw >= 360.0f) yaw -= 360.0f;

                disp_x_entero = (int)roll;
                disp_y_entero = (int)pitch;
                disp_z_entero = (int)yaw;

                // reset missing counter y guardar last yaw
                bno_missing_count = 0;
                last_known_yaw = yaw;
            }
            else
            {
                // No hay datos del BNO en este ciclo
                bno_missing_count++;
                if (bno_missing_count > BNO_MISSING_THRESHOLD)
                {
                    ESP_LOGW(TAG, "BNO085 no provee datos por %d ciclos -> deshabilitando BNO y intentando MPU6050", BNO_MISSING_THRESHOLD);
                    bno_present = false;
                    // intentar inicializar MPU6050
                    if (mpu_detect_and_init())
                    {
                        mpu_present = true;
                        ESP_LOGI(TAG, "Fallback: MPU6050 listo");
                    }
                    else
                    {
                        mpu_present = false;
                        ESP_LOGW(TAG, "Fallback: MPU6050 no detectado");
                    }
                }
                // mientras tanto, mantener últimos valores o 0
                disp_x_entero = disp_y_entero = (int)0;
                disp_z_entero = (int)last_known_yaw;
            }
        }
        else if (mpu_present)
        {
            float ax = 0, ay = 0, az = 0;
            if (mpu_read_accel_g(&ax, &ay, &az))
            {
                // calcular roll/pitch aproximado desde acelerómetro
                float roll = atan2f(ay, az) * 180.0f / M_PI;
                float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / M_PI;
                float yaw = last_known_yaw; // no hay magnetómetro; usar último yaw conocido

                if (roll < 0.0f) roll += 360.0f;
                if (pitch < 0.0f) pitch += 360.0f;
                if (yaw < 0.0f) yaw += 360.0f;
                if (roll >= 360.0f) roll -= 360.0f;
                if (pitch >= 360.0f) pitch -= 360.0f;
                if (yaw >= 360.0f) yaw -= 360.0f;

                disp_x_entero = (int)roll;
                disp_y_entero = (int)pitch;
                disp_z_entero = (int)yaw;
            }
            else
            {
                // lectura fallida -> mandar ceros
                disp_x_entero = disp_y_entero = disp_z_entero = 0;
            }
        }
        else
        {
            // Ningún sensor disponible
            disp_x_entero = disp_y_entero = disp_z_entero = 0;
        }

        char mensaje[512];
        snprintf(mensaje, sizeof(mensaje),
                 "%d,%d,%d,"    // BNO X, Y, Z (enteros) o fallback
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
   MQTT handlers / start  (idénticos)
--------------------------- */
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
}

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    mqtt_client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, "comandos/esp32", 0);
        ESP_LOGI(TAG, "Suscrito a 'comandos/esp32', msg_id=%d", msg_id);

        int msg_id2 = esp_mqtt_client_subscribe(mqtt_client, "display/lcd", 0);
        ESP_LOGI(TAG, "Suscrito a 'display/lcd', msg_id=%d", msg_id2);

        // Suscribir nuevos topics de power
        int pid_on = esp_mqtt_client_subscribe(mqtt_client, "power/on", 0);
        ESP_LOGI(TAG, "Suscrito a 'power/on', msg_id=%d", pid_on);
        int pid_off = esp_mqtt_client_subscribe(mqtt_client, "power/off", 0);
        ESP_LOGI(TAG, "Suscrito a 'power/off', msg_id=%d", pid_off);

        if (lcd_ready)
        {
            const char *payload = "ON";
            int id = esp_mqtt_client_publish(mqtt_client, "display/lcd", payload, 0, 1, 1);
            ESP_LOGI(TAG, "Publicado estado LCD '%s' en 'display/lcd' (retained), msg_id=%d", payload, id);
        }
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        {
            char topic_buf[128];
            size_t tlen = event->topic_len < sizeof(topic_buf) - 1 ? event->topic_len : sizeof(topic_buf) - 1;
            memcpy(topic_buf, event->topic, tlen);
            topic_buf[tlen] = '\0';

            size_t dlen = event->data_len;
            char *data_buf = (char *)malloc(dlen + 1);
            if (data_buf)
            {
                memcpy(data_buf, event->data, dlen);
                data_buf[dlen] = '\0';

                // Manejo topic display
                if (strcmp(topic_buf, "display/lcd") == 0)
                {
                    ESP_LOGI(TAG, "Actualizar LCD con texto MQTT: %s", data_buf);
                    lcd_set_text(data_buf);
                }
                // Manejo power topics
                else if (strcmp(topic_buf, "power/off") == 0)
                {
                    ESP_LOGI(TAG, "Received power/off -> payload: %s", data_buf);
                    handle_power_off_cmd();
                }
                else if (strcmp(topic_buf, "power/on") == 0)
                {
                    ESP_LOGI(TAG, "Received power/on -> payload: %s", data_buf);
                    handle_power_on_cmd();
                }
                else
                {
                    ESP_LOGI(TAG, "Mensaje recibido en topic diferente: %s", topic_buf);
                }

                free(data_buf);
            }
            else
            {
                ESP_LOGW(TAG, "No hay memoria para copiar payload MQTT");
            }
        }
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

/* Arranca el cliente MQTT (usa CONFIG_BROKER_URL/USER/PASS si existen, si no usa fallback) */
static void mqtt5_app_start(void)
{
    const char *broker_uri = "mqtt://192.168.4.1:1884";
    const char *mqtt_user = "franco";
    const char *mqtt_pass = "_fr4nco_";

#ifdef CONFIG_BROKER_URL
    broker_uri = CONFIG_BROKER_URL;
#endif

#ifdef CONFIG_EXAMPLE_MQTT_USERNAME
    mqtt_user = CONFIG_EXAMPLE_MQTT_USERNAME;
#else
    mqtt_user = "franco"; /* fallback si no config */
#endif
#ifdef CONFIG_EXAMPLE_MQTT_PASSWORD
    mqtt_pass = CONFIG_EXAMPLE_MQTT_PASSWORD;
#else
    mqtt_pass = "_fr4nco_"; /* fallback si no config */
#endif

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = broker_uri,
        .broker.address.port = 1884, /* puerto que usás en la RPi */
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,
    };

    if (mqtt_user || mqtt_pass)
    {
        mqtt5_cfg.credentials.username = mqtt_user;
        mqtt5_cfg.credentials.authentication.password = mqtt_pass;
    }

    mqtt_client = esp_mqtt_client_init(&mqtt5_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* ---------------------------
   LVGL UI task (label único centrado y desplazable)
--------------------------- */
static void lvgl_ui_task(void *pvParameter)
{
    lv_disp_t *disp = (lv_disp_t *)pvParameter;
    ESP_LOGI(TAG, "lvgl_ui_task: UI single centered scrolling label (full screen)");
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_disp_get_scr_act(disp);
        lv_obj_clean(scr); /* borrar objetos previos */

        g_label = lv_label_create(scr);
        lv_label_set_long_mode(g_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text(g_label, "Esperando Conexion. . .");
        lv_obj_set_width(g_label, disp->driver->hor_res - 4);
        lv_obj_align(g_label, LV_ALIGN_CENTER, 0, 0);

        lvgl_port_unlock();
    }

    while (1)
    {
        if (lvgl_port_lock(0))
        {
            lv_timer_handler();
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ---------------------------
   SSD1306 raw helper (prueba)
--------------------------- */
static esp_err_t ssd1306_write_cmd_raw(uint8_t dev_addr_7bit, uint8_t cmd)
{
    int8_t slave_addr = dev_addr_7bit;
    i2c_cmd_handle_t cmdlink = i2c_cmd_link_create();
    i2c_master_start(cmdlink);
    i2c_master_write_byte(cmdlink, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmdlink, 0x00, true); // control byte command
    i2c_master_write_byte(cmdlink, cmd, true);
    i2c_master_stop(cmdlink);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmdlink, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmdlink);
    return err;
}
static bool ssd1306_raw_test(uint8_t dev_addr_7bit)
{
    ESP_LOGI(LCD_TAG, "SSD1306 raw test: enviar DISPLAY OFF then ON a 0x%02x", dev_addr_7bit);
    if (ssd1306_write_cmd_raw(dev_addr_7bit, 0xAE) != ESP_OK)
    {
        ESP_LOGW(LCD_TAG, "raw 0xAE falló");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    if (ssd1306_write_cmd_raw(dev_addr_7bit, 0xAF) != ESP_OK)
    {
        ESP_LOGW(LCD_TAG, "raw 0xAF falló");
        return false;
    }
    ESP_LOGI(LCD_TAG, "ssd1306 raw write OK");
    return true;
}

/* ---------------------------
   app_main
--------------------------- */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ManoMQTT", ESP_LOG_NONE);
    esp_log_level_set("mqtt_client", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    esp_log_level_set("wifi:sta", ESP_LOG_DEBUG);

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

    /* I2C legacy init (SSD1306 + BNO085 comparten bus) */
    ESP_LOGI(LCD_TAG, "Initialize I2C legacy driver port %d SDA=%d SCL=%d", I2C_BUS_PORT, EXAMPLE_PIN_NUM_SDA, EXAMPLE_PIN_NUM_SCL);
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

    /* Escaneo I2C */
    ESP_LOGI(LCD_TAG, "Escaneando bus I2C (puerto %d) ...", I2C_BUS_PORT);
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
            ESP_LOGI(LCD_TAG, "I2C device encontrado en 0x%02x", addr);
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
        ESP_LOGW(LCD_TAG, "No se detectó device I2C");
    else
        ESP_LOGI(LCD_TAG, "Direccion I2C detectada: 0x%02x", detected_addr);

    if (detected_addr != -1)
    {
        bool ok = ssd1306_raw_test((uint8_t)detected_addr);
        if (!ok)
            ESP_LOGW(LCD_TAG, "ssd1306_raw_test falló");
    }

    // Crear panel IO: scl_speed_hz debe ser la velocidad real del bus I2C
    ESP_LOGI(LCD_TAG, "Install panel IO (i2c port)");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .scl_speed_hz = 0,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS,
        .dc_bit_offset = 6,
    };
    if (detected_addr != -1)
        io_config.dev_addr = detected_addr;

    esp_err_t ret = esp_lcd_new_panel_io_i2c(I2C_BUS_PORT, &io_config, &io_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LCD_TAG, "esp_lcd_new_panel_io_i2c failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(LCD_TAG, "Panel IO creado correctamente");

    /* Crear driver SSD1306 */
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {.height = EXAMPLE_LCD_V_RES};
    panel_config.vendor_config = &ssd1306_config;

    ret = esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LCD_TAG, "esp_lcd_new_panel_ssd1306 failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(LCD_TAG, "SSD1306 driver creado correctamente");

    if (EXAMPLE_PIN_NUM_RST >= 0)
    {
        ESP_LOGI(LCD_TAG, "Reset físico del panel (gpio %d)", EXAMPLE_PIN_NUM_RST);
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    }
    else
    {
        ESP_LOGI(LCD_TAG, "No hay pin RST definido, se omite reset físico");
    }

    ESP_LOGI(LCD_TAG, "Inicializando panel (init + display on)");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI(LCD_TAG, "Panel inicializado correctamente");

    /* marcar listo y (si mqtt ya conectado) publicar LCD ON en device/lcd */
    lcd_ready = true;

    // Inicializar Arduino core para usar Wire/SparkFun
    initArduino(); // función disponible si agregaste arduino-esp32 como componente
    // Inicializar Wire con los pines ya definidos
    Wire.begin(EXAMPLE_PIN_NUM_SDA, EXAMPLE_PIN_NUM_SCL);

    // Inicializar BNO080 (SparkFun)
    ESP_LOGI(TAG, "Iniciando BNO08x (SparkFun)...");
    if (myIMU.begin() == true)
    {
        ESP_LOGI(TAG, "BNO08x iniciado OK (SparkFun). Habilitando rotation vector.");
        bno_present = true;
        // Habilitar feature rotation vector (fusionado)
        myIMU.enableRotationVector();
        // opcional: establecer tasa de salida en el driver si la librería lo permite
        // myIMU.setFeatureFrequency(SH2_ROTATION_VECTOR, 50); // ejemplo
    }
    else
    {
        ESP_LOGW(TAG, "No se pudo iniciar BNO08x (SparkFun). Verificar conexión I2C y alimentación.");
        bno_present = false;
    }

    // Si BNO no está, intentar detectar/init MPU6050 como fallback
    if (!bno_present)
    {
        if (mpu_detect_and_init())
        {
            mpu_present = true;
            ESP_LOGI(TAG, "MPU6050 inicializado como fallback");
        }
        else
        {
            mpu_present = false;
            ESP_LOGW(TAG, "MPU6050 no detectado en arranque");
        }
    }

    if (mqtt_client)
    {
        int id = esp_mqtt_client_publish(mqtt_client, "display/lcd", "ON", 0, 1, 1);
        ESP_LOGI(TAG, "Publicado estado LCD 'ON' en 'display/lcd' (retained), msg_id=%d", id);
    }
    else
    {
        ESP_LOGI(TAG, "LCD listo pero mqtt_client no inicializado aún; se publicará al conectar");
    }

    // Initialize LVGL
    ESP_LOGI(LCD_TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {.swap_xy = false, .mirror_x = true, .mirror_y = true}};
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    // arrancar tarea que publica sensores cada PUBLISH_PERIOD_MS (serial+MQTT si conectado)
    xTaskCreate(sensor_publish_task, "sensor_publish_task", 4096, NULL, 5, NULL);
    xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 4096, disp, 6, NULL);

    /* Conectar a la red */
    ESP_LOGI(LCD_TAG, "Connecting to network (example_connect) after display init");
    ESP_ERROR_CHECK(example_connect());

    /* Iniciar MQTT y LVGL UI */
    mqtt5_app_start();
}