import asyncio
from bleak import BleakClient, BleakScanner
from gpiozero import LED
import sys

# --- Configuración ---
DEVICE_ADDRESS = "E8:06:90:65:C6:38" 
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
    async with BleakScanner() as scanner:
        device = None

        # Intentar buscar por dirección primero (opcional, puedes comentar estas líneas si solo quieres probar por nombre)
        # device_by_address = await scanner.find_device_by_address(DEVICE_ADDRESS, timeout=20.0)
        # if device_by_address:
        #     device = device_by_address
        #     print(f"Dispositivo encontrado por dirección: {device.name} ({device.address})")
        # else:
        device_by_address = await scanner.find_device_by_address(DEVICE_ADDRESS, timeout=20.0)
                if device_by_address:
                    device = device_by_address
                    print(f"Dispositivo encontrado por dirección: {device.name} ({device.address})")
                else:
                    print(f"No se encontró dispositivo con dirección {DEVICE_ADDRESS}, buscando por nombre '{DEVICE_NAME}'...")
                    devices = await scanner.discover(timeout=10.0)
                    for d in devices:
                        if d.name == DEVICE_NAME:
                            device = d
                            print(f"Dispositivo encontrado por nombre: {device.name} ({device.address})")
                            break
        
                if device is None:
                    print(f"No se pudo encontrar el dispositivo con nombre '{DEVICE_NAME}' o dirección '{DEVICE_ADDRESS}'. Asegúrate de que esté encendido y anunciando.")
                    return

                print(f"Conectando a {device.name} ({device.address})...")
                    async with BleakClient(device) as client:
                        client_is_connected = await client.connect()
                        if client_is_connected:
                            print(f"Conectado a {client.address}")
                            try:
                                await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                                print(f"Suscrito a la característica: {CHARACTERISTIC_UUID}")
                                while client_is_connected:
                                    await asyncio.sleep(1)  # Mantener la conexión abierta y esperar notificaciones
                            except Exception as e:
                                print(f"Error durante la conexión/suscripción: {e}")
                            finally:
                                if client_is_connected:
                                    try:
                                        await client.stop_notify(CHARACTERISTIC_UUID)
                                        print("Notificaciones detenidas.")
                                    except Exception as e:
                                        print(f"Error al detener notificaciones: {e}")
                                await client.disconnect()
                                print("Desconectado.")
                        else:
                            print("Fallo al conectar.")
                                                        

if __name__ == "__main__":
    try:
        asyncio.run(run_ble_client())
    except KeyboardInterrupt:
        print("\nPrograma detenido por el usuario.")
    finally:
        # Asegúrate de apagar el LED al salir (si tienes lógica para el LED aquí)
        print("Programa finalizado.")
