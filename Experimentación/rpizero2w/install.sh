#!/bin/bash

# Author: Signalink

# Colours
greenColour="\e[0;32m\033[1m"
endColour="\033[0m\e[0m"
redColour="\e[0;31m\033[1m"
blueColour="\e[0;34m\033[1m"
yellowColour="\e[0;33m\033[1m"
purpleColour="\e[0;35m\033[1m"
turquoiseColour="\e[0;36m\033[1m"
grayColour="\e[0;37m\033[1m"

# Variables
SERVICE_FILE="signalink.service"
SERVICE_PATH="/etc/systemd/system/$SERVICE_FILE"
MOSQUITTO_CONF_FILE="mosquitto_conf/default.conf"
MOSQUITTO_PASSWD_FILE="mosquitto_conf/passwd"
HOSTAPD_CONF_FILE="wifi_ap/hostapd.conf"
DNSMASQ_CONF_FILE="wifi_ap/dnsmasq.conf"

tmp_file=$(mktemp) # Archivo temporal para Mosquitto

trap ctrl_c INT

function ctrl_c(){

	echo -e "\n${yellowColour}[*]${endColour}${grayColour} Exiting...${endColour}"; sleep 1
	rm $tmp_file 2>/dev/null
	tput cnorm; exit 1
}

# --- Script de instalación para el proyecto SignaLink ---
echo -e "\n${yellowColour}=========================================${endColour}"
echo -e "${yellowColour}  Iniciando la instalación de SignaLink  ${endColour}"
echo -e "${yellowColour}=========================================${endColour}"

set -e

# 1. Actualizar sistema e instalar dependencias necesarias.
echo -e "${blueColour}\n[+] Actualizando el sistema...${endColour}"

sudo apt update
sudo apt upgrade -y 

# 2. Instalar dependencias del sistema.
echo -e "\n${blueColour}[+] Instalando dependencias necesarias${endColour}"

sudo apt install -y \
    python3-pip python3-venv python3-dev \
    portaudio19-dev libasound2-dev alsa-utils \
    espeak-ng \
    i2c-tools \
    python3-pyaudio python3-serial python3-smbus python3-spidev \
    mosquitto mosquitto-clients \
    hostapd dnsmasq \
    git build-essential
    
echo -e "${greenColour}[+] Dependencias del sistema instaladas.${endColour}"

# 3. Configuración de Mosquitto (MQTT Broker).
echo -e "\n${blueColour}[+] Configurando el broker Mosquitto (MQTT)...${endColour}"

MOSQUITTO_PASSWD_PATH="/etc/mosquitto/passwd"
MOSQUITTO_CONFIG_PATH="/etc/mosquitto/conf.d/default.conf"

# Crear el archivo de contraseñas de Mosquitto
if [ ! -f "$MOSQUITTO_PASSWD_FILE" ]; then
	echo -e "${redColour}[!] Error: Archivo de contraseñas de Mosquitto '$MOSQUITTO_PASSWD_FILE' no encontrado.${endColour}"
	exit 1
fi
sudo cp "$MOSQUITTO_PASSWD_FILE" "$MOSQUITTO_PASSWD_PATH"
sudo chown mosquitto:mosquitto "$MOSQUITTO_PASSWD_PATH"
sudo chmod 600 "$MOSQUITTO_PASSWD_PATH"
echo -e "${greenColour}    - Archivo de contraseñas copiado y permisos establecidos.${endColour}"

# Copiar la configuración de Mosquitto
if [ ! -f "$MOSQUITTO_CONF_FILE" ]; then
	echo -e "${redColour}[!] Error: Archivo de configuración de Mosquitto '$MOSQUITTO_CONF_FILE' no encontrado.${endColour}"
	exit 1
fi
sudo cp "$MOSQUITTO_CONF_FILE" "$MOSQUITTO_CONFIG_PATH"
echo -e "${greenColour}    - Archivo de configuración copiado a '$MOSQUITTO_CONFIG_PATH'.${endColour}"

