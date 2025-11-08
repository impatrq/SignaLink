#!/usr/bin/env python3
import asyncio
import time
import threading
from dbus_next.aio import MessageBus
from dbus_next import Variant, Message
from dbus_next.service import ServiceInterface, method, dbus_property, PropertyAccess

# Constantes del Servicio y Características
SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHAR_RW_UUID = "12345678-1234-5678-1234-56789abcdef1"
CHAR_NOTIFY_UUID = "12345678-1234-5678-1234-56789abcdef2"

_char_value = bytearray(b"Hola desde SignaLinkCM4")

# Variables de configuración del reintento
MAX_RETRIES = 5
RETRY_DELAY = 2  # Segundos

# ----------------- ADVERTISEMENT -----------------

class LEAdvertisement(ServiceInterface):
    def __init__(self, index):
        super().__init__("org.bluez.LEAdvertisement1")
        self.path = f"/org/bluez/example/advertisement{index}"

    @dbus_property(access=PropertyAccess.READ)
    def Type(self) -> "s":
        return "peripheral"

    @dbus_property(access=PropertyAccess.READ)
    def LocalName(self) -> "s":
        return "SignaLinkCM4"

    @dbus_property(access=PropertyAccess.READ)
    def ServiceUUIDs(self) -> "as":
        return [SERVICE_UUID]

    @dbus_property(access=PropertyAccess.READ)
    def Includes(self) -> "as":
        return ["tx-power"]

    @dbus_property(access=PropertyAccess.READ)
    def Duration(self) -> "q":
        return 0

    @dbus_property(access=PropertyAccess.READ)
    def Timeout(self) -> "q":
        return 0

    @method()
    def Release(self):
        # Esta función es llamada por BlueZ cuando deja de necesitar el advertising.
        print("Advertisement liberado por BlueZ.")
        pass


# ----------------- GATT APPLICATION -----------------

class GattApplication(ServiceInterface):
    def __init__(self, path):
        super().__init__("org.bluez.GattApplication1")
        self.path = path
        self.services = []

    def get_paths(self):
        paths = [self.path]
        for srv in self.services:
            paths += srv.get_paths()
        return paths


# ----------------- SERVICE -----------------

class GattService(ServiceInterface):
    def __init__(self, index, uuid, primary):
        super().__init__("org.bluez.GattService1")
        self.path = f"/org/bluez/example/service{index}"
        self.uuid = uuid
        self.primary = primary
        self.characteristics = []

    @dbus_property(access=PropertyAccess.READ)
    def UUID(self) -> "s":
        return self.uuid

    @dbus_property(access=PropertyAccess.READ)
    def Primary(self) -> "b":
        return self.primary

    def get_paths(self):
        paths = [self.path]
        for c in self.characteristics:
            paths += c.get_paths()
        return paths


# ----------------- CHARACTERISTIC -----------------

class GattCharacteristic(ServiceInterface):
    def __init__(self, index, uuid, flags, service):
        super().__init__("org.bluez.GattCharacteristic1")
        self.path = service.path + f"/char{index}"
        self.uuid = uuid
        self.flags = flags
        self.service = service
        self.notifying = False

    @dbus_property(access=PropertyAccess.READ)
    def UUID(self) -> "s":
        return self.uuid

    @dbus_property(access=PropertyAccess.READ)
    def Flags(self) -> "as":
        return self.flags

    @method()
    def ReadValue(self, options: "a{sv}") -> "ay":
        # Devuelve el valor actual de la variable global _char_value
        return bytes(_char_value)

    @method()
    def WriteValue(self, value: "ay", options: "a{sv}"):
        global _char_value
        decoded = bytes(value).decode()
        print("[BLE WRITE] Recibido:", decoded)
        # Actualiza el valor para simular una respuesta de ACK
        _char_value = bytearray(f"ACK: {decoded}", "utf-8")
        # Opcional: notificar a los clientes que el valor de la característica RW ha cambiado
        # self.emit_properties_changed({"Value": Variant("ay", bytes(_char_value))})

    @method()
    def StartNotify(self):
        print("[BLE NOTIFY] Notificaciones iniciadas.")
        self.notifying = True

    @method()
    def StopNotify(self):
        print("[BLE NOTIFY] Notificaciones detenidas.")
        self.notifying = False

    # BlueZ puede leer la propiedad directamente, aunque el método ReadValue también existe.
    @dbus_property(access=PropertyAccess.READ)
    def Value(self) -> "ay":
        return bytes(_char_value)

    def send_notify(self, data: bytes):
        """Emite la señal de cambio de propiedad para enviar la notificación BLE."""
        if self.notifying:
            # Es crucial usar Variant("ay", data) para el tipo array-of-bytes
            self.emit_properties_changed({"Value": Variant("ay", data)})

    def get_paths(self):
        return [self.path]


