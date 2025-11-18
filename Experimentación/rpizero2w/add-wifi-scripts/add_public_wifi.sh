#!/bin/bash

# Nombre de la red WiFi del colegio
SSID=""
NOMBRE_CONEXION=""

echo "Agregando red WiFi '$SSID' como conexion '$NOMBRE_CONEXION'..."

# Crear conexión WiFi abierta
sudo nmcli connection add type wifi ifname wlan0 con-name "$NOMBRE_CONEXION" ssid "$SSID"

# Especificar que no tiene seguridad (abierta)
sudo nmcli connection modify "$NOMBRE_CONEXION" wifi-sec.key-mgmt none

# Hacer que se conecte automáticamente cuando esté disponible
sudo nmcli connection modify "$NOMBRE_CONEXION" connection.autoconnect yes

echo " [+] Conexion guardada. Se conectara automaticamente a '$NOMBRE_CONEXION'"
