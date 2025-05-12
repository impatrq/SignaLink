from machine import I2C, Pin, RTC
from time import sleep_ms
import bluetooth
import json
from math import atan2, sqrt, pi
import struct
import time

# Device specific configuration
MAC_ADDR = "ABCDEF123456"  # PONÉ TU MAC SI QUERÉS
BLE_NAME = f"ESP_32C3_{MAC_ADDR[-8:]}"
DEV_ID = 1

# MPU6050 I2C address and registers
MPU6050_ADDR = 0x68
ACCEL_XOUT_H = 0x3B
ACCEL_YOUT_H = 0x3D
ACCEL_ZOUT_H = 0x3F
GYRO_XOUT_H = 0x43
GYRO_YOUT_H = 0x45
GYRO_ZOUT_H = 0x47
PWR_MGMT_1 = 0x6B

# BLE configs
SENSOR_SERVICE_UUID = bluetooth.UUID('6E400001-B5A3-F393-E0A9-E50E24DCCA9E')
SENSOR_CHAR_UUID = bluetooth.UUID('6E400003-B5A3-F393-E0A9-E50E24DCCA9E')

# Configuration registers
GYRO_CONFIG = 0x1B
ACCEL_CONFIG = 0x1C

# Initialize I2C
sda = Pin(6)
scl = Pin(7)
i2c = I2C(0, scl=Pin(7), sda=Pin(6), freq=400000)

# Initialize RTC
rtc = RTC()

# Calibration offsets
class MPU6050Calibration:
    def __init__(self):
        self.accel_offset_x = 0
        self.accel_offset_y = 0
        self.accel_offset_z = 0
        self.gyro_offset_x = 0
        self.gyro_offset_y = 0
        self.gyro_offset_z = 0

calibration = MPU6050Calibration()

class BLEDevice:
    def __init__(self, name):
        self._ble = bluetooth.BLE()
        self._ble.active(True)
        self._ble.irq(self._ble_irq)
        self._connections = set()
        self._name = name
        self._payload = None
        self._register_services()
        self._advertise()

    def _ble_irq(self, event, data):
        if event == 1:  # _IRQ_CENTRAL_CONNECT
            conn_handle, _, _ = data
            self._connections.add(conn_handle)
            print(f'Client connected to {self._name} ({MAC_ADDR})')
        elif event == 2:  # _IRQ_CENTRAL_DISCONNECT
            conn_handle, _, _ = data
            self._connections.remove(conn_handle)
            print('Client disconnected')
            self._advertise()

    def _register_services(self):
        service = (
            SENSOR_SERVICE_UUID,
            (
                (SENSOR_CHAR_UUID, bluetooth.FLAG_NOTIFY),
            ),
        )
        services = (service,)
        ((self._handle,),) = self._ble.gatts_register_services(services)

    def _advertise(self, interval_us=100000):
        payload = bytearray()
        payload.extend(struct.pack('BBB', 0x02, 0x01, 0x06))

        name_bytes = self._name.encode()
        name_length = len(name_bytes)
        payload.extend(struct.pack('BB', name_length + 1, 0x09))
        payload.extend(name_bytes)

        self._ble.gap_advertise(interval_us, adv_data=payload)

    def send(self, data):
        if self._connections:
            data['device_id'] = DEV_ID
            encoded_data = json.dumps(data).encode()
            for conn_handle in self._connections:
                self._ble.gatts_notify(conn_handle, self._handle, encoded_data)

    def is_connected(self):
        return len(self._connections) > 0

def mpu6050_init():
    i2c.writeto_mem(MPU6050_ADDR, PWR_MGMT_1, b'\x00')
    sleep_ms(100)
    i2c.writeto_mem(MPU6050_ADDR, GYRO_CONFIG, b'\x00')
    i2c.writeto_mem(MPU6050_ADDR, ACCEL_CONFIG, b'\x00')
    print("MPU6050 initialized")

