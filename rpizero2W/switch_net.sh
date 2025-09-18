#!/bin/bash

# --- Colores para la salida ---
greenColour="\e[0;32m\033[1m"
endColour="\033[0m\e[0m"
redColour="\e[0;31m\033[1m"
blueColour="\e[0;34m\033[1m"
yellowColour="\e[0;33m\033[1m"
grayColour="\e[0;37m\033[1m"

trap ctrl_c INT

function ctrl_c(){
	echo -e "\n${redColour}[!] Operación cancelada. Restaurando modo Access Point...${endColour}"
	# Intentar restaurar el modo AP en caso de interrupción
	sudo systemctl stop wpa_supplicant &>/dev/null
	sudo systemctl stop NetworkManager &>/dev/null
	sudo systemctl start hostapd &>/dev/null
	sudo systemctl start dnsmasq &>/dev/null
	echo -e "${greenColour}[+] Modo Access Point restaurado.${endColour}"
	exit 1
}

echo -e "${blueColour}[*] Cambiando a modo Wi-Fi Cliente (conexión a Internet)...${endColour}"

# Detener servicios de Access Point
sudo systemctl stop hostapd
sudo systemctl stop dnsmasq

# Iniciar servicios de Cliente Wi-Fi
sudo systemctl restart wpa_supplicant
sudo systemctl restart NetworkManager

echo -e "${yellowColour}[*] Esperando 15 segundos para establecer conexión a Internet...${endColour}"
sleep 15

# Verificar conexión
if ping -c 1 8.8.8.8 &> /dev/null; then
    echo -e "${greenColour}[+] Conexión a Internet establecida.${endColour}"
else
    echo -e "${redColour}[!] No se pudo conectar a Internet. Revisa la configuración de wpa_supplicant.conf.${endColour}"
fi

echo -e "\n${yellowColour}--- Modo Cliente Activado ---${endColour}"
echo -e "${grayColour}Ahora puedes ejecutar los comandos que necesites (ej. git pull, sudo apt update).${endColour}"
echo -e "${yellowColour}Presiona ENTER cuando hayas terminado para volver al modo Access Point.${endColour}"
read -r

echo -e "\n${blueColour}[*] Volviendo a modo Access Point (AP)...${endColour}"

# Detener servicios de Cliente Wi-Fi
sudo systemctl stop wpa_supplicant
sudo systemctl stop NetworkManager
sudo systemctl start hostapd
sudo systemctl start dnsmasq
