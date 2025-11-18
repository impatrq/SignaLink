#!/bin/bash

# Nombre y datos de la red WiFi
SSID="MOVISTAR-WIFI6-PATRICIA"
PASSWORD="patricia"
NOMBRE_CONEXION="4G"

echo "Agregando red WiFi '$SSID' como conexión '$NOMBRE_CONEXION'..."

# Crear conexión WiFi con contraseña WPA-PSK
sudo nmcli connection add type wifi ifname wlan0 con-name "$NOMBRE_CONEXION" ssid "$SSID"

# Configurar seguridad WPA2 con contraseña
sudo nmcli connection modify "$NOMBRE_CONEXION" wifi-sec.key-mgmt wpa-psk
sudo nmcli connection modify "$NOMBRE_CONEXION" wifi-sec.psk "$PASSWORD"

# Hacer que se conecte automáticamente
sudo nmcli connection modify "$NOMBRE_CONEXION" connection.autoconnect yes

echo " [+] Conexión guardada. Se conectará automáticamente a '$SSID'"