# Reiniciar Mosquitto para aplicar la nueva configuración
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
echo -e "${greenColour}[+] Mosquitto configurado y reiniciado.${endColour}"

# 4. Configuración del Access Point (HostAPD y dnsmasq).
echo -e "\n${blueColour}[+] Configurando HostAPD y dnsmasq para SignaLink AP...${endColour}"

HOSTAPD_CONFIG_PATH="/etc/hostapd/hostapd.conf"
DNSMASQ_CONFIG_PATH="/etc/dnsmasq.conf"

# Copiar configuración de HostAPD
if [ ! -f "$HOSTAPD_CONF_FILE" ]; then
	echo -e "${redColour}[!] Error: Archivo de configuración de HostAPD '$HOSTAPD_CONF_FILE' no encontrado.${endColour}"
	exit 1
fi
sudo cp "$HOSTAPD_CONF_FILE" "$HOSTAPD_CONFIG_PATH"
echo -e "${greenColour}    - Configuración de HostAPD copiada.${endColour}"

# Mover la configuración de dnsmasq a un backup y copiar la nueva
if [ ! -f "$DNSMASQ_CONF_FILE" ]; then
	echo -e "${redColour}[!] Error: Archivo de configuración de dnsmasq '$DNSMASQ_CONF_FILE' no encontrado.${endColour}"
	exit 1
fi
sudo mv "$DNSMASQ_CONFIG_PATH" "$DNSMASQ_CONFIG_PATH.bak" 2>/dev/null || true
sudo cp "$DNSMASQ_CONF_FILE" "$DNSMASQ_CONFIG_PATH"
echo -e "${greenColour}    - Configuración de dnsmasq copiada.${endColour}"

# Habilitar servicios AP (no los iniciamos aquí ya que requieren configuración de IP estática/interfaces)
sudo systemctl unmask hostapd 2>/dev/null || true
sudo systemctl enable hostapd
sudo systemctl enable dnsmasq
echo -e "${yellowColour}[*] Los servicios HostAPD y dnsmasq están habilitados, pero requerirán una configuración manual de IP estática en 'wlan0' antes de iniciarse.${endColour}"

# 5. Crear el entorno virtual de Python
echo -e "\n${blueColour}[+] Creando el entorno virtual 'venv'...${endColour}"
if [ ! -d "venv" ]; then
	python3 -m venv venv
	echo -e "${greenColour}[+] Entorno virtual creado exitosamente${endColour}\n"
else 
	echo -e "${redColour}El entorno virtual 'venv' ya existe. Omitiendo creación.${endColour}\n"
fi

# 6. Instalar dependencias de Python con pip.
echo -e "\n${turquoiseColour}[+] Activando dependencias Python...${endColour}"
source venv/bin/activate 
pip install --upgrade pip setuptools wheel
pip install -r requirements.txt
deactivate

# 7. Instalar y habilitar servicio systemd.
echo -e "${blue}[+] Configurando servicio systemd...${end}"

# Copiar servicio al directorio de systemd
sudo cp $SERVICE_FILE $SERVICE_PATH

# Recargar systemd para reconocerlo
sudo systemctl daemon-reload

# Habilitar para que arranque al boot
sudo systemctl enable $SERVICE_FILE

# Arrancar ahora mismo
sudo systemctl start $SERVICE_FILE

echo -e "${green}[+] Servicio SignaLink habilitado y arrancado${end}"

# 5. Finalización.
echo -e "\n${purpleColour}[+] Instalación completada!${endColour}"
echo -e "\n${yellowColour}================================================================${endColour}"
echo -e "${yellowColour}  Para activar el entorno virtual, ejecuta:${endColour}"
echo -e "${yellowColour}  source venv/bin/activate${endColour}"
echo -e "${yellowColour}================================================================${endColour}\n"
