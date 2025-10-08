#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

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
#include <math.h> 
#include "esp_lcd_panel_ssd1306.h"

/* ---------------------------
   Defines / configuración
--------------------------- */

#ifndef EXAMPLE_PIN_NUM_SDA
#define EXAMPLE_PIN_NUM_SDA 6
#endif
#ifndef EXAMPLE_PIN_NUM_SCL
#define EXAMPLE_PIN_NUM_SCL 7
#endif
#ifndef EXAMPLE_PIN_NUM_RST
#define EXAMPLE_PIN_NUM_RST -1
#endif
#ifndef EXAMPLE_I2C_HW_ADDR
#define EXAMPLE_I2C_HW_ADDR 0x3C
#endif
#ifndef EXAMPLE_LCD_H_RES
#define EXAMPLE_LCD_H_RES 128
#endif
#ifndef EXAMPLE_LCD_V_RES
#define EXAMPLE_LCD_V_RES 32
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
// static float mpu_angle_x = 0.0f, mpu_angle_y = 0.0f, mpu_angle_z = 0.0f; // Ángulos acumulados
static int64_t last_mpu_time = 0;

static const char *TAG = "ManoMQTT";
static const char *LCD_TAG = "example";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle[5];
// static char ultimo_estado_flex[5][20] = {"", "", "", "", ""};

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool lcd_ready = false; /* indica que el LCD fue inicializado */

/* Label único accesible globalmente para actualizar desde MQTT u otros tasks */
static lv_obj_t *g_label = NULL;

/* ---------------------------
   Helpers: actualizar texto del LCD desde otros tasks (ej.: MQTT)
   --------------------------- */
