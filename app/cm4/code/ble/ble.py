#!/usr/bin/env python3
import dbus
import dbus.exceptions
import dbus.mainloop.glib
import dbus.service
from gi.repository import GLib

BLUEZ_SERVICE_NAME = 'org.bluez'
GATT_MANAGER_IFACE = 'org.bluez.GattManager1'
GATT_SERVICE_IFACE = 'org.bluez.GattService1'
GATT_CHRC_IFACE = 'org.bluez.GattCharacteristic1'

PATH_BASE = '/org/bluez/example'

# -------------------------------
# Característica simple de lectura
# -------------------------------
class ExampleCharacteristic(dbus.service.Object):
    def __init__(self, bus, index, service):
        self.path = service.path + '/char' + str(index)
        self.bus = bus
        self.uuid = '12345678-1234-5678-1234-56789abcdef1'
        self.flags = ['read']
        self.service = service
        dbus.service.Object.__init__(self, bus, self.path)

    @dbus.service.method(dbus_interface=GATT_CHRC_IFACE,
                         in_signature='a{sv}', out_signature='ay')
    def ReadValue(self, options):
        print("📖 Characteristic read")
        value = [dbus.Byte(c.encode()) for c in "Hola desde la Raspberry!"]
        return value

    @dbus.service.method(dbus_interface='org.freedesktop.DBus.Properties',
                         in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface):
        if interface != GATT_CHRC_IFACE:
            raise dbus.exceptions.DBusException('org.freedesktop.DBus.Error.InvalidArgs')
        return {
            'UUID': self.uuid,
            'Service': self.service.get_path(),
            'Flags': dbus.Array(self.flags, signature='s'),
        }

    def get_path(self):
        return dbus.ObjectPath(self.path)


# -------------------------------
# Servicio GATT
# -------------------------------
class ExampleService(dbus.service.Object):
    def __init__(self, bus, index):
        self.path = PATH_BASE + '/service' + str(index)
        self.bus = bus
        self.uuid = '12345678-1234-5678-1234-56789abcdef0'
        self.primary = True
        self.characteristics = [ExampleCharacteristic(bus, 0, self)]
        dbus.service.Object.__init__(self, bus, self.path)

    @dbus.service.method(dbus_interface='org.freedesktop.DBus.Properties',
                         in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface):
        if interface != GATT_SERVICE_IFACE:
            raise dbus.exceptions.DBusException('org.freedesktop.DBus.Error.InvalidArgs')
        return {
            'UUID': self.uuid,
            'Primary': self.primary,
            'Characteristics': dbus.Array(
                [c.get_path() for c in self.characteristics],
                signature='o'
            ),
        }

    def get_path(self):
        return dbus.ObjectPath(self.path)


# -------------------------------
# Aplicación GATT (con GetManagedObjects)
# -------------------------------
class Application(dbus.service.Object):
    def __init__(self, bus):
        self.path = PATH_BASE
        self.bus = bus
        self.services = [ExampleService(bus, 0)]
        dbus.service.Object.__init__(self, bus, self.path)

    @dbus.service.method('org.freedesktop.DBus.ObjectManager',
                         out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        managed_objects = {}
        for service in self.services:
            managed_objects[service.get_path()] = {
                GATT_SERVICE_IFACE: service.GetAll(GATT_SERVICE_IFACE)
            }
            for chrc in service.characteristics:
                managed_objects[chrc.get_path()] = {
                    GATT_CHRC_IFACE: chrc.GetAll(GATT_CHRC_IFACE)
                }
        return managed_objects

    def get_path(self):
        return dbus.ObjectPath(self.path)


# -------------------------------
# Main
# -------------------------------
def main():
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()

    service_manager = dbus.Interface(
        bus.get_object(BLUEZ_SERVICE_NAME, '/org/bluez/hci0'),
        GATT_MANAGER_IFACE
    )

    app = Application(bus)
    print("📡 Registering GATT app…")

    try:
        service_manager.RegisterApplication(
            app.get_path(),
            {},
            reply_handler=lambda: print("✅ GATT app registered!"),
            error_handler=lambda e: print("❌ Failed:", e)
        )
    except Exception as e:
        print("❌ Exception:", e)
        return

    mainloop = GLib.MainLoop()
    mainloop.run()


if __name__ == '__main__':
    main()
