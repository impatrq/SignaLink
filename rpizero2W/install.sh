#!/bin/bash

# Author: Signalink

#Colours
greenColour="\e[0;32m\033[1m"
endColour="\033[0m\e[0m"
redColour="\e[0;31m\033[1m"
blueColour="\e[0;34m\033[1m"
yellowColour="\e[0;33m\033[1m"
purpleColour="\e[0;35m\033[1m"
turquoiseColour="\e[0;36m\033[1m"
grayColour="\e[0;37m\033[1m"

trap ctrl_c INT

function ctrl_c(){

	echo -e "\n${yellowColour}[*]${endColour}${grayColour} Exiting...${endColour}"; sleep 1
	rm $tmp_file 2>/dev/null
	tput cnorm; exit 1
}

# --- Script de instalación para el proyecto SignaLink en Raspberry Pi Zero 2W ---
echo -e "\n${yellowColour}=========================================${endColour}"
echo -e "${yellowColour}  Iniciando la instalación de SignaLink  ${endColour}"
echo -e "${yellowColour}=========================================${endColour}"

# Asegurarse de que el script se detenga si hay un error
set -e

# --- 1. Actualizar sistema e instalar dependencias necesarias ---
echo -e "${blueColour}\n[+] Actualizando el sistema...${endColour}"
sudo apt update
sudo apt upgrade -y 
echo -e "${greenColour}[+] Sistema Actualizado${endColour}\n"
echo -e "\n${blueColour}[+] Instalando dependencias necesarias${endColour}"
sudo apt install -y python3-pip python3-venv python3-pyaudio git espeak-ng portaudio19-dev libasound2-dev mosquitto mosquitto-clients
echo -e "${greenColour}[+] Dependencias del sistema instaladas.${endColour}"

# --- 2. Crear el entorno virtual de Python ---
echo -e "\n${blueColour}[+] Creando el entorno virtual 'venv'...${endColour}"
if [ ! -d "venv" ]; then
	python3 -m venv venv
	echo -e "${greenColour}[+] Entorno virtual creado exitosamente${endColour}\n"
else 
	echo -e "${redColour}El entorno virtual 'venv' ya existe. Omitiendo creación.${endColour}\n"
fi

# --- 3. Instalar dependencias de Python con pip ---
echo -e "\n${turquoiseColour}[+] Activando entorno virtual e instalando paquetes desde requirements.txt...${endColour}"
source venv/bin/activate 
pip install -r requirements.txt
deactivate
echo -e "${greenColour}[+]Dependencias de Python instaladas exitosamente.${endColour}"

# --- 4. Finalización ---
echo -e "\n${purpleColour}¡Instalación completada!${endColour}"
echo -e "\n${yellowColour}================================================================${endColour}"
echo -e "${yellowColour}  Para activar el entorno virtual, ejecuta:${endColour}"
echo -e "${yellowColour}  source venv/bin/activate${endColour}"
echo -e "${yellowColour}================================================================${endColour}\n"
