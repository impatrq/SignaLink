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
#define I2C_MASTER_FREQ_HZ   400000

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

static const char *TAG = "MPU6050";

// Estructura para almacenar los offsets de calibración
typedef struct {
    float accel_offset_x;
    float accel_offset_y;
    float accel_offset_z;
    float gyro_offset_x;
    float gyro_offset_y;
    float gyro_offset_z;
} mpu6050_calibration_t;

mpu6050_calibration_t calibration = {0};

/**
 * @brief Inicializa la interfaz I2C.
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
 */
static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    // Directamente retornamos el resultado de la función del driver I2C
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_FREQ_HZ / 100);
}

/**
 * @brief Lee múltiples bytes de un registro del MPU6050.
 */
static esp_err_t mpu6050_register_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len) {
    // Directamente retornamos el resultado de la función del driver I2C
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg_addr, 1, data, len, I2C_MASTER_FREQ_HZ / 100);
}

/**
 * @brief Inicializa el MPU6050.
 */
void mpu6050_init(void) {
    // Usamos ESP_ERROR_CHECK para manejar los posibles errores de escritura
    ESP_ERROR_CHECK(mpu6050_register_write_byte(PWR_MGMT_1, 0x00)); // Salir del modo sleep
    vTaskDelay(pdMS_TO_TICKS(100)); // Esperar a que el sensor se active
    ESP_ERROR_CHECK(mpu6050_register_write_byte(GYRO_CONFIG, 0x00)); // Configurar rango del giroscopio (+/- 250 deg/s)
    ESP_ERROR_CHECK(mpu6050_register_write_byte(ACCEL_CONFIG, 0x00)); // Configurar rango del acelerómetro (+/- 2g)
    ESP_LOGI(TAG, "MPU6050 initialized");
}

/**
 * @brief Lee los datos crudos de un registro del MPU6050.
 * Retorna el valor crudo de 16 bits. En caso de error de lectura I2C, retorna 0.
 */
int16_t read_raw_data(uint8_t register_addr) {
    uint8_t data[2];
    // Verificamos si la lectura I2C fue exitosa
    esp_err_t err = mpu6050_register_read_bytes(register_addr, data, 2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading raw data from register 0x%02X: %s", register_addr, esp_err_to_name(err));
        return 0; // Retornar 0 en caso de error (podrías manejarlo de otra forma si es necesario)
    }
    int16_t value = (data[0] << 8) | data[1];
    return value;
}

/**
 * @brief Calibra el MPU6050.
 */
void calibrate_mpu6050(int samples) {
    ESP_LOGI(TAG, "Starting MPU6050 calibration... Keep the sensor still!");

    long accel_x_sum = 0;
    long accel_y_sum = 0;
    long accel_z_sum = 0;
    long gyro_x_sum = 0;
    long gyro_y_sum = 0;
    long gyro_z_sum = 0;

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
    // Asumimos que en reposo, la gravedad actúa solo en el eje Z (aproximadamente 1g = 16384 LSB para el rango +/- 2g)
    calibration.accel_offset_z = ((float)accel_z_sum / samples) - 16384.0;
    calibration.gyro_offset_x = (float)gyro_x_sum / samples;
    calibration.gyro_offset_y = (float)gyro_y_sum / samples;
    calibration.gyro_offset_z = (float)gyro_z_sum / samples;

    ESP_LOGI(TAG, "Calibration complete!");
    ESP_LOGI(TAG, "Accelerometer offsets: X=%.2f, Y=%.2f, Z=%.2f", calibration.accel_offset_x, calibration.accel_offset_y, calibration.accel_offset_z);
    ESP_LOGI(TAG, "Gyroscope offsets: X=%.2f, Y=%.2f, Z=%.2f", calibration.gyro_offset_x, calibration.gyro_offset_y, calibration.gyro_offset_z);
}

/**
 * @brief Calcula los ángulos Euler (Roll y Pitch) a partir de los datos del acelerómetro.
 */
void calculate_euler(float accel_x, float accel_y, float accel_z, float *roll, float *pitch) {
    *roll = atan2(accel_y, accel_z) * (180.0 / M_PI);
    *pitch = atan2(-accel_x, sqrt(accel_y * accel_y + accel_z * accel_z)) * (180.0 / M_PI);
}

/**
 * @brief Obtiene los datos del sensor (acelerómetro, giroscopio, Roll, Pitch).
 */
void get_sensor_data(float *accel_x, float *accel_y, float *accel_z,
                     float *gyro_x, float *gyro_y, float *gyro_z,
                     float *roll, float *pitch) {

    int16_t raw_accel_x = read_raw_data(ACCEL_XOUT_H);
    int16_t raw_accel_y = read_raw_data(ACCEL_YOUT_H);
    int16_t raw_accel_z = read_raw_data(ACCEL_ZOUT_H);
    int16_t raw_gyro_x = read_raw_data(GYRO_XOUT_H);
    int16_t raw_gyro_y = read_raw_data(GYRO_YOUT_H);
    int16_t raw_gyro_z = read_raw_data(GYRO_ZOUT_H);

    // Aplicar offsets y escalar según el rango configurado (2g para acelerómetro, 250 deg/s para giroscopio)
    *accel_x = ((float)raw_accel_x - calibration.accel_offset_x) / 16384.0;
    *accel_y = ((float)raw_accel_y - calibration.accel_offset_y) / 16384.0;
    *accel_z = ((float)raw_accel_z - calibration.accel_offset_z) / 16384.0;
    *gyro_x = ((float)raw_gyro_x - calibration.gyro_offset_x) / 131.0;
    *gyro_y = ((float)raw_gyro_y - calibration.gyro_offset_y) / 131.0;
    *gyro_z = ((float)raw_gyro_z - calibration.gyro_offset_z) / 131.0;

    calculate_euler(*accel_x, *accel_y, *accel_z, roll, pitch);
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing I2C...");
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    ESP_LOGI(TAG, "Initializing MPU6050...");
    mpu6050_init();

    ESP_LOGI(TAG, "Starting calibration...");
    calibrate_mpu6050(500); // Calibrar con 500 muestras

    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float roll, pitch;
    float yaw = 0; // El Yaw no se obtiene directamente del acelerómetro, requiere integración del giroscopio o un magnetómetro.

    while (1) {
        get_sensor_data(&accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z, &roll, &pitch);

        // Para una estimación simple del Yaw, podrías integrar la velocidad angular del giroscopio en Z
        // Sin embargo, esto acumulará error con el tiempo (drift).
        // Una solución más robusta implica un filtro (como un filtro de Kalman o complementario)
        // y posiblemente un magnetómetro para corregir el drift.
        // Por ahora, solo mostramos el valor del giroscopio en Z como una indicación de movimiento rotacional.
        yaw = gyro_z; // Usamos el giroscopio en Z como una representación básica del Yaw

        printf("Alabeo= %.2f, Cabeceo= %.2f, Guiñada= %.2f\n", roll, pitch, yaw);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo antes de la siguiente lectura
    }
}
