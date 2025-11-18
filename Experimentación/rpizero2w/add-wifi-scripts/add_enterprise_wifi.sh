#!/bin/bash

# Datos de la red WiFi de UADE
SSID="UADE"
USER="flesmegarcia@uade.edu.ar"
PASS="_fr4nco_06"
NOMBRE_CONEXION="uade-wifi"

echo "Agregando red WiFi '$SSID' como conexión '$NOMBRE_CONEXION'..."

# Crear la conexión WiFi
sudo nmcli connection add type wifi ifname wlan0 con-name "$NOMBRE_CONEXION" ssid "$SSID"

# Configurar WPA2-Enterprise (PEAP + MSCHAPv2, sin validar certificado)
sudo nmcli connection modify "$NOMBRE_CONEXION" \
    wifi-sec.key-mgmt wpa-eap \
    802-1x.eap peap \
    802-1x.phase2-auth mschapv2 \
    802-1x.identity "$USER" \
    802-1x.password "$PASS" \
    802-1x.system-ca-certs no

# Autoconexión
sudo nmcli connection modify "$NOMBRE_CONEXION" connection.autoconnect yes

echo " [+] Conexión guardada. Se conectará automáticamente a '$SSID'"
