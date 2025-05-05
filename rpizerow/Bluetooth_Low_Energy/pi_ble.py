import asyncio
from bleak import BleakClient, BleakScanner
import sys
from gpiozero import LED # Importar LED de gpiozero
import signal # Para limpieza

# --- Configuración ---
# LED conectado al GPIO 17
LED_PIN = 17
try:
    led = LED(LED_PIN)
    led.off() # Asegurar que empieza apagado
    print(f"PI Client: LED configurado en GPIO {LED_PIN}")
except Exception as e:
    print(f"PI Client: Error al configurar LED en GPIO {LED_PIN}: {e}")
    print("PI Client: El script continuará sin control de LED.")
    led = None # Marcar que no hay LED disponible

# Nombre del ESP32 al que conectar
DEVICE_NAME = "Mi_ESP32_SignaLink"
# Dirección MAC (opcional)
# DEVICE_ADDRESS = "E8:06:90:65:C6:3A" # Reemplaza con la MAC real si la usas

# UUIDs (DEBEN COINCIDIR EXACTAMENTE CON EL ESP32)
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

stop_event = asyncio.Event() # Para manejar cierre limpio

# --- Callback para Notificaciones ---
def notification_handler(sender: int, data: bytearray):
    """Se ejecuta cuando llega una notificación 'ON'/'OFF' del ESP32."""
    global led
    try:
        # Decodificar el mensaje recibido (debe ser "ON" o "OFF")
        message = data.decode('utf-8').strip().upper()
        print(f"PI Client: Notificación recibida -> Mensaje: '{message}'")

        # Controlar el LED basado en el mensaje
        if led: # Solo si el LED se configuró correctamente
            if message == "ON":
                led.on()
                print("PI Client: -> LED Encendido")
            elif message == "OFF":
                led.off()
                print("PI Client: -> LED Apagado")
            else:
                print(f"PI Client: -> Mensaje no reconocido: {message}")
        else:
            print("PI Client: -> (Control de LED no disponible)")

    except Exception as e:
        print(f"PI Client: Error en notification_handler: {e}")

# --- Cliente BLE Principal ---
async def run_ble_client():
    global led
    print(f"PI Client: Buscando dispositivo: {DEVICE_NAME}...")
    device = None

    # Búsqueda (preferible por nombre para empezar)
    print(f"PI Client: Buscando por nombre {DEVICE_NAME}...")
    devices = await BleakScanner.discover(timeout=10.0)
    for d in devices:
        if d.name == DEVICE_NAME:
            device = d
            print(f"PI Client: Dispositivo encontrado por nombre: {device.address}")
            break

    if not device:
        print(f"PI Client: No se pudo encontrar el dispositivo {DEVICE_NAME}.")
        return # Salir limpiamente si no se encuentra

    print(f"PI Client: Conectando a {device.name} ({device.address})...")
    try:
        async with BleakClient(device.address) as client:
            if client.is_connected:
                print("PI Client: ¡Conectado exitosamente al ESP32!")

                try:
                    print(f"PI Client: Suscribiéndose a notificaciones de {CHARACTERISTIC_UUID}...")
                    await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                    print("PI Client: ¡Suscrito exitosamente! Esperando mensajes ON/OFF...")

                    # Mantener la conexión abierta y esperar evento de parada
                    await stop_event.wait() # Espera hasta que Ctrl+C sea presionado

                except Exception as e:
                    print(f"PI Client: Error durante la suscripción o espera: {e}")
                finally:
                    print("PI Client: Desconectando / Limpiando...")
                    if client.is_connected:
                        try:
                            print("PI Client: Deteniendo notificaciones...")
                            # No esperar la parada si ya estamos cerrando por señal
                            # await client.stop_notify(CHARACTERISTIC_UUID)
                            # Lanzar y olvidar puede ser mejor durante el cierre forzado
                            asyncio.create_task(client.stop_notify(CHARACTERISTIC_UUID))
                            print("PI Client: Solicitud para detener notificaciones enviada.")
                        except Exception as e:
                            print(f"PI Client: Error al detener notificaciones: {e}")
            else:
                print("PI Client: Fallo al conectar.")
    except Exception as e:
        print(f"PI Client: Error al conectar o durante la conexión: {e}")

# --- Manejo de Cierre Limpio ---
def signal_handler(sig, frame):
    print("\nPI Client: Señal de parada recibida, cerrando conexión...")
    stop_event.set()

# --- Ejecución ---
if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler) # Captura Ctrl+C
    signal.signal(signal.SIGTERM, signal_handler) # Captura otras señales de terminación

    try:
        asyncio.run(run_ble_client())
    except KeyboardInterrupt:
        # Esto no debería ocurrir si signal_handler funciona, pero por si acaso
        print("\nPI Client: KeyboardInterrupt (nivel superior)")
    finally:
        print("PI Client: Script finalizado.")
        if led:
            led.off() # Asegurar que el LED quede apagado
            print("PI Client: LED apagado.")
