import asyncio
from bleak import BleakClient, BleakScanner
from gpiozero import LED
import sys

# --- Configuración ---
# Reemplaza con la dirección MAC de tu ESP32 (la verás en el Monitor Serie del ESP32 al iniciar, o escaneando)
# O puedes buscar por nombre "SignaLink_ESP32"
DEVICE_ADDRESS = "E8:06:90:65:C6:3A" 
DEVICE_NAME = "Mi_ESP32_SignaLink"

# UUIDs (deben coincidir con los del ESP32)
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

led = LED(17) 

# --- Callback para Notificaciones ---
def notification_handler(sender, data):
    """Se ejecuta cuando llega una notificación del ESP32."""
    message = data.decode('utf-8')
    print(f"Recibido: {message}")
    if message == "ON":
        led.on()
        print("LED Encendido")
    elif message == "OFF":
        led.off()
        print("LED Apagado")

async def run_ble_client():
    print(f"Buscando dispositivo: {DEVICE_NAME} ({DEVICE_ADDRESS})...")
    device = None

    # Intentar encontrar por dirección si se especificó
    if DEVICE_ADDRESS != "E8:06:90:65:C6:38":
         device = await BleakScanner.find_device_by_address(DEVICE_ADDRESS, timeout=20.0)

    # Si no se encontró por dirección o no se especificó, buscar por nombre
    if not device:
        print(f"No se encontró por dirección, buscando por nombre {DEVICE_NAME}...")
        devices = await BleakScanner.discover(timeout=10.0)
        for d in devices:
            if d.name == DEVICE_NAME:
                device = d
                print(f"Dispositivo encontrado por nombre: {device.address}")
                break

    if not device:
        print(f"No se pudo encontrar el dispositivo {DEVICE_NAME}. Asegúrate de que esté encendido y anunciando.")
        sys.exit(1) # Salir si no se encuentra

    print(f"Conectando a {device.name} ({device.address})...")
    async with BleakClient(device.address) as client:
        if client.is_connected:
            print("¡Conectado!")

            # Suscribirse a notificaciones
            try:
                await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                print(f"Suscrito a notificaciones de {CHARACTERISTIC_UUID}")

                # Mantener la conexión abierta y esperar notificaciones
                # Puedes añadir lógica aquí para detener el script si es necesario
                while client.is_connected:
                    await asyncio.sleep(1) # Espera para mantener el script corriendo

            except Exception as e:
                print(f"Error durante la conexión/suscripción: {e}")
            finally:
                 if client.is_connected:
                    try:
                       await client.stop_notify(CHARACTERISTIC_UUID)
                    except Exception as e:
                       print(f"Error al detener notificaciones: {e}")
        else:
            print("Fallo al conectar.")

if __name__ == "__main__":
    try:
        asyncio.run(run_ble_client())
    except KeyboardInterrupt:
        print("\nPrograma detenido por el usuario.")
    finally:
        led.off() # Asegurarse de apagar el LED al salir
        print("LED apagado.")