/* Actualiza el único label centrado. Safe para llamar desde cualquier task. */
void lcd_set_text(const char *txt)
{
    if (!txt)
        return;
    if (lvgl_port_lock(0))
    {
        if (g_label)
        {
            lv_label_set_text(g_label, txt);
            /* activar scroll circular para textos largos */
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

/* MPU6050 */
static bool mpu_present = false;
static uint8_t mpu_addr = 0x68; /* dirección 7-bit típica MPU6050 */

/* ---------------------------
   Filtro Kalman para ángulo MPU6050
--------------------------- */
typedef struct {
    float angle; // ángulo estimado
    float bias;  // sesgo estimado
    float rate;  // velocidad angular
    float P[2][2]; // matriz de error
    float Q_angle; // varianza proceso ángulo
    float Q_bias;  // varianza proceso sesgo
    float R_measure; // varianza medición
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

// Filtro Kalman para un eje
float kalman_update(Kalman_t *kalman, float newAngle, float newRate, float dt) {
    // Predicción
    kalman->rate = newRate - kalman->bias;
    kalman->angle += dt * kalman->rate;

    // Actualización de la matriz de error (misma lógica)
    kalman->P[0][0] += dt * (dt * kalman->P[1][1] - kalman->P[0][1] - kalman->P[1][0] + kalman->Q_angle);
    kalman->P[0][1] -= dt * kalman->P[1][1];
    kalman->P[1][0] -= dt * kalman->P[1][1];
    kalman->P[1][1] += kalman->Q_bias * dt;

    // Medición: calculamos la diferencia con corrección por wrapping [-180,180]
    float S = kalman->P[0][0] + kalman->R_measure;
    float K[2];
    K[0] = kalman->P[0][0] / S;
    K[1] = kalman->P[1][0] / S;

    // Residual (nuevo ángulo - estimado). Corregir por paso por +/-180°
    float y = newAngle - kalman->angle;
    if (y > 180.0f)  y -= 360.0f;
    if (y < -180.0f) y += 360.0f;

    // Aplicar corrección
    kalman->angle += K[0] * y;
    kalman->bias  += K[1] * y;

    // Actualizar matriz de error
    float P00_temp = kalman->P[0][0];
    float P01_temp = kalman->P[0][1];

    kalman->P[0][0] -= K[0] * P00_temp;
    kalman->P[0][1] -= K[0] * P01_temp;
    kalman->P[1][0] -= K[1] * P00_temp;
    kalman->P[1][1] -= K[1] * P01_temp;

    // Mantener ángulo en rango -180..+180 para la continuidad interna
    if (kalman->angle > 180.0f)  kalman->angle -= 360.0f;
    if (kalman->angle < -180.0f) kalman->angle += 360.0f;

    return kalman->angle;
}

/* ---------------------------
   Helpers I2C / MPU6050
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

/* Detecta e inicializa MPU6050 (wake up) */
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

/* Lee giroscopio (reg 0x43..0x48), devuelve deg/s aproximados en ints */
static bool mpu6050_read_gyro_deg(int *gx, int *gy, int *gz)
{
    uint8_t buf[6];
    if (mpu6050_read_regs(mpu_addr, 0x43, buf, 6) != ESP_OK)
        return false;
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);
    /* sensibilidad por defecto +/-250 deg/s -> 131 LSB/(deg/s) */
    *gx = (int)(raw_x / 131.0f + (raw_x >= 0 ? 0.5f : -0.5f));
    *gy = (int)(raw_y / 131.0f + (raw_y >= 0 ? 0.5f : -0.5f));
    *gz = (int)(raw_z / 131.0f + (raw_z >= 0 ? 0.5f : -0.5f));
    return true;
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
        // Resistencia fija para normalizar lecturas (~20kΩ)
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
   Tarea periódica: publicar MPU + flex cada 1s
   - Imprime por serial y publica en MQTT si está conectado.
--------------------------- */
// Lee acelerómetro (reg 0x3B..0x40), devuelve valores en g (float)
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

static void sensor_publish_task(void *arg)
{
    ESP_LOGI(TAG, "sensor_publish_task iniciado (cada %d ms)", PUBLISH_PERIOD_MS);
    last_mpu_time = esp_timer_get_time(); // microsegundos

    while (1)
    {
        char estado_flex[5][20];
        for (int i = 0; i < 5; ++i)
        {
            leer_estado_flex(i, (adc_channel_t)(ADC_CHANNEL_0 + i), cali_handle[i], estado_flex[i], sizeof(estado_flex[i]));
        }

        int gx = 0, gy = 0, gz = 0;
        float ax = 0, ay = 0, az = 0;
        
        // La variable mpu_part ya no se necesita, pero la dejamos para no romper la estructura local.
        // char mpu_part[64];

        int64_t now = esp_timer_get_time();
        float delta_t = (now - last_mpu_time) / 1000000.0f; // segundos
        last_mpu_time = now;

        float angle_acc_x = 0, angle_acc_y = 0;
        
        // Variables para los 6 valores de MPU que se publicarán
        int disp_x_entero = 0, disp_x_decimal = 0;
        int disp_y_entero = 0, disp_y_decimal = 0;
        int disp_z_entero = 0, disp_z_decimal = 0;


        if (mpu_present && mpu6050_read_gyro_deg(&gx, &gy, &gz) && mpu6050_read_accel_g(&ax, &ay, &az))
        {
            // Fórmulas estándar (medición del acelerómetro) -> rango: -180..+180
            angle_acc_x = atan2f(ay, az) * 180.0f / M_PI;   // Roll
            angle_acc_y = atan2f(ax, az) * 180.0f / M_PI;   // Pitch

            // NO normalizar aquí a 0..360 — mantener -180..+180 para Kalman
            // Filtro Kalman (gyro en deg/s)
            float kalman_angle_x = kalman_update(&kalmanX, angle_acc_x, gx, delta_t);
            float kalman_angle_y = kalman_update(&kalmanY, angle_acc_y, gy, delta_t);
            static float yaw = 0.0f;  // mantener estado
            yaw += gz * delta_t;      // sumar rotación en grados/seg * segundos
            if (yaw >= 360.0f) yaw -= 360.0f;
            if (yaw < 0.0f) yaw += 360.0f;
            float kalman_angle_z = yaw;

            // Para mostrar/publicar convertimos a 0..360 (visual)
            float disp_x = kalman_angle_x;
            float disp_y = kalman_angle_y;
            float disp_z = kalman_angle_z;

            if (disp_x < 0.0f) disp_x += 360.0f;
            if (disp_y < 0.0f) disp_y += 360.0f;
            if (disp_z < 0.0f) disp_z += 360.0f;

            if (disp_x >= 360.0f) disp_x -= 360.0f;
            if (disp_y >= 360.0f) disp_y -= 360.0f;
            if (disp_z >= 360.0f) disp_z -= 360.0f;
            
            // ==============================================================
            // CÁLCULO DE ENTERO Y DECIMAL PARA PUBLICACIÓN DE DATOS CRUDOS
            // ==============================================================
            disp_x_entero = (int)disp_x;
            disp_x_decimal = (int)(fabsf(disp_x) * 10.0f) % 10;
            
            disp_y_entero = (int)disp_y;
            disp_y_decimal = (int)(fabsf(disp_y) * 10.0f) % 10;
            
            disp_z_entero = (int)disp_z;
            disp_z_decimal = (int)(fabsf(disp_z) * 10.0f) % 10;

            /* BLOQUE ORIGINAL ELIMINADO:
            snprintf(mpu_part, sizeof(mpu_part),
                     "MPU: x=%.1f°, y=%.1f°, z=%.1f° - ",
                     disp_x, disp_y, disp_z);
            */
        }
        else
        {
            /* Si el MPU no está presente, enviamos ceros para los 6 valores */
            // snprintf(mpu_part, sizeof(mpu_part), "MPU6050: N/A - ");
            disp_x_entero = disp_x_decimal = 0;
            disp_y_entero = disp_y_decimal = 0;
            disp_z_entero = disp_z_decimal = 0;
        }

        char mensaje[512];
        
        // ==============================================================
        // NUEVA CONSTRUCCIÓN DEL MENSAJE (11 VALORES CRUDOS SEPARADOS POR COMA)
        // ==============================================================
        snprintf(mensaje, sizeof(mensaje),
                 "%d,%d,"    // MPU X (entero, decimal)
                 "%d,%d,"    // MPU Y (entero, decimal)
                 "%d,%d,"    // MPU Z (entero, decimal)
                 "%s,%s,%s,%s,%s", // 5 Flex Sensores (cadenas de resistencia)
                 disp_x_entero, disp_x_decimal,
                 disp_y_entero, disp_y_decimal,
                 disp_z_entero, disp_z_decimal,
                 estado_flex[0], estado_flex[1], estado_flex[2], estado_flex[3], estado_flex[4]);

        /* BLOQUE ORIGINAL ELIMINADO:
        snprintf(mensaje, sizeof(mensaje),
                 "%sPulgar=%s, Indice=%s, Mayor=%s, Anular=%s, Meñique=%s",
                 mpu_part,
                 estado_flex[0], estado_flex[1], estado_flex[2], estado_flex[3], estado_flex[4]);
        */

        // Log que muestra la salida RAW (cruda) para debug
        ESP_LOGI(TAG, "MQTT RAW DATA: %s", mensaje);

        if (mqtt_client)
        {
            // Tópico correcto: sensors/mpu_flex
            int msg_id = esp_mqtt_client_publish(mqtt_client, "sensors/mpu_flex", mensaje, 0, 1, 0);
            ESP_LOGI(TAG, "Publicado sensors/mpu_flex, msg_id=%d", msg_id);
        }

        vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));
    }
}

/* ---------------------------
   MQTT handlers / start
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
        // Inicializar filtros Kalman en 0 al conectar
        kalman_init(&kalmanX);
        kalman_init(&kalmanY);
        kalman_init(&kalmanZ);

        int msg_id = esp_mqtt_client_subscribe(mqtt_client, "comandos/esp32", 0);
        ESP_LOGI(TAG, "Suscrito a 'comandos/esp32', msg_id=%d", msg_id);

        int msg_id2 = esp_mqtt_client_subscribe(mqtt_client, "display/lcd", 0);
        ESP_LOGI(TAG, "Suscrito a 'display/lcd', msg_id=%d", msg_id2);

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

        /* Copiar topic y payload a cadenas nulas-terminadas */
        {
            char topic_buf[128];
            size_t tlen = event->topic_len < sizeof(topic_buf) - 1 ? event->topic_len : sizeof(topic_buf) - 1;
            memcpy(topic_buf, event->topic, tlen);
            topic_buf[tlen] = '\0';

            size_t dlen = event->data_len;
            char *data_buf = malloc(dlen + 1);
            if (data_buf)
            {
                memcpy(data_buf, event->data, dlen);
                data_buf[dlen] = '\0';

                /* Si el topic es display/lcd mostrar en el LCD (seguro para llamar desde task) */
                if (strcmp(topic_buf, "display/lcd") == 0)
                {
                    ESP_LOGI(TAG, "Actualizar LCD con texto MQTT: %s", data_buf);
                    lcd_set_text(data_buf);
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

    /* IDF v5 mqtt client uses 'credentials' field; set if available */
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

        /* Crear un único label centrado que ocupe todo el ancho.
           Usar scroll circular para permitir leer frases largas. */
        g_label = lv_label_create(scr);
        lv_label_set_long_mode(g_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text(g_label, "Esperando Conexion. . ."); /* texto inicial */
        lv_obj_set_width(g_label, disp->driver->hor_res - 4);  /* ancho completo menos padding */
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
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    /* Silenciar logs por UART (usar solo errores si querés) */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ManoMQTT", ESP_LOG_NONE);
    esp_log_level_set("mqtt_client", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    esp_log_level_set("wifi:sta", ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
    ESP_LOGI(LCD_TAG, "menuconfig: CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306 = y");
#else
    ESP_LOGW(LCD_TAG, "menuconfig: CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306 = n");
#endif
#ifdef CONFIG_EXAMPLE_SSD1306_HEIGHT
    ESP_LOGI(LCD_TAG, "menuconfig: CONFIG_EXAMPLE_SSD1306_HEIGHT = %d", CONFIG_EXAMPLE_SSD1306_HEIGHT);
#endif

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

    /* I2C legacy init (SSD1306 + MPU6050 comparten bus) */
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
        .scl_speed_hz = 0, // Debe ser 0 para usar un bus I2C pre-inicializado
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
        // probar double buffer activado para mejor rendering en LVGL/SSD1306
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        // probar mirror_x/y true (si no funciona probar false/false)
        .rotation = {.swap_xy = false, .mirror_x = true, .mirror_y = true}};
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    // arrancar tarea que publica sensores cada 1s (serial+MQTT si conectado)
    xTaskCreate(sensor_publish_task, "sensor_publish_task", 4096, NULL, 5, NULL);
    xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 4096, disp, 6, NULL);

    /* Conectar a la red (example_connect lee SSID/pass desde menuconfig / provisioning) */
    ESP_LOGI(LCD_TAG, "Connecting to network (example_connect) after display init");
    ESP_ERROR_CHECK(example_connect());

    /* Iniciar MQTT y LVGL UI */
    mqtt5_app_start();
}