# ----------------- NOTIFY THREAD -----------------

def notify_loop(ch: GattCharacteristic):
    """Bucle que simula el envío de datos de notificación cada 5 segundos."""
    while True:
        time.sleep(5)
        if ch.notifying:
            payload = f"tick {int(time.time())}".encode()
            print("[BLE NOTIFY]", payload)
            ch.send_notify(payload)


# ----------------- MAIN -----------------

async def main():
    # Conectar al bus de sistema (donde corre BlueZ)
    bus = await MessageBus().connect()

    # Construir la estructura GATT
    app = GattApplication("/org/bluez/example")
    service = GattService(0, SERVICE_UUID, True)
    c_rw = GattCharacteristic(0, CHAR_RW_UUID, ["read", "write"], service)
    c_nt = GattCharacteristic(1, CHAR_NOTIFY_UUID, ["notify"], service)

    service.characteristics += [c_rw, c_nt]
    app.services.append(service)

    # Exportar los objetos GATT al bus de DBus
    for path in app.get_paths():
        if "service" in path:
            bus.export(path, service)
        elif "char0" in path:
            bus.export(path, c_rw)
        elif "char1" in path:
            bus.export(path, c_nt)
        else:
            bus.export(app.path, app)

    # Configurar la publicidad
    ad = LEAdvertisement(0)
    bus.export(ad.path, ad)

    # Registrar el anuncio con BlueZ
    # Usamos hci1, ya que hci0 no se pudo encender
    manager_path = "/org/bluez/hci1" 

    for attempt in range(1, MAX_RETRIES + 1):
        print(f"Intentando registrar advertising en {manager_path} (Intento {attempt}/{MAX_RETRIES})...")
        
        msg = Message(
            destination="org.bluez",
            path=manager_path,
            interface="org.bluez.LEAdvertisingManager1",
            member="RegisterAdvertisement",
            signature="oa{sv}",
            body=[ad.path, {}]
        )

        reply = await bus.call(msg)

        if reply.error_name is None:
            print(f"✅ BLE GATT + Advertising iniciado en {manager_path} — aparece en nRF Connect")
            break  # Éxito, salir del bucle
        
        if reply.error_name == "org.freedesktop.DBus.Error.ServiceUnknown":
            print(f"⚠️ Error de servicio desconocido. Reintentando en {RETRY_DELAY} segundos...")
            await asyncio.sleep(RETRY_DELAY)
        else:
            # Otro error diferente, abortar
            raise Exception(f"Error al registrar advertising: {reply.error_name}")
    else:
        # Esto se ejecuta si el bucle de reintento termina sin un 'break' (es decir, falló todos los intentos)
        raise Exception(f"Fallo al registrar advertising después de {MAX_RETRIES} intentos. El servicio no está disponible.")

    # Iniciar el hilo de notificaciones
    threading.Thread(target=notify_loop, args=(c_nt,), daemon=True).start()

    # Mantener el bucle de asyncio corriendo indefinidamente
    await asyncio.get_running_loop().create_future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception as e:
        print(f"Ha ocurrido un error en la ejecución: {e}")
        print("Si el error persiste, verifica que el adaptador utilizado esté encendido (powered) o que BlueZ esté en modo experimental.")