def read_raw_data(register):
    high = i2c.readfrom_mem(MPU6050_ADDR, register, 1)[0]
    low = i2c.readfrom_mem(MPU6050_ADDR, register + 1, 1)[0]
    value = (high << 8) | low
    if value > 32768:
        value -= 65536
    return value

def calibrate_mpu6050(samples=500):
    print("Starting MPU6050 calibration... Keep the sensor still!")

    accel_x_sum = 0
    accel_y_sum = 0
    accel_z_sum = 0
    gyro_x_sum = 0
    gyro_y_sum = 0
    gyro_z_sum = 0

    for _ in range(samples):
        accel_x_sum += read_raw_data(ACCEL_XOUT_H)
        accel_y_sum += read_raw_data(ACCEL_YOUT_H)
        accel_z_sum += read_raw_data(ACCEL_ZOUT_H)
        gyro_x_sum += read_raw_data(GYRO_XOUT_H)
        gyro_y_sum += read_raw_data(GYRO_YOUT_H)
        gyro_z_sum += read_raw_data(GYRO_ZOUT_H)
        sleep_ms(2)

    calibration.accel_offset_x = accel_x_sum / samples
    calibration.accel_offset_y = accel_y_sum / samples
    calibration.accel_offset_z = (accel_z_sum / samples) - 16384
    calibration.gyro_offset_x = gyro_x_sum / samples
    calibration.gyro_offset_y = gyro_y_sum / samples
    calibration.gyro_offset_z = gyro_z_sum / samples

    print("Calibration complete!")
    print(f"Accelerometer offsets: X={calibration.accel_offset_x:.2f}, "
          f"Y={calibration.accel_offset_y:.2f}, Z={calibration.accel_offset_z:.2f}")
    print(f"Gyroscope offsets: X={calibration.gyro_offset_x:.2f}, "
          f"Y={calibration.gyro_offset_y:.2f}, Z={calibration.gyro_offset_z:.2f}")

def calculate_euler(accel_x, accel_y, accel_z):
    roll = atan2(accel_y, accel_z) * (180 / pi)
    pitch = atan2(-accel_x, sqrt(accel_y**2 + accel_z**2)) * (180 / pi)
    return roll, pitch

def get_sensor_data():
    raw_accel_x = read_raw_data(ACCEL_XOUT_H)
    raw_accel_y = read_raw_data(ACCEL_YOUT_H)
    raw_accel_z = read_raw_data(ACCEL_ZOUT_H)
    raw_gyro_x = read_raw_data(GYRO_XOUT_H)
    raw_gyro_y = read_raw_data(GYRO_YOUT_H)
    raw_gyro_z = read_raw_data(GYRO_ZOUT_H)

    accel_x = (raw_accel_x - calibration.accel_offset_x) / 16384.0
    accel_y = (raw_accel_y - calibration.accel_offset_y) / 16384.0
    accel_z = (raw_accel_z - calibration.accel_offset_z) / 16384.0
    gyro_x = (raw_gyro_x - calibration.gyro_offset_x) / 131.0
    gyro_y = (raw_gyro_y - calibration.gyro_offset_y) / 131.0
    gyro_z = (raw_gyro_z - calibration.gyro_offset_z) / 131.0

    roll, pitch = calculate_euler(accel_x, accel_y, accel_z)
    return accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, roll, pitch

def get_timestamp():
    datetime = rtc.datetime()
    return "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}".format(
        datetime[0], datetime[1], datetime[2],
        datetime[4], datetime[5], datetime[6]
    )

def main():
    print("Initializing MPU6050...")
    mpu6050_init()

    print("Starting calibration...")
    calibrate_mpu6050()

    ble = BLEDevice(BLE_NAME)
    print(f"Device {BLE_NAME} is ready for connection...")

    try:
        while True:
            accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, roll, pitch = get_sensor_data()

            yaw = gyro_z

            print(f"angleX= {roll:.2f}    angleY= {pitch:.2f}    angleZ= {yaw:.2f}")

            time.sleep(3)

    except KeyboardInterrupt:
        print("\nData collection stopped by user.")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        ble._ble.active(False)

if __name__ == "__main__":
    main()