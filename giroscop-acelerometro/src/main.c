#include <stdio.h>
#include <esp_log.h>
#include <driver/i2c.h>
#include <math.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Definiciones de pines I2C para ESP32-C3 Super Mini
#define I2C_MASTER_SDA_IO    6
#define I2C_MASTER_SCL_IO    7
#define I2C_MASTER_NUM       0
#define I2C_MASTER_FREQ_HZ   400000 // 400 KHz

// Direcciones y registros del MPU6050
#define MPU6050_ADDR         0x68
#define PWR_MGMT_1           0x6B
#define ACCEL_XOUT_H         0x3B
#define ACCEL_YOUT_H         0x3D
#define ACCEL_ZOUT_H         0x3F
#define GYRO_XOUT_H          0x43
#define GYRO_YOUT_H          0x45
#define GYRO_ZOUT_H          0x47
#define GYRO_CONFIG          0x1B
#define ACCEL_CONFIG         0x1C

static const char *TAG = "MPU6050"; // Etiqueta para mensajes de log

// Estructura para almacenar los offsets de calibración
typedef struct {
    float accel_offset_x; // Offset del acelerómetro en X
    float accel_offset_y; // Offset del acelerómetro en Y
    float accel_offset_z; // Offset del acelerómetro en Z
    float gyro_offset_x;  // Offset del giroscopio en X
    float gyro_offset_y;  // Offset del giroscopio en Y
    float gyro_offset_z;  // Offset del giroscopio en Z
} mpu6050_calibration_t;

mpu6050_calibration_t calibration = {0}; // Variable global de calibración

// Variables globales para la integración del Yaw
static int64_t last_time_us = 0; // Marca de tiempo de la última lectura en microsegundos
static float current_yaw = 0.0;  // Ángulo de guiñada (Yaw) integrado actual

// Umbrales para mensajes de movimiento de la mano (ajustar según necesidad)
#define THRESHOLD_CENTERED_RANGE 30.0f // Rango para considerar la mano centrada (ej. +/- 30 grados de 0/360 para X, y +/- 30 grados de 180 para Y)
#define THRESHOLD_X_RIGHT 30.0f    // Inclinación derecha de la mano (Eje X)
#define THRESHOLD_X_LEFT 330.0f     // Inclinación izquierda de la mano (Eje X)
#define THRESHOLD_Y_FORWARD 45.0f   // Mano levantada hacia adelante (Eje Y)
#define THRESHOLD_Y_BACKWARD 315.0f // Mano bajada hacia atrás (Eje Y)
#define THRESHOLD_Z_90 90.0f        // Rotación en Z a 90 grados
#define THRESHOLD_Z_180 180.0f      // Rotación en Z a 180 grados
#define THRESHOLD_Z_270 270.0f      // Rotación en Z a 270 grados

// Banderas para controlar los mensajes (evitar repetición)
static bool msg_hand_centered_triggered = false; // Nueva bandera para el estado centrado
static bool msg_x_right_triggered = false;
static bool msg_x_left_triggered = false;
static bool msg_y_forward_triggered = false;
static bool msg_y_backward_triggered = false;
static bool msg_z_90_triggered = false;
static bool msg_z_180_triggered = false;
static bool msg_z_270_triggered = false;


/**
 * @brief Inicializa la interfaz I2C.
 * @return ESP_OK si es exitoso, código de error en caso contrario.
 */
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/**
 * @brief Escribe un byte en un registro del MPU6050.
 * @param reg_addr Dirección del registro.
 * @param data Byte de datos a escribir.
 * @return ESP_OK si es exitoso.
 */
static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_FREQ_HZ / 100);
}

/**
 * @brief Lee múltiples bytes de un registro del MPU6050.
 * @param reg_addr Dirección inicial del registro.
 * @param data Puntero al buffer para almacenar los datos.
 * @param len Número de bytes a leer.
 * @return ESP_OK si es exitoso.
 */
static esp_err_t mpu6050_register_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg_addr, 1, data, len, I2C_MASTER_FREQ_HZ / 100);
}

/**
 * @brief Inicializa el sensor MPU6050.
 * Sale del modo de suspensión y configura los rangos.
 */
void mpu6050_init(void) {
    ESP_ERROR_CHECK(mpu6050_register_write_byte(PWR_MGMT_1, 0x00)); // Despertar MPU6050
    vTaskDelay(pdMS_TO_TICKS(100)); // Esperar encendido
    ESP_ERROR_CHECK(mpu6050_register_write_byte(GYRO_CONFIG, 0x00)); // Rango giroscopio +/- 250 deg/s
    ESP_ERROR_CHECK(mpu6050_register_write_byte(ACCEL_CONFIG, 0x00)); // Rango acelerómetro +/- 2g
    ESP_LOGI(TAG, "MPU6050 inicializado");
}

/**
 * @brief Lee datos crudos de 16 bits de un registro del MPU6050.
 * @param register_addr Dirección del byte alto del dato.
 * @return Valor crudo de 16 bits, o 0 en caso de error.
 */
int16_t read_raw_data(uint8_t register_addr) {
    uint8_t data[2];
    esp_err_t err = mpu6050_register_read_bytes(register_addr, data, 2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error leyendo datos crudos del registro 0x%02X: %s", register_addr, esp_err_to_name(err));
        return 0;
    }
    return (data[0] << 8) | data[1]; // Combinar bytes
}

/**
 * @brief Calibra el sensor MPU6050.
 * Toma muestras para calcular los offsets del acelerómetro y giroscopio.
 * @param samples Número de muestras para la calibración.
 */
void calibrate_mpu6050(int samples) {
    ESP_LOGI(TAG, "Iniciando calibracion del MPU6050... ¡Mantenga el sensor quieto!");

    long accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    long gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;

    for (int i = 0; i < samples; i++) {
        accel_x_sum += read_raw_data(ACCEL_XOUT_H);
        accel_y_sum += read_raw_data(ACCEL_YOUT_H);
        accel_z_sum += read_raw_data(ACCEL_ZOUT_H);
        gyro_x_sum += read_raw_data(GYRO_XOUT_H);
        gyro_y_sum += read_raw_data(GYRO_YOUT_H);
        gyro_z_sum += read_raw_data(GYRO_ZOUT_H);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    calibration.accel_offset_x = (float)accel_x_sum / samples;
    calibration.accel_offset_y = (float)accel_y_sum / samples;
    calibration.accel_offset_z = ((float)accel_z_sum / samples) - 16384.0; // Restar 1g para Z
    calibration.gyro_offset_x = (float)gyro_x_sum / samples;
    calibration.gyro_offset_y = (float)gyro_y_sum / samples;
    calibration.gyro_offset_z = (float)gyro_z_sum / samples;

    ESP_LOGI(TAG, "Calibracion completa!");
    ESP_LOGI(TAG, "Offsets Acelerometro: X=%.2f, Y=%.2f, Z=%.2f", calibration.accel_offset_x, calibration.accel_offset_y, calibration.accel_offset_z);
    ESP_LOGI(TAG, "Offsets Giroscopio: X=%.2f, Y=%.2f, Z=%.2f", calibration.gyro_offset_x, calibration.gyro_offset_y, calibration.gyro_offset_z);
}

/**
 * @brief Calcula ángulos Euler (Roll y Pitch) a partir de datos del acelerómetro.
 * Los ángulos se ajustan para representar Alabeo (Roll) y Cabeceo (Pitch) de la mano.
 * @param accel_x Aceleración en X (en g's).
 * @param accel_y Aceleración en Y (en g's).
 * @param accel_z Aceleración en Z (en g's).
 * @param roll Puntero para almacenar el ángulo de alabeo (Roll) de la mano.
 * @param pitch Puntero para almacenar el ángulo de cabeceo (Pitch) de la mano.
 */
void calculate_euler(float accel_x, float accel_y, float accel_z, float *roll, float *pitch) {
    // Alabeo (Roll) de la mano: rotación alrededor del eje Y del MPU (afecta accel_x y accel_z)
    *roll = atan2(accel_x, accel_z) * (180.0 / M_PI);

    // Cabeceo (Pitch) de la mano: rotación alrededor del eje X del MPU (afecta accel_y y accel_z)
    // Ajuste para que 0° sea mano plana y 180° sea mano invertida.
    float temp_pitch = atan2(accel_y, accel_z) * (180.0 / M_PI);
    *pitch = fmod(temp_pitch + 180.0, 360.0); // Sumar 180 y tomar el módulo 360
    if (*pitch < 0) { // Asegurar que el resultado sea positivo
        *pitch += 360.0;
    }

    // Normalizar Roll a 0-360 grados si es negativo
    if (*roll < 0) {
        *roll += 360.0;
    }
}

/**
 * @brief Obtiene datos del sensor, aplica calibración y calcula ángulos de orientación.
 * @param accel_x_g Puntero para aceleración X (en g's).
 * @param accel_y_g Puntero para aceleración Y (en g's).
 * @param accel_z_g Puntero para aceleración Z (en g's).
 * @param gyro_x_dps Puntero para velocidad angular X (deg/s).
 * @param gyro_y_dps Puntero para velocidad angular Y (deg/s).
 * @param gyro_z_dps Puntero para velocidad angular Z (deg/s).
 * @param angle_x_deg Puntero para ángulo X (Roll de la mano).
 * @param angle_y_deg Puntero para ángulo Y (Pitch de la mano).
 * @param angle_z_deg Puntero para ángulo Z (Yaw de la mano).
 */
void get_sensor_data(float *accel_x_g, float *accel_y_g, float *accel_z_g,
                     float *gyro_x_dps, float *gyro_y_dps, float *gyro_z_dps,
                     float *angle_x_deg, float *angle_y_deg, float *angle_z_deg) {

    int16_t raw_accel_x = read_raw_data(ACCEL_XOUT_H);
    int16_t raw_accel_y = read_raw_data(ACCEL_YOUT_H);
    int16_t raw_accel_z = read_raw_data(ACCEL_ZOUT_H);
    int16_t raw_gyro_x = read_raw_data(GYRO_XOUT_H);
    int16_t raw_gyro_y = read_raw_data(GYRO_YOUT_H);
    int16_t raw_gyro_z = read_raw_data(GYRO_ZOUT_H);

    // Aplicar offsets y escalar datos del acelerómetro a 'g's (sensibilidad 16384 LSB/g)
    *accel_x_g = ((float)raw_accel_x - calibration.accel_offset_x) / 16384.0;
    *accel_y_g = ((float)raw_accel_y - calibration.accel_offset_y) / 16384.0;
    *accel_z_g = ((float)raw_accel_z - calibration.accel_offset_z) / 16384.0;

    // Aplicar offsets y escalar datos del giroscopio a deg/s (sensibilidad 131 LSB/(deg/s))
    *gyro_x_dps = ((float)raw_gyro_x - calibration.gyro_offset_x) / 131.0;
    *gyro_y_dps = ((float)raw_gyro_y - calibration.gyro_offset_y) / 131.0;
    *gyro_z_dps = ((float)raw_gyro_z - calibration.gyro_offset_z) / 131.0;

    // Calcular Roll y Pitch de la mano
    calculate_euler(*accel_x_g, *accel_y_g, *accel_z_g, angle_x_deg, angle_y_deg);

    // Integración simple del Yaw (ángulo Z) desde el giroscopio Z. Acumula error.
    int64_t current_time_us = esp_timer_get_time();
    float dt_s = (float)(current_time_us - last_time_us) / 1000000.0; // Diferencia de tiempo en segundos
    current_yaw += (*gyro_z_dps * dt_s); // Actualizar Yaw
    last_time_us = current_time_us; // Actualizar última marca de tiempo

    // Normalizar ángulo Yaw (0-360 grados)
    if (current_yaw > 360.0) {
        current_yaw -= 360.0;
    } else if (current_yaw < 0.0) {
        current_yaw += 360.0;
    }
    *angle_z_deg = current_yaw;
}

/**
 * @brief Función principal de la aplicación.
 * Inicializa I2C y MPU6050, calibra, lee y imprime ángulos X, Y, Z.
 */
void app_main(void) {
    ESP_LOGI(TAG, "Inicializando I2C...");
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C inicializado correctamente");

    ESP_LOGI(TAG, "Inicializando MPU6050...");
    mpu6050_init();

    ESP_LOGI(TAG, "Iniciando calibracion...");
    calibrate_mpu6050(500); // Calibrar con 500 muestras

    float accel_x_g, accel_y_g, accel_z_g;
    float gyro_x_dps, gyro_y_dps, gyro_z_dps;
    float angle_x_deg, angle_y_deg, angle_z_deg; // X=Roll de la mano, Y=Pitch de la mano, Z=Yaw de la mano

    last_time_us = esp_timer_get_time(); // Inicializar marca de tiempo

    while (1) {
        get_sensor_data(&accel_x_g, &accel_y_g, &accel_z_g,
                        &gyro_x_dps, &gyro_y_dps, &gyro_z_dps,
                        &angle_x_deg, &angle_y_deg, &angle_z_deg);

        // Imprimir ángulos X, Y, Z (Roll, Pitch, Yaw) como números enteros
        printf("Eje X %.0f°, Eje Y %.0f°, Eje Z %.0f°\n",
               angle_x_deg, angle_y_deg, angle_z_deg);

        // Lógica para el estado "Mano Centrada"
        // Se considera centrada si X está cerca de 0/360 Y Y está cerca de 180.
        bool is_x_centered = (angle_x_deg <= THRESHOLD_CENTERED_RANGE || angle_x_deg >= (360.0f - THRESHOLD_CENTERED_RANGE));
        bool is_y_centered = (angle_y_deg >= (180.0f - THRESHOLD_CENTERED_RANGE) && angle_y_deg <= (180.0f + THRESHOLD_CENTERED_RANGE));

        if (is_x_centered && is_y_centered) {
            if (!msg_hand_centered_triggered) {
                printf("¡MANO CENTRADA!\n");
                msg_hand_centered_triggered = true;
                // Resetear otras banderas cuando la mano está centrada para permitir nuevas detecciones
                msg_x_right_triggered = false;
                msg_x_left_triggered = false;
                msg_y_forward_triggered = false;
                msg_y_backward_triggered = false;
            }
        } else {
            msg_hand_centered_triggered = false; // Resetear bandera de centrado si la mano se mueve
        }


        // Lógica para el Eje X (Roll de la mano) - Inclinación lateral
        // Solo activar si no está en estado centrado para evitar mensajes duplicados
        if (!msg_hand_centered_triggered) {
            if (angle_x_deg >= THRESHOLD_X_RIGHT && angle_x_deg < 180.0f) {
                if (!msg_x_right_triggered) {
                    printf("¡Mano inclinada a la IZQUIERDA! (Eje X > %.0f°)\n", THRESHOLD_X_RIGHT);
                    msg_x_right_triggered = true;
                }
            } else {
                msg_x_right_triggered = false;
            }

            if (angle_x_deg <= THRESHOLD_X_LEFT && angle_x_deg > 180.0f) {
                if (!msg_x_left_triggered) {
                    printf("¡Mano inclinada a la DERECHA! (Eje X < %.0f°)\n", 360.0f - THRESHOLD_X_LEFT);
                    msg_x_left_triggered = true;
                }
            } else {
                msg_x_left_triggered = false;
            }
        }

        // Lógica para el Eje Y (Pitch de la mano) - Levantamiento/Bajada de la mano
        // Solo activar si no está en estado centrado
        if (!msg_hand_centered_triggered) {
            if (angle_y_deg >= THRESHOLD_Y_FORWARD && angle_y_deg < 180.0f) {
                if (!msg_y_forward_triggered) {
                    printf("¡Mano inclinada hacia ADELANTE! (Eje Y > %.0f°)\n", THRESHOLD_Y_FORWARD);
                    msg_y_forward_triggered = true;
                }
            } else {
                msg_y_forward_triggered = false;
            }

            if (angle_y_deg <= THRESHOLD_Y_BACKWARD && angle_y_deg > 180.0f) {
                if (!msg_y_backward_triggered) {
                    printf("¡Mano inclinada hacia ATRÁS! (Eje Y < %.0f°)\n", 360.0f - THRESHOLD_Y_BACKWARD);
                    msg_y_backward_triggered = true;
                }
            } else {
                msg_y_backward_triggered = false;
            }
        }

        // Lógica para el Eje Z (Yaw de la mano) - Rotación de la muñeca
        // Esta lógica no depende del estado centrado de X e Y.
        if (angle_z_deg >= THRESHOLD_Z_90 && angle_z_deg < THRESHOLD_Z_180) {
            if (!msg_z_90_triggered) {
                printf("¡Rotación en Eje Z > %.0f°!\n", THRESHOLD_Z_90);
                msg_z_90_triggered = true;
            }
        } else {
            msg_z_90_triggered = false;
        }

        if (angle_z_deg >= THRESHOLD_Z_180 && angle_z_deg < THRESHOLD_Z_270) {
            if (!msg_z_180_triggered) {
                printf("¡Rotación en Eje Z > %.0f°!\n", THRESHOLD_Z_180);
                msg_z_180_triggered = true;
            }
        } else {
            msg_z_180_triggered = false;
        }

        if (angle_z_deg >= THRESHOLD_Z_270) {
            if (!msg_z_270_triggered) {
                printf("¡Rotación en Eje Z > %.0f°!\n", THRESHOLD_Z_270);
                msg_z_270_triggered = true;
            }
        } else if (angle_z_deg < THRESHOLD_Z_270 && msg_z_270_triggered) {
            msg_z_270_triggered = false;
        }

        vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 segundos
    }
